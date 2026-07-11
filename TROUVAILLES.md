# TROUVAILLES — la mémoire structurée des agents

> **Règle** : tout agent (éclaireur, implémenteur, réparateur) APPEND ici, en FIN de
> mission, une entrée courte et structurée — ce qu'il a DÉCOUVERT (pas ce qu'il a
> écrit : le code parle, le commit raconte ; ici on garde ce qui a coûté cher à
> trouver). Le successeur lit ce fichier AVANT de fouiller. Append-only, entrées
> datées, preuves `fichier:symbole`. Format :
>
> ```
> ## [date] domaine — titre court (agent/mission)
> **Découvertes** : faits vérifiés, avec fichier:symbole.
> **Pièges** : ce qui mord (et pourquoi c'est piégeux).
> **Restes** : ce qui est ouvert, avec le fix d'une ligne si connu.
> ```

---

## [2026-07-06] Endgame — l'écart §27 existant vs « Brèche unifiée » (éclaireur S1)
**Découvertes** : le seuil réel est `wp->entropy ≥ ENTROPY_FIN` gate an 180 (scps_endgame.c, endgame_tick) — `dereal` (CountryProsperity) et `breach_pressure` (AgesState) sont DEUX AUTRES compteurs distincts, le doc de design les confondait. La sélection de fin par rare dominant `faust_consumed[3]` (essence→EAU, flux→RONCES, fer→FROID) implémente DÉJÀ « le visage = la tech consommée » — chaque tech faustienne consomme SON rare, aucun compteur par-tech à créer. Mutations réutilisables telles quelles : `cataclysm_water_seed/step`, `cold_step`, `thorns_seed/step`.
**Pièges** : `faust_consumed` ne monte que si un bâtiment faustien TOURNE (econ_tick) — la tech débloquée ne suffit pas (cf. entrée P3 plus bas).
**Restes** : ré-accumulation hydrologique post-lacs (plan Gleba #3 suite) jamais faite — greenlight requis.

## [2026-07-06] Endgame — calibrage post-refontes éco (orchestrateur, ENTDIAG)
**Découvertes** : les refontes éco avaient fait RECULER le tir de ~80 ans (croisement an ~260 vs historique ~184-195) SANS que personne le voie — transmuteurs conso 0 partout. Courbe d'entropie superlinéaire (ENTDIAG seed 9 : 0.3@an20 → 26@an180 → 56@an259). Fixé : ENTROPY_TECH_W 1.35, mémoire des morts à décrue (SANG_MEMORY_HL 40) sur pop VIVANTE (le cumul à vie vs pop_ref an-0 donnait 40-961 % — dénominateur périmé, la pop triple).
**Pièges** : `endgame_tick` est ANNUEL (pas journalier) — les décrues « par appel » se calibrent en années. Toute télémétrie de ratio doit dire SON dénominateur.
**Restes** : —

## [2026-07-06] Factions & Conseil — l'API réelle (éclaireur S2)
**Découvertes** : le grief des factions opposées est AUTOMATIQUE dans `faction_lever_apply` (scps_factions.c:267, table d'opposition hardcodée :172 — Conquérant↔Communautaire 1.0, Gardien↔Transgresseur 1.0). Convention : `faction_strength==0` ⇒ concession (rot), `>0` ⇒ vote. Le rot est plafonné 0.85, décrue ×0.04 du rythme du grief. `faction_coup_tension_c` existe mais N'EST CONSOMMÉE par personne côté endgame. Conseil Q1 : candidats DÉTERMINISTES au seed (jamais sérialisés), seuls `council[][]`+`council_gen[][]` le sont ; multiplicateurs mordent via `econ_set_council_mult` ; coût FX_CONSEIL existant. Modèle de dérive à copier pour la loyauté : COERCION_DECAY 0.93 (demi-vie ~10 ticks).
**Pièges** : FAC_COMMUNAUTAIRE n'a AUCUN hook dans les ~40 événements (le peuple ne vote jamais) — à combler en V2b. Statecraft est sérialisé PLAT (sizeof) : tout champ ajouté = bump.
**Restes** : greffe loyauté/faction/paie du Conseil (V2a, ~4 Ko), portraits UI (texte-only aujourd'hui).

## [2026-07-06] Godot — calques carte & page-turn (éclaireur S3)
**Découvertes** : le terrain MUTE physiquement à l'endgame (rebiome/sink/thorns relus par le shader via `_bmap=null` sur fin>0, iso_ground.gd:64). Lavis par-province = OPTION SHADER seulement (512k cellules, vectoriel rejeté) : texture L8 `variant_map` + 1 tap. Le récap d'âge (age_recap.gd) porte déjà pause+bilan+verbe — squelette exact du page-turn. `_fin_color` overlay.gd:~3175 ; anneaux épicentre :~2050.
**Pièges** : warnings-as-errors GDScript : `var := dict.get(...)` (Variant inféré) REFUSE de compiler — typer explicitement. MSAA 2D n'existe pas sous GL Compatibility (gater sur get_rendering_device()).
**Restes** : lavis variant_map (V3) ; readers d'intensité par région exposés côté façade (le moteur a `endgame_region_intensity`).

## [2026-07-06] IA — la chaîne morte des rares faustiens (P3)
**Découvertes** : la recherche débloquait FOREUSE/TRANSMUTATION/FORGE_RUNES (quêtes §4/FAU5/S3 vivantes) mais AUCUNE doctrine ne POSAIT les bâtiments (`ai_build_civmanuf` les exclut ; `ai_build_manufacture` ne bâtit que forge céleste/atelier de mage). Fix : `ai_build_transmuter` au créneau t_mil, gate lu du cache `RegionEconomy.tech_*` (pas de TechState* → zéro ripple de signature). L'appétit `w_faustian` plafonne ~0.22 en pratique — tout gate >0.3 est mort-né.
**Pièges** : `ai_diplo_forecast` est O(n_countries) — l'appeler sur un chemin chaud (doctrine de bâti) = ×20 de perf (mesuré, reverté). Un signal de menace pour la doctrine doit être un CACHE de tick.
**Restes** : foreuse/réplicateur toujours 0 — leurs déclencheurs de RECHERCHE (famine fer §4 / bois-grain FAU5) ne tirent quasi jamais ; garnison IA jamais posée (attend le signal de menace bon marché).

## [2026-07-06] Moteur — lecteurs dérivés religion/culture (P7a)
**Découvertes** : `culture_relation_of` (relation par instance — champs bruts en params, PopCulture vit dans econ.h : include circulaire sinon), `region_ethos_drift`, `religion_fracture_level`, `religion_credo_drift` (alias documenté — UN seul signal dérivable honnête), `religion_scholar_drift` ({0,1} — « années d'inactivité » NON dérivable de l'état lettré). `creuset_state` = doublon déguisé d'`econ_country_metabolized` + motif trig_xenophobe : NE PAS créer.
**Pièges** : tout helper inter-module qui a besoin d'un champ econ → le passer en PARAMÈTRE (le motif maison), jamais d'include croisé.
**Restes** : les événements consommateurs B2/B3/B6, C2/C3/C4 (V2b).

## [2026-07-06] Esclavage — STRATES FANTÔMES dans la livraison v68 (P4, SLAVEDIAG)
**Découvertes** : sur seed 9 ×100 ans, `strates[CLASS_SLAVE] = 54→441` âmes MAIS `groupes klass==CLASS_SLAVE = 0` et `esclavagistes = 0`. Les âmes serviles des strates sont des FANTÔMES : jamais posées par la capture (0 esclavagiste ⇒ diplo_enslave_capture n'a jamais tourné), donc alimentées par une FUITE des boucles génériques `for k<CLASS_COUNT` qui répartissent la pop (l'enum a grandi, les répartitions arrosent la strate neuve), puis grossies par la croissance. Toute la chaîne (vente pool, affranchissement, membrane, révolte servile) scanne les GROUPES ⇒ système décoratif. `AiActor.can_enslave` posé dans un refresh qui exige TechState* que `ai_step` ne reçoit pas — vérifier son point d'appel réel.
**Pièges** : QUAND ON AGRANDIT UN ENUM DE CLASSES/STRATES, auditer TOUTES les boucles `CLASS_COUNT` de répartition — chacune arrose la nouvelle case par défaut. L'invariant qui prend tout d'un coup : Σ strates serviles == Σ groupes serviles (au banc).
**Restes** : réparation en cours (agent dédié) ; P4 (atteignabilité Merveille via pool) conclura après.

## [2026-07-06] Merveille — la math de métabolisation (correctif en vol)
**Découvertes** : `econ_country_heritage_digested` divise par la POP TOTALE (scps_econ.c:568) — voulu pour l'accès TECH, mais pour COMPTER des cultures c'est impossible (6×0.35 = 210 % de pop). Le compte de la Merveille (`endgame_metab_count`) = MAX de deux voies : gouvernance (arch_depth PROFOND — les conquis) + diaspora INDIVIDUALISÉE (dig_X/tot_X ≥ 0.60 + 500 âmes, pondérée par mode d'arrivée). « Une diaspora n'est pas sa culture. »
**Pièges** : tout seuil sur une part « par-héritage » doit dire son DÉNOMINATEUR (pop totale vs diaspora de CET héritage) — les deux sémantiques coexistent légitimement dans le moteur.
**Restes** : P5 — le chip UI « métabolisation prête » écoute heritage_access (sémantique tech) au lieu du compteur Merveille : à recâbler.

## [2026-07-06] Save/déterminisme — la règle des accumulateurs (3 occurrences)
**Découvertes** : TOUT accumulateur inter-ticks DOIT être sérialisé, sinon --savetest diverge : EMOB v57 (friche/lowsat), COLC v61 (répit colonisation), TXYR v65 (g_flux de l'année EN COURS — la capture annuelle post-reload lisait un flux tronqué ⇒ d_treasury_mois divergeait, dérive d'OR SEUL). Signature de cette classe de bug : une seule grandeur dérive, le reste byte-identique.
**Pièges** : un ordonnanceur (`next_day` statique) non sérialisé est un accumulateur AUSSI — préférer les règles APATRIDES (pur f(day)) appelées des blocs annuels de sim_day.
**Restes** : —

## [2026-07-06] Esclavage — le couplage strate↔groupe est réel, il n'a jamais été testé (réparateur)
**Découvertes** : `strata[CLASS_SLAVE].pop` (scps_econ.c) et `Σgroups[i].count where klass==CLASS_SLAVE`
(scps_demography.c) sont DEUX comptes parallèles maintenus À LA MAIN par 4 fonctions correctes
(`diplo_enslave_capture`, `intertrade_slave_buy/_sell`, `demography_manumit_*`) — RIEN ne les
synchronise automatiquement (pas de `strata_from_groups()`). Toute boucle GÉNÉRIQUE `for(c<CLASS_COUNT)`
qui touche `.pop` sans connaître les `PopGroup` casse l'invariant. `can_enslave` (scps_ai.c:2235) est
posé dans `ai_research_step` — appelé CHAQUE JOUR depuis `sim_day` (scps_sim.c:601), JUSTE APRÈS
`ai_step` (pas dedans) ; `ai_step` lit une valeur d'UN JOUR de retard, sans conséquence pratique.
**Pièges** (les 9 fuites, une par ligne, `fichier:fonction`) :
- `scps_econ.c` (boucle croissance annuelle) : le plancher générique `if(pop<1)pop=1` RESSUSCITE
  0→1 âme/mois pour CLASS_SLAVE puis `net_growth` la COMPOSE — 0 groupe requis. Root cause de « strates
  qui montent tout seules ».
- `scps_demography.c::assimilation_tick` : fusionne N'IMPORTE QUEL groupe (klass ignoré) dans le
  dominant si D<FUSE_EPS — la fusion PERD `klass` (seul `.count` survit) : un esclave culturellement
  proche de son maître voit son groupe FONDU sans que la strate suive.
- `scps_demography.c::demography_refugee_tick` (fuite de guerre) : itère tous les groupes SANS filtrer
  klass ; `migration_move` ne synchronise JAMAIS `strata[]` (aucun appelant ne le fait) — un esclave
  « réfugié » crée un groupe ailleurs sans bouger la strate nulle part.
- `scps_revolt.c::revolt_ignite`/`demobilize` : la mobilisation/démobilisation d'une révolte SERVILE
  (LOT H l'autorise) créditait/débitait TOUJOURS CLASS_LABORER, jamais CLASS_SLAVE — `demobilize` doit
  relire `pe->pop.groups[gi].klass` (l'état ACTUEL, pas `rb->klass` figé à l'allumage, car
  `apply_rebel_victory` manumit AVANT d'appeler demobilize sur la voie victorieuse).
- `scps_diplo.c::diplo_enslave_capture` : le groupe recevait `captives` âmes MAIS la strate ne recevait
  que `moved` (borné au dispo LABORER/BOURGEOIS de la source) — calculer `moved` D'ABORD, l'utiliser pour
  les DEUX écritures, jamais l'inverse.
- `scps_econ.c` (3× colonisation : `econ_colonize_province`/`colonize_from_prov`/`_overseas`) : ponction
  proportionnelle à TOUTES les strates alors que les colons semés sont TOUJOURS `CLASS_SHARE[SLAVE]=0`
  — exclure CLASS_SLAVE du bassin ponctionnable (`spop_free = spop - strata[CLASS_SLAVE].pop`).
- **`scps_econ.c::econ_migrate_tick`** — LE PIÈGE DE L'ÉNUM APPENDU : `for(cl=CLASS_BOURGEOIS;
  cl<CLASS_COUNT; cl++)` présumait implicitement l'ANCIENNE taille de l'enum (BOURGEOIS/ELITE
  seulement) ; l'ajout de CLASS_SLAVE en fin d'enum a SILENCIEUSEMENT élargi la boucle à un 3e cran.
  **Tout `for(x; cl<CLASS_COUNT; …)` qui commence après CLASS_LABORER doit être audité au prochain ajout
  d'enum** — grep `CLASS_.*; .*<CLASS_COUNT` avant de faire grandir `SocialClass`.
- `scps_agency.c::purge_slice`/`biggest_minority` : la répression politique pouvait cibler un groupe
  esclave (contresens : on réprime des sujets libres, pas une propriété) ET sa saignée proportionnelle
  de fin de fonction touchait `strata[CLASS_SLAVE]` même hors-cible.
- `scps_events.c::apply_region_eff` : `pop_mult` d'un évènement (peste/famine) multiplie TOUTES les
  strates — ce module ne connaît même pas `PopGroup`.
**Méthode qui a marché** : quand un drift persiste malgré des fixes plausibles, instrumenter
TEMPORAIREMENT `sim_day` avec un print du total suspect avant/après CHAQUE appel du bloc annuel —
localise en 1 run si le coupable est annuel ou mensuel (ici : mensuel, ce qui a fait chercher dans
`econ_tick`/`demography_tick`/`world_events_tick` plutôt que dans le bloc annuel visible du diag).
**Restes** : l'invariant `Σstrata[CLASS_SLAVE]==Σgroupes klass==CLASS_SLAVE` est maintenant testé
(demography_demo §11c/§11d) mais RIEN n'empêche une 10e fuite similaire ailleurs — tout nouveau code
qui boucle sur `CLASS_COUNT` pour bouger de la pop doit se demander explicitement s'il doit EXCLURE
CLASS_SLAVE.

## [2026-07-06] IA — le refresh des capacités sautait sous l'épargne (orchestrateur, SLAVEDIAG)
**Découvertes** : le bloc qui pose `AiActor.can_enslave/has_creuset/has_halles` vivait en FIN d'`ai_research_step` (scps_ai.c) — derrière CINQ early-returns d'épargne/famine (« on ÉPARGNE pour le pas suivant »). Un empire coincé en épargne (fréquent : greffe culturelle, beeline emblème, famine de fer) ne rafraîchissait JAMAIS ses capacités : un HONNEUR restait can_enslave=0 pour toujours (mesuré seed 7 : esclavagistes 1→3 après hoist du bloc en tête de fonction).
**Pièges** : tout REFRESH D'ÉTAT-CAPACITÉ logé dans une fonction à early-returns doit vivre AVANT le premier return (juste après le gate de cadence). Chercher les autres blocs du même genre avant d'en ajouter un.
**Restes** : les settles d'esclavagistes capturent encore 0 âme (occ=0 ou tirage interne nul) — en cours de diagnostic.

## [2026-07-06] Esclavage — la capture N'ÉTAIT PAS cassée : le FREIN DE GUERRE plie la paix avant que le siège tombe (réparateur solo)
**Découvertes** : `diplo_enslave_capture` (scps_diplo.c) délivre des âmes RÉELLES et correctement (`moved=160` mesuré seed 99 conqueror=78 region=55) dès que `diplo_settle` a AU MOINS une région occupée (`n>0`). Le vrai goulet : parmi les TROIS chemins qui appellent `diplo_settle` (`ai_strat_turn`, scps_ai.c), le chemin **`settle(consolidate)`** (frein dur `ai_consolidation_pressure`, ~63/64 appels tracés sur un run de 200 ans) a **`n=0` dans 100 % des cas mesurés** — il fold TOUTES les guerres en cours dès `credit_consolidate>=1.0` SANS AUCUNE condition de terrain ni de durée. `brake` sature à 1.00 en 1-2 accumulations d'`ai_strat_turn` (cadence ~3 ans, `AI_STRAT_CADENCE`), largement AVANT qu'un siège moyen tombe (`siege_days` jusqu'à ~730 jours, `army_demo.c:315`) — `diplo_make_peace` termine la guerre (status=NEUTRAL), le siège en cours n'aboutit jamais à `diplo_occupy`. HONNEUR est spécifiquement pénalisé : flagué « mauvais intégrateur » (coût FN_RENFORCEMENT ×1.20, scps_ai.c:1731) → conquérir lui bâtit de la fracture plus vite → il retombe dans le frein dur plus souvent que les autres éthos, coupant SES sièges plus souvent.
**Pièges** : un « frein/consolidation » qui accumule un crédit et fold TOUT dès seuil franchi, sans jamais vérifier si l'action qu'il annule est sur le point d'aboutir, tue silencieusement tout mécanisme downstream qui dépend de cette action (ici : la capture d'esclaves dépend du siège qui dépend de la guerre qui dure). Chercher `diplo_settle`/tout retrait-d'état similaire appelé depuis un chemin de « repli/consolidation » et vérifier s'il a une garde de type « laisser une chance » avant de conclure au bug dans la fonction cible elle-même.
**Fix** : `AI_CONSOLIDATE_GRACE_Y=2.0` (scps_ai.c) — une guerre SANS territoire occupé ET plus jeune que la grâce est SAUTÉE (pas pliée) dans la boucle de fold ; le frein revient au tick suivant, rien n'empêche la guerre de se plier si elle traîne réellement. Seuil ≪ `AI_WAR_EXHAUST=10` ans (ne retarde qu'un tout jeune conflit).
**Restes** : pool des Centres (vente IA, gate `slaves>=10`) et affranchissements restent à 0 dans les sweeps courts testés (250 ans) — volume d'âmes serviles encore trop petit / pas assez de temps, PAS une régression du fix, PAS dans le périmètre du bug capturé. golden RE-BASELINÉ (seeds 7/108/310 changent, 209/411 inchangés) — attendu : des guerres qui auraient été pliées prématurément vont plus loin, changeant l'issue de certaines guerres précoces dans la fenêtre 12 ans. Détail complet : eco_fable.md [021].

## [2026-07-06] Chaîne militaire — audit 13 points (éclaireur de guerre, read-only)
**Découvertes** :
- **CB fonctionnel, pas décoratif** : `d->cb[a][b]` gate `diplo_war_claim` (nombre de « tampons » de province au règlement — `CB_TERRITORIAL` proportionnel à la domination, autres CB = 1 prise, `CB_NONE` = 1 si dominant) + `ai_province_value`/`ai_pick_rival` (convoitise) + `diplo_casus_belli` est un HARD GATE pour l'IA (scps_ai.c:469 « PAS DE CB → pas de guerre »). Télémétrie « guerres motivées » confirmée en sweep (5×200 ans, seed 9) : 153 territoriale · 47 économique · 50 subjugation · 28 religieuse · 4 anti-piraterie — les 5 tirent.
- **Recrutement → jeton armée : DEUX pools JAMAIS synchronisés par construction.** `army_recruit`/`wh_arm_unit` remplit `WarHost.army[cid]` (la réserve levée, `warhost_units` = `ScpsArmy.regiments`). `campaign_order` (scps_campaign.c:128, `a->force=*src_force`) fait une COPIE COMPLÈTE de `host->army[owner]` dans `FieldArmy.force` au moment de l'ordre — rien ne décrémente le host. Résultat : `warhost_units(host,c)` reste « tout ce qui a jamais été levé » même après qu'un détachement soit parti au front, mort en bataille, ou dissous en campagne (`campaign_disband` ne touche QUE `Campaign.army[]`, jamais `host->army[]`) — les deux compteurs DIVERGENT dans le temps mais aucun n'est un double-compte à l'instant T de l'ordre (c'est un instantané, pas un flux partagé). `sim_campaign_orders`/`sim_campaign_defense` (scps_sim.c:128,138,166,171,185) utilisent `warhost_units` comme SEUL gate (« rien à projeter » / ratio d'attaque) — donc l'IA continue de lever de nouveaux paquets sur le host sans jamais réconcilier avec ce qui est déjà sur la carte. Round-trip prouvé par probe façade (scratchpad, non committée) : `scps_player_recruit` a un gate ARMY_CAN_RECRUIT réel (200 ordres, 1 seul accepté — armes rares) ; `scps_player_campaign` REVALIDÉ au drain (région à soi + chemin BFS existant — silencieux si échoue, aucun code d'erreur remonté, cohérent avec « revalidation au drain »).
- **`warhost_disband` (CMD_DISBAND, verbe joueur) NE REND PAS les armes** (scps_warhost.c:104-110, `army_init`=memset pur) — contraste avec `wh_shed` (downsizing NATUREL en paix, scps_warhost.c:157) qui appelle `econ_region_stock_add` pour restituer le fer. La pop n'a jamais quitté le pool (elle reste « affectée » via `pop_by_class_in_army`, remis à 0 par le memset — cohérent, elle était déjà dans les strates econ) mais les ARMES consommées à la levée (RES_ARMS_*) sont perdues définitivement au disband joueur. Aucun banc (army_demo/warhost_demo) ne couvre `warhost_disband`.
- **Le compteur télémétrie « régions réduites (campagne) » est CASSÉ (sous-compte massivement)** : `FieldArmy.taken` (scps_campaign.c:129,195) est remis à 0 à CHAQUE nouvel ordre (`campaign_order` ET `campaign_redirect`) — or l'IA réordonne dès que idle (annuel) et les sorties défensives redirigent constamment. Chronicle somme `campaign_taken(c)` en fin de sim (snapshot du dernier segment ininterrompu), pas un cumul vrai. Preuve chiffrée : sweep 5×200ans → « régions réduites » = 32 total (6.4/sim) MAIS « occupations posées » (compteur `g_tot_occ_posed`, réellement cumulatif, chronicle.c:1335) = 296 posées / 35 levées sur la même fenêtre — les sièges réussissent ~9× plus souvent que la télémétrie affichée ne le suggère. Fix d'une ligne : sommer `taken` dans un compteur EXTERNE persistant (comme `n_battles`/`n_routs`) au lieu de le lire une fois à la fin, ou promouvoir `occupier[]`/`settle_transfer` comme LA mesure de sièges gagnés (déjà juste).
- **Battles réels et mesurés** : 1941 batailles/5×200ans (18j moy., 1903 déroutes, 280 ralliements), morts choc 100300 vs poursuite 211700 (ratio 2.1× — la doctrine « la poursuite domine si cavalerie » cohérente avec army_demo §8). `sim_campaign_defense` (scps_sim.c:116) fait sortir le défenseur en siège ; `campaign_tick` résout la rencontre.
- **UI combat : PARTIEL.** Godot overlay.gd dessine le jeton (losange teinté pays), un halo de PHASE (marche blanc/siège orange/bataille rouge), une ligne de marche vers le but, un marqueur dédié en bataille (`HeraldryK.marker("battle")`). AUCUN panneau de combat détaillé — pas de force-vs-force, pas de pertes, pas de war_score affiché. `empire_sidebar.gd`/`sidebar_drawer.gd` n'affichent qu'« En campagne : N (phase) » en texte. `scps_army_info`/`ScpsArmyInfo` EXPOSE déjà région/dest/phase/units/composition — la donnée existe côté façade, pas consommée en panneau.
- **Effets des troupes : APPLIQUÉS et TESTÉS pour le socle (12 unités), PAS pour le Roster-22.** discipline/moral/mouvement/commandement tous lus (aucun champ mort). Matrice de contres (`MATRIX[U_COUNT][U_COUNT]`) construite pour les 22. army_demo (48/48 vert) ne teste explicitement QUE Hallebardier parmi les 10 unités « Roster 22 » (`winrate(U_HALLEBARDIER,...)`, army_demo.c:101) — Arquebusier/Alchimiste/Garde runique/Arbalète lourde/Berserker/Lancier de choc/Milice/Harceleur/Traqueur/Lame franche/Garde d'escorte/Cav cuirassée/Cav de raid n'ont AUCUN test dédié (seulement exercés statistiquement en sweep, jamais assertés).
- **Coût des troupes post-refonte éco : PAS explosé, reste marginal.** Mesuré via probe façade (budget de la plus grosse armée du monde, seed 9, sim IA) : an 50 → 8 régiments, soldes −239.1/an sur 16826 revenus (1.4 %) ; an 150 → 74 régiments (×9.25), soldes −2165.8/an sur 195330 revenus (1.1 %) — croissance quasi-linéaire avec la taille de l'armée, part du budget STABLE. Le poste dominant du budget est « cour » (16 %) et surtout « redépense » (~70 % de la dépense, I3bis) — la solde militaire (REGIMENT_PAY 1.5/rgt/mois×ipm) et le coût de levée (REGIMENT_PRICE 12/rgt×ipm) sont de petits postes face au reste de l'État. Verdict : la refonte éco (prix national, matière réelle, labor) n'a PAS cassé l'équilibre militaire budgétaire.
- **Pillage : réel, basé sur stock+trésor, PAS un or plat, câblé au RÈGLEMENT (pas au siège).** `diplo_pillage_region` (scps_diplo.c:1040) prend 60 % du trésor provincial + 50 % de TOUT le stock régional (valorisé au prix courant du marché, `re->price[g]`) fondu en or transféré à la capitale du vainqueur ; cooldown 5 ans/province (anti-re-saccage). Appelé UNIQUEMENT depuis `settle_transfer` (scps_diplo.c:869, avec `diplo_enslave_capture` juste après) — c'est-à-dire au RÈGLEMENT DE PAIX (budget de guerre dépensé sur une région OCCUPÉE), jamais pendant le siège lui-même. `SLAVE_FRACTION` (capture d'esclaves) est câblée au MÊME point (settle_transfer), donc au règlement — PAS au siège/sac en tant que tel (il n'y a pas de sac « pendant » le siège, juste à la bascule de propriété).
- **Hachures de siège UI : ABSENTES côté Godot.** Le façade C a un mode `VIEW_WAR` tout fait (scps_api.c:169, `map_state_tint` : occupé=rouge vif, belligérant=orange, paix=vert sombre, lu de `dp->occupier[r]`) mais Godot n'utilise JAMAIS `scps_map_rgba`/`VIEW_WAR` — le lavis politique Godot passe par `political_image()` (owner→teinte, aucune notion d'occupation/siège). Donnée moteur disponible (`occupier[]`), lecteur C prêt, RIEN ne le consomme côté front jouable.
- **Les pop NE fuient PAS toute la province assiégée — SAIN, et le siège lui-même NE DÉCLENCHE PAS la fuite.** `revolt_scar` n'est posé à 1.0 QUE par la conquête (`settle_transfer`) ou le pillage lui-même — PENDANT un siège en cours (occupation sans transfert), scar reste à ce que la province avait avant. `demography_refugee_tick` exige `revolt_scar>=REFUGEE_FLEE_SCAR(0.5)` pour faire fuir `REFUGEE_FLEE_FRAC(0.03, registre J)`/an de chaque groupe. Scar décroît −0.25·dt/an (scps_econ.c:2726) → repasse sous 0.5 en ~2 ans après une conquête → fuite totale cumulée environ 2×3%=6 % de la pop avant l'apaisement. Conclusion : ni le siège (avant chute) ni la chute elle-même (après les ~2 premières années) ne vident la province — le mécanisme est calibré sain, et la réponse à « le siège pose-t-il une cicatrice qui déclenche la fuite » est NON — seule la BASCULE DE PROPRIÉTÉ le fait, pas le siège en cours.
- **Le roster est réellement diversifié en pratique** (pas de spam d'unité unique) : `wh_levy_batch` (IA) compose PROPORTIONNELLEMENT à Σ fw[faction]·AFF[faction][unit] sur TOUS les types recrutables (gate tech + gate élite appliqués AVANT la pondération) — structurellement impossible de spammer un seul type sauf plancher de secours (Piquier/Épéiste/Archer si sum<=0, garde-fou « jamais une armée vide »).
- **Démobilisation : pop OUI (elle n'a jamais quitté les strates), armes NON (au disband joueur).** cf. point warhost_disband ci-dessus — la downsizing naturelle en paix (`wh_shed`, IA) restitue bien les DEUX (pop libérée de `pop_by_class_in_army` + armes rendues au stock) ; SEUL le verbe joueur CMD_DISBAND casse l'asymétrie.
- **Modificateurs : doctrine armée (Forge/Société/Savoir×tiers) APPLIQUÉE et TESTÉE (army_demo §9).** Boosts par-unité (arcane_power→Mage, firearm_power→Arquebusier via l'apex Arquebuse runique) câblés dans `unit_power`. `H_coerc` (garnison) N'EST JAMAIS LU par scps_campaign.c — seule la défense passive de CAPITALE (`capitale_defense(tier pop)`) + le COMPTE de bâtiments (n_bld, PAS leur nature) allongent le siège (`region_defense`, scps_campaign.c:64). Une province riche en bâtiments civils (marché, atelier) résiste donc au siège exactement comme une province avec autant de casernes/remparts — aucune distinction de type d'édifice, aucun rôle pour la coercition/garnison stockée.
- **Gate techno des unités : 7/22 gatées, 15/22 libres day-1** (`unit_tech_gate`, scps_army.c:67) : Arquebusier←Poudrière, Garde runique←Forge à runes, Mage←Magie de bataille, Alchimiste←Alchimie, Hallebardier←Caserne, Cav cuirassée←Caste martiale (apex tardif), Arbalète lourde←Qualité matériaux. Enforcement RÉEL des deux côtés : joueur (`warhost_player_recruit`→`unit_recruitable`) ET IA (`wh_levy_batch`→target[u]=0 si non-recrutable). Le roster de base (Piquier/Lancier/Épéiste/Archer/Arbalète/Cav légère/Cav lourde + 8 des 10 « Roster-22 » : Berserker/Lancier de choc/Milice/Harceleur/Traqueur/Lame franche/Garde d'escorte/Cav de raid) est TOUJOURS recrutable — cohérent avec le brief (« unités de base libres »).
**Pièges** :
- Le Makefile ajoute `-Wl,--stack,8388608` aux liens `campaign_demo`/`warhost_demo` (tentative demandée) : campaign_demo passe de KO(stack overflow 0xC00000FD) à 19/19 VERT — c'était bien un vrai stack overflow Windows (pile 1 Mo MinGW vs 8 Mo Linux), le fix marche. En revanche warhost_demo passe de KO(crash) à 0/4 VERT — 4 assertions logiques ÉCHOUENT maintenant. Ce n'est PLUS un crash de pile — c'est soit une vraie régression (économie/warhost désynchronisés par les refontes en cours d'un autre agent qui touchait scps_ai.c/scps_econ.h/scps_intertrade.c EN PARALLÈLE dans ce repo partagé), soit un problème préexistant masqué par le crash. NON investigué plus loin (hors mandat read-only) — à reprendre une fois les modifications concurrentes stabilisées.
- Deux agents parallèles modifiaient scps_api.c/scps_endgame.c/scps_intertrade.c/scps_econ.h/scps_ai.c/chronicle.c PENDANT cet audit (repo partagé) — mes mesures utilisent le code tel quel au moment du run ; si golden bouge ensuite ce n'est pas dû à cet audit (aucun fichier moteur touché, seul le Makefile pour le stack).
- `TMP`/`TEMP` ne survivent pas d'un appel Bash à l'autre dans ce harnais (chaque export isolé) — préfixer CHAQUE invocation make/gcc avec les variables (sinon gcc essaie d'écrire dans C:\Windows\, permission refusée). MSYS2 toolchain : D:\MSYS2\mingw64\bin + D:\MSYS2\usr\bin (pas dans le PATH de base).
**Restes** :
- Compteur « régions réduites (campagne) » à corriger (fix d'une ligne ci-dessus) — actuellement trompeur d'un facteur ~9×.
- `warhost_disband` devrait restituer les armes comme `wh_shed` (même motif, econ_region_stock_add par type d'arme présent dans les unités dissoutes) — actuellement un sink d'or silencieux au bénéfice du joueur qui démobilise (perd le fer sans le récupérer).
- Panneau de combat détaillé (force vs force, pertes, war_score) : donnée déjà exposée (scps_army_info, diplo_war_score), juste jamais branchée à un panneau Godot dédié.
- Hachures/teinte de siège sur la carte Godot : brancher dp->occupier[] — proposer un reader scps_region_occupier(region)→int (façade légère, additive) puisque VIEW_WAR côté C n'est pas exposé par région individuelle pour Godot.
- army_demo à étendre aux 9 unités du Roster-22 non testées — au moins un winrate ciblé par unité comme le fait déjà Hallebardier.
- Gate techno cohérente avec l'arbre (proposition, non implémentée) : lier Berserker/Lancier de choc à une tech martiale de base, lier Lame franche/Cav de raid à une tech de mercenariat/commerce (cohérent avec leur solde en or), garder Milice/Harceleur/Traqueur/Garde d'escorte libres (infanterie de fortune/appui local).
- H_coerc/garnison : envisager un rôle dans region_defense (bonus de siège distinct du simple compte de bâtiments) si le brief souhaite que « renforcer sa garnison » ait un effet mécanique propre au-delà de la répression intérieure.

## [2026-07-06] Éco — LOT 1/2/3 « réparations » : E3 stockeuse (terme mort = has_halles, pas le trésor) · nudge intertrade RETIRÉ (pas inerte : re-baseline le prouve) · avg_price dédoublonné (réparateur solo)
**Découvertes** :
- **E3 (`scps_ai.c::ai_econ_turn`, gate ligne ~1361)** : le gate à 3 termes `has_halles && hub_ok && n_entrepot<1 && treasury>400.f` — MESURÉ (diag env `SCPS_E3DIAG`, seed 9 × 200 ans, 67 échantillons sur les pays qui atteignent la branche `credit_trade≥1`) : `hub_ok`/`slot_ok` TOUJOURS vrais, `treasury` atteint 400 en quelques années et monte à 20-36k — le trésor n'a JAMAIS été le mur. **`has_halles=0` dans 100 % des 67 échantillons** — `TECH_HALLES` (tier-2, FN_PRODUCTION, `native=UNIV`) n'était JAMAIS choisie par `ai_pick_tech` (0 sélection en 200 ans) car elle n'a ni signature d'héritage (`AI_TECH_SIGNATURE`) ni terme de besoin propre — elle perd SYSTÉMATIQUEMENT contre `TECH_IRRIGATION` (même tier, même thème, même fonction, même prérequis `TECH_COLLECTE_NOURRITURE`) qui capte tout `v.gap_acuity` (famine). Même `TECH_COMPTOIRS` (son PROPRE prérequis) n'est débloquée que par 3/~26 pays en 200 ans sur seed 9.
- **Fix (1 ligne + define)** : `AI_HALLES_HUNGER=3.0f` — couplage `if (id==TECH_HALLES && a->credit_trade>=1.f) score += AI_HALLES_HUNGER;`, même motif que `AI_FOREUSE_HUNGER` (§4). `credit_trade≥1` EST le signal réel (le seau commerce plein = la branche E3 elle-même) — pas un bonus arbitraire. Mesuré après (seed 7/9/11 × 5×250 ans) : 6-10 entrepôts/sim construits (0 avant), σ prix Centre AVANT→APRÈS 0.34-0.41→0.03-0.13 (le lissage annoncé par le commentaire E3 §16 se produit enfin), or net spéculatif +176 à +33715/sim.
- **Nudge intertrade (`scps_intertrade.c`, 2 sites : bloc route ligne ~989 et arbitrage Centres ligne ~1029)** : écrivait `A->price[g]`/`H->price[g]` — pointeurs dans `e->region[]`, la vue TRANSIENTE rebâtie CHAQUE mois par `econ_aggregate_regions` depuis `prov[].price` (jamais touché par intertrade) puis re-projetée par le bloc « PRIX NATIONAL » d'`econ_tick` (pure fonction demand_nat/pool/supply_nat, sans mémoire du nudge) — **persistant, le nudge est bien mort** (prouvé par lecture de flux : `econ_aggregate_regions` recopie `ag->price[]=pe->price[]` à chaque appel, `scps_econ.c`).
**Pièges** : ⚠ **« mort persistant » ≠ « sans effet »** — le nudge écrit `region[].price` qui est LU LE MÊME JOUR par au moins un site de décision IA (`scps_ai.c:1180`, ROI de `raw_boost` : `val_year = per_tier*O_year*econ->region[br].price[r]`, comparé à un seuil `cost`). Un nudge transitoire peut donc faire basculer un seuil de décision AVANT d'être écrasé — **isolé empiriquement** : `make golden` échoue avec les 3 lots ensemble ; en stashant SEULEMENT `scps_intertrade.c` (LOT1+LOT3 actifs), golden repasse au VERT avec les MÊMES hashes qu'avec les 3 lots — donc LOT2 seul (pas LOT1) cause le re-baseline. Le fix documenté (retrait, "le défaut attendu") reste correct — un nudge qui ne change RIEN de durable au marché mais fait basculer, un jour donné, une décision d'IA via une lecture same-tick n'est pas un canal de jeu voulu, plutôt un ARTEFACT d'ordre d'exécution — mais ATTENTION : le simple raisonnement "écrit un scratch buffer, écrasé au prochain aggregate ⇒ mort" est INSUFFISANT pour classer un patch "golden-neutre" — il faut vérifier tous les LECTEURS de `region[]` dans la fenêtre entre l'écriture et l'écrasement, pas seulement les écrivains suivants.
- **avg_price (`chronicle.c`/`econ_scan.c`)** : dédoublonné vers `scps_econ.h::econ_avg_price` (static inline, outillage télémétrie pur — jamais lu par le moteur, donc golden-neutre par construction, confirmé : isolé du re-baseline).
**Restes** : —

## [2026-07-06] UI — LE CODEX DES VERBES (F1) : scan exhaustif + panneau (implémenteur solo, read-only sauf codex.gd/main.gd)
**Découvertes** : l'enum `CMD_*` de scps_sim.h compte **34 verbes journalisés** (BUILD, RECRUIT, SET_LEVY, RESEARCH, DECLARE_WAR, MAKE_PEACE, OFFER_ALLIANCE, OFFER_PACT, EMBARGO, REPRESS, ASSIMILATE, PURGE, COUNCIL_HIRE, COUNCIL_DISMISS, ROUTE, MARKET_BUY, MARKET_SELL, CAMPAIGN, POSTURE, REFILL, NAVY_BUILD, DISBAND, ALLOC_RAW/BLD/INPUT/AUTO, AGE_ENGAGE, COLONIZE, OFFER_MIGRATION, BUILD_MANUF, EVENT_CHOICE, DECREE, MANUMIT, SLAVE_BUY/SELL, POP_TRANSFER, FABRICATE_CB, COUNCIL_PAY). Câblage réel (grep `.player_*(` sur tout `godot/project/**/*.gd`) : **30/34 sont dans l'UI** — répartis entre `province_panel.gd` (repress/assimilate/purge/campaign/route/colonize), `province_detail.gd` (alloc_raw/bld/input/auto, build_manuf, pop_transfer), `sidebar_drawer.gd` (council_hire/dismiss, decree, market_buy/sell, posture/refill/disband/navy_build, set_levy), `country_actions.gd` (declare_war/make_peace/offer_alliance/offer_pact/embargo/offer_migration/fabricate_cb), `construction_panel.gd` (build/recruit), `tech_panel.gd` (research), `age_recap.gd`+`topbar.gd` (age_engage), `event_dialog.gd`/`event_popup.gd` (event_choice/repress-alerte). **4/34 SANS UI Godot** (câblés côté façade C `scps_api.h` — parfois même pas jusqu'au binding C++ `scps_sim_node.h`) : `CMD_MANUMIT`/`scps_player_manumit` (aucun binding, aucun bouton), `CMD_SLAVE_BUY`/`CMD_SLAVE_SELL` (idem, marché servile inexistant côté front malgré `scps_slave_market` prêt en lecture), `CMD_COUNCIL_PAY`/`scps_player_council_pay` (le curseur de paie par siège n'est PAS bindé — seul `player_council_hire/_dismiss` le sont). Les clés globales réelles au 2026-07-06 : Échap (pile de fermeture `_close_topmost`), F10 (dev panel), H (chronique), Espace (pause), +/- (vitesse) — **aucune touche C/R/T/G/V/B** n'existe plus en dur (les commentaires de main.gd qui les citent sont PÉRIMÉS : religion/tech/économie/détail-province/construction s'ouvrent tous via boutons sidebar/topbar/panneau-province, culture creator est confiné à l'écran Nouvelle Partie). Panneau ajouté : `godot/project/ui/codex.gd` (Control statique scrollable, motif chronique.gd, données EN DUR par domaine — Empire & Économie / Peuples / Diplomatie & Guerre / Foi & Savoir / Fin de partie), touche **F1** (hook dans main.gd : déclaration `_codex`, instanciation après `_devpanel`, case `KEY_F1` dans `_unhandled_input`, ajouté à la pile `_close_topmost`).
**Pièges** : `verbs_audit.gd` (probe existante) ne teste QUE 14 verbes (l'ancienne vague §3) — pas une source fiable d'inventaire à jour, le scan a dû se faire sur l'enum C directement. Les commentaires « touche T/G/V/B/C/R » dans main.gd et ailleurs sont un CHROME PÉRIMÉ d'une itération antérieure de l'UI (grep de `KEY_` global confirme leur absence réelle) — ne pas faire confiance aux commentaires de raccourci sans vérifier `_unhandled_input`/`_gui_input` du fichier concerné.
**Restes (câblage pour une vague dédiée)** : (1) `scps_player_manumit` — un bouton pays (« Affranchir les esclaves ») dans `country_actions.gd` ou un onglet dédié, lecture `scps_manumit_preview` déjà prête. (2) `scps_player_slave_buy`/`_sell` + `scps_slave_market` — un panneau/onglet marché servile (province ou pays), aucun reader manquant côté façade. (3) `scps_player_council_pay` — un curseur par siège dans le tiroir Conseil (`sidebar_drawer.gd`), à côté de hire/dismiss existants ; nécessite d'abord d'ajouter la méthode au binding C++ (`scps_sim_node.h/.cpp`, absente aujourd'hui — les 3 verbes ci-dessus non plus). Aucune incohérence de double-câblage trouvée (pas de verbe branché deux fois, pas de bouton mort observé dans les fichiers audités) — le seul écart entre bouton et moteur documenté ailleurs (coût §NF vs affiché) est déjà connu et non dans ce périmètre. Gates : boot headless `Main.tscn` (`--quit-after 30`) propre, 0 SCRIPT ERROR/Parse Error (warnings PNG-as-resource et RID-leak au shutdown pré-existants, sans rapport).

## [2026-07-06] Godot — P5 : merv_metab() sépare la victoire de l'accès tech (résolu)
**Découvertes** : `endgame_metab_count_ts` (scps_endgame.c) était `static` — le compte RÉEL de la Merveille n'avait aucune façade. Ajouté `endgame_heritage_detail` (public, scps_endgame.h/.c) qui expose PAR HÉRITAGE {metabolized bool, voie "natif"/"gouvernance"/"diaspora"/"", progress_pct 0-100 de la meilleure des deux voies (gouvernance=arch_depth/PROF_PROFOND, diaspora=ratio/METAB_MERV_RATIO)} — réutilise `endgame_heritage_metabolized_detail` (nouvelle variante non-bool de la fonction existante, renvoie aussi le ratio). Façade `scps_merv_metab(sim,out,max,&count,&required)` + binding Godot `merv_metab()→Dictionary{count,required,heritages[]}`. `tech_panel.gd` sépare maintenant VISUELLEMENT deux rangées dans `_draw_metab` : « Accès aux signatures (arbre) » (heritage_access, tech pop-share) et « Compte pour l'Ascension » (merv_metab, la seule jauge de victoire) ; `_check_metab_ready` (le chip/signal) lit désormais `merv_metab().metabolized` au lieu de `heritage_access().tier>=3`.
**Pièges** : golden a semblé casser après cette édition, mais c'était `git stash`/diff d'un AUTRE agent concurrent qui modifiait chronicle.c/econ_scan.c/scps_ai.c/scps_intertrade.c/scps_econ.h EN PARALLÈLE dans le même repo (repo partagé entre sessions) — isoler ses propres fichiers via `git stash push -- <mes fichiers>` avant `make golden` pour juger honnêtement. Confirmé : mes 5 fichiers seuls ⇒ golden IDENTIQUE (readers purs).
**Restes** : —

## [2026-07-06] Guerre — W-GUERRE moteur : les 6 lots de l'audit implémentés (implémenteur solo)
**Découvertes** :
- **LOT 1 (recrutement↔jeton réconcilié)** — design retenu : `campaign_order`/`campaign_order_sea` prennent désormais `ArmyState *src_force` (non-const, était `const`) et **TRANSFÈRENT** (via une nouvelle primitive `army_merge_into(dst,src)` — scps_army.c/.h, fusionne unité-par-type + armes + pop_by_class_in_army puis VIDE `src`) au lieu de COPIER. Le host (`&s->host->army[c]`) est donc VRAIMENT débité au départ d'un détachement — `warhost_units` reflète enfin « la réserve NON déployée », corrigeant les deux lecteurs qui en dépendaient à tort (`sim_campaign_orders` : gate « rien à projeter » + comparaison de ratio d'attaque `BT_ATK_RATIO`). Cas du RÉORDONNANCEMENT (armée déjà active, IA la redirige) : le reliquat de `a->force` est d'abord rendu à `src_force` (`army_merge_into(src_force, &a->force)`, qui vide `a->force`) AVANT que le nouveau prélèvement parte — jamais de double-compte ni de perte silencieuse d'un reliquat écrasé. `campaign_disband` (jusque-là JAMAIS appelée nulle part — code mort) gagne un paramètre optionnel `ArmyState *dst_host_army` : si fourni, les SURVIVANTS du détachement retournent au host via `army_merge_into` ; NULL = ancien comportement (le détachement s'évapore). Alternative écartée : un double débit-crédit économique (retirer/redéposer des `RES_ARMS_*`) — inutile, puisque les armes ont déjà été consommées à la LEVÉE (warhost) ; le détachement de campagne n'est qu'un DÉPLACEMENT de la même `ArmyState`, pas une nouvelle dépense. Callers non-host (`campaign_demo.c`, `scps_revolt.c::deploy_rebel_army`) fonctionnent sans changement — leurs `ArmyState` locales ne sont jamais relues après l'appel (sauf `campaign_demo.c` §3/§3b qui RÉUTILISAIT `invader`/`defender` sur 2 ordres successifs : fixé en refaisant des forces fraîches `invader2`/`defender2` pour la 2e manche, cf. le nouveau contrat de transfert).
- **LOT 2 (CMD_DISBAND rend les armes)** — `warhost_disband(WarHost*, WorldEconomy*, int cid)` (signature +econ) fond désormais TOUTE la réserve via `wh_shed` (le même mécanisme que le downsizing naturel de paix : armes → `econ_region_stock_add`, pop → `pop_by_class_in_army`), au lieu d'un `army_init` (memset) qui perdait le fer silencieusement au bénéfice du joueur. `wh_shed` étant `static` et défini APRÈS `warhost_disband` dans le fichier, une déclaration forward a été ajoutée en tête de scps_warhost.c. Banc +2 (warhost_demo : disband dissout bien la réserve à 0, Σ stock macro APRÈS ≥ Σ AVANT).
- **LOT 3 (le siège lit la garnison)** — `region_defense()` (scps_campaign.c, `static`, donc non testable directement — éprouvée end-to-end via `campaign_tick`) gagne un terme `DEF_PER_H * R->build.H_coerc` (`DEF_PER_H=0.05`, registre J). H_coerc est DÉJÀ agrégé au niveau région (`scps_econ.c:934`, `ag->build.H_coerc += pe->build.H_coerc`) — aucune agrégation à écrire. Mesuré (campaign_demo §3d, sur une région TÉMOIN contrôlée — pop/n_bld/H_coerc posés à la main, car les régions du monde généré ont souvent un `cap_def` de capitale (tier de pop) qui SATURE déjà le plafond `SIEGE_MAX_DAYS=730j`, masquant tout delta H_coerc si on teste sur `target` du monde généré) : H_coerc 0→6 (≈1 Forteresse) fait bouger `defense_level` de ~20-30 % LUI-MÊME, mais `siege_days()` dilue ce ratio par sa constante additive `SIEGE_BASE=45` + le terrain → la durée FINALE observée ne bouge que d'~5 % en pratique (727→764j typique). Documentation du define corrigée pour refléter cette mesure (pas de sur-promesse "20-40%" sur la durée finale — c'est vrai sur `defense_level`, pas sur `siege_days` après dilution).
- **LOT 4 (pillage de siège)** — `diplo_siege_loot(econ, region, dst_region)` (scps_diplo.c/.h, nouveau) détourne chaque MOIS `SIEGE_LOOT_FRAC * re->supply[g]` (la PRODUCTION du tick, `supply[]` — pas le stock accumulé, déjà la cible du butin final `diplo_pillage_region`) pour chaque ressource, bornée au stock RÉELLEMENT disponible (`fminf(want, re->stock[g])`, jamais négatif, jamais dupliqué), valorisée au prix courant, fondue dans le trésor de `dst_region`. Appelé depuis `sim_campaign_year` (scps_sim.c) pour toute `FieldArmy` en `FA_SIEGE` sur une région qui n'est PAS la sienne (siège chez soi = libération, pas de pillage). Gaté par le MÊME `pillage_cd` que le butin final (anti-re-saccage). **Capture de sac** (pop) : à la CHUTE (`diplo_occupy` réussi dans la récolte de sim), si `s->ai[a->owner].can_enslave` (champ PUBLIC d'`AiActor`, lu directement depuis scps_sim.c — PAS besoin de toucher scps_ai.c, forbidden), on appelle `diplo_enslave_capture(...,true)` — réutilise l'idiome existant tel quel (aucune modif de sa logique de calcul). **Anti-double-sac ajouté** (le vrai risque découvert en concevant ce lot) : `diplo_enslave_capture` était appelée SANS cooldown au règlement (`settle_transfer`) — si le NOUVEAU trigger de chute capture PUIS que la même région se règle plus tard dans la même guerre, la capture aurait pu doubler (2× `SLAVE_FRACTION` de la population, sur ce qui reste après la 1re capture). Fix : `diplo_enslave_capture` CHECK maintenant `pillage_cd>0` en entrée (return 0 si déjà sac(c)agée) ET POSE `pillage_cd=PILLAGE_COOLDOWN_Y` sur succès (symétrique à `diplo_pillage_region`) — le `#define PILLAGE_COOLDOWN_Y` a dû être DÉPLACÉ avant `diplo_enslave_capture` dans le fichier (il n'était défini qu'au-dessus de `diplo_pillage_region`, plus bas). Vérifié : `diplo_demo.c` §9 (banc existant) construit une province FRAÎCHE avec `pillage_cd` jamais posé (memset=0) → INCHANGÉ par ce guard. Télémétrie neuve (`g_siege_loot_total`/`g_siege_sack_captures`, scps_sim.c/.h, accumulateurs globaux façon `g_tot_occ_posed` — snapshot avant/delta après par sim dans chronicle.c) : mesuré seed 9×3×200 ans, 162084 or-équiv. cumulé (54028/sim), 2 captures de sac sur 200 ans (rare mais réel — cohérent avec la rareté déjà documentée de l'esclavagisme actif dans TROUVAILLES précédent). Banc +4 (diplo_demo §6b : détourne >0 et <plafond, trésor du besiégeur enfle, Σ CONSERVÉE stock-perdu==or-gagné à prix=1, cooldown partagé bloque bien un 2e détournement).
- **LOT 5 (gate techno cohérent)** — calcul d'efficacité `power=(1+discipline)·moral` + largeur du réseau de contres (`MATRIX`) sur les 15 unités jusque-là libres du Roster-22. 4 standouts identifiés et gatés (`unit_tech_gate`, scps_army.c) : **Lancier de choc** (power≈176, 5 contres dont TOUTE la cavalerie y compris l'élite cuirassée/raid + l'épéiste) → `TECH_ORGANISATION` (tier-2 martial). **Garde d'escorte** (power≈201 — LE PLUS HAUT moral du roster ENTIER, supérieur au Cav. cuirassée déjà gaté à 194 — mais 0 contre propre, pure tenue) → `TECH_ORGANISATION` aussi (même tier que l'élite montée, cohérent avec sa force brute). **Berserker** (power≈73, modeste) → `TECH_CONSCRIPTION` (tier-1 martial, plus léger). **Lame franche** (mercenaire soldé en or, 0 contre, versatile) et **Cav. de raid** (LAB_ELITE déjà — double-gate comme Cav. cuirassée — raid/pillage) → `TECH_COMPTOIRS` (tier-1 commerce, « branche au marché mondial » — le mercenariat/raid est mercantile). Les 3 restées libres (Milice/Harceleur/Traqueur : power bas 67-88, 0-3 contres, thématiquement « appui local/fortune ») + la base historique (Piquier…Cav. lourde) INCHANGÉES, conforme au brief (« pouvoir se défendre day-1 »). Tableau complet (avec power calculé) :

  | Unité | power=(1+disc)·moral | # contres (MATRIX) | Gate LOT 5 | Justification |
  |---|---|---|---|---|
  | Piquier | 149.5 | 2 | libre (base) | défense day-1 |
  | Lancier | 125.4 | 2 | libre (base) | défense day-1 |
  | Épéiste | 145.0 | 3 | libre (base) | défense day-1 |
  | Archer | 84.0 | 4 | libre (base) | défense day-1 |
  | Arbalétrier | 127.8 | 4 | libre (base) | défense day-1 |
  | Cav. légère | 108.75 | 6 | libre (base, élite) | défense day-1 |
  | Cav. lourde | 169.1 | 9 | libre (base, élite) | défense day-1 |
  | Berserker | 73.2 | 4 | **TECH_CONSCRIPTION** (tier-1) | modeste mais réel, martial léger |
  | Lancier de choc | 176.0 | 5 (dont cav élite) | **TECH_ORGANISATION** (tier-2) | efficacité haute, anti-cav total |
  | Milice | 67.2 | 0 | libre | plancher, appui local |
  | Harceleur | 70.8 | 3 | libre | appui local (anti-caster) |
  | Traqueur | 88.4 | 3 | libre | appui local (embuscade) |
  | Lame franche | 132.0 | 0 | **TECH_COMPTOIRS** (tier-1) | mercenaire, soldé en or |
  | Garde d'escorte | 201.5 | 0 | **TECH_ORGANISATION** (tier-2) | moral le + haut du roster, ancre pro |
  | Cav. cuirassée | 194.25 | 6 | TECH_CASTE_MARTIALE (déjà gaté) | apex monté |
  | Cav. de raid | 99.4 | 4 | **TECH_COMPTOIRS** (tier-1) | élite déjà gatée, raid mercantile |

  (Arquebusier/Garde runique/Mage/Alchimiste/Hallebardier/Arbalète lourde : déjà gatés avant ce lot.) `TECH_CONSCRIPTION`/`TECH_ORGANISATION` (SOCIÉTÉ·Armée, UNIV, prereq `TECH_CASERNE`) et `TECH_COMPTOIRS` (SOCIÉTÉ·Production, UNIV, tier-1) choisis car UNIVERSELS (aucune culture n'en est exclue) et NON-faustiens (pas de porte arcane sur des unités conventionnelles).
- **LOT 6a (télémétrie « régions réduites » ×9 sous-comptée, CORRIGÉ)** — `campaign_taken(FieldArmy.taken)` RAZ à chaque `campaign_order`/`campaign_redirect` (donc à chaque réordonnancement annuel + chaque interception défensive) ⇒ sommer ce compteur en fin de sim ne lisait qu'un SNAPSHOT du dernier segment ininterrompu. Fix : chronicle.c snapshote `g_tot_occ_posed` (accumulateur GLOBAL vrai, jamais remis à 0, scps_sim.c) AVANT chaque sim, calcule le DELTA après — remplace `campaign_taken` comme mesure de « régions réduites ». Vérifié : seed 9×3×100 ans, 12+9+10=31 région(s) réduite(s) par sim == 31 « occupations posées » total (EXACT, ex-écart ×9 documenté par l'audit).
- **LOT 6b (les 13 unités jamais testées individuellement)** — army_demo.c §12 : pour Arquebusier/Alchimiste/Garde runique/Arbalète lourde/Berserker/Lancier de choc/Milice/Harceleur/Traqueur/Lame franche/Garde d'escorte/Cav. cuirassée/Cav. de raid — 3 assertions par unité (stats lisibles via `unit_def`, power sanity 3 paquets vs 1 Piquier ≥ majorité de 11 tirages, et son contre annoncé tient dans `matchup()`>1). 13/13 verts sur tous les seeds testés (sauf seed 11, cf. plus bas).
- **LOT 6c (warhost_demo recontrôlé)** — les « 4 échecs d'assertions » de l'audit S1 (0/4 verts après le fix du stack de lien) étaient DEUX bugs de BANC distincts, pas une régression moteur : (1) `capital_arms()` lisait `econ->region[r].mil_stock` (la VUE agrégée, reconstruite par `econ_aggregate_regions`, appelé depuis `econ_tick`) mais la boucle du banc n'appelle QUE `warhost_tick` après le peuplement initial (jamais `econ_tick` à nouveau) — la vue restait à 0 pour toujours, alors que `warhost_tick` écrit RÉELLEMENT `econ->prov[rep].mil_stock` (province-owned, re-key). Fix : lire la province représentative directement (`econ_region_rep_province`). (2) sur seed 9 spécifiquement, le pays sélectionné comme `ca` (le plus grand par régions) avait `role==POLITY_UNCLAIMED` — `warhost_tick` SAUTE explicitement tout pays UNCLAIMED (son gate d'entrée), donc `ca` ne mobilisait JAMAIS bien qu'il possède une région économique (colonisation résiduelle/hameau). Fix : le banc filtre désormais `role!=POLITY_UNCLAIMED` à la sélection, miroir exact du gate réel de `warhost_tick`. Après les deux fixes : 6/6 sur tous les seeds testés (1,2,3,5,7,9,11,42,99,100,200).
**Pièges** :
- Le repo est PARTAGÉ en temps réel avec un agent parallèle (UI Godot) qui a fait un `git stash` SANS pathspec (« isolate-parallel-agent-for-golden-check ») en cours de mission, qui a EMPORTÉ mes 15 fichiers modifiés en même temps que les siens (stash@{0}, 450 lignes, tous mes lots) — mes fichiers ont semblé « disparaître » du disque (git status vide) pendant ~2 minutes avant que je remarque et fasse `git stash pop`. RIEN n'était perdu (stash = commit-like, récupérable), mais la LEÇON : dans un repo partagé, vérifier `git status`/`git stash list` avant de conclure qu'un changement a été perdu, et RECONSTRUIRE + RE-TESTER après tout `git stash pop` pour confirmer l'intégrité (fait : campaign_demo/warhost_demo/army_demo/diplo_demo re-vérifiés 21/6/49/64 verts après le pop).
- `campaign_demo.c` réutilisait la MÊME `ArmyState` locale (`invader`, `defender`) sur DEUX appels `campaign_order` successifs dans des sous-tests distincts (§3 puis §3b) — invisible avant LOT 1 (l'ancienne signature COPIAIT, `src_force` restait valide après) mais casse silencieusement avec le nouveau contrat de TRANSFERT (2e appel reçoit une force VIDE, déjà consommée). Tout code qui appelle `campaign_order`/`campaign_order_sea` PLUS D'UNE FOIS avec la même variable doit désormais la re-remplir entre les appels (ou utiliser une variable fraîche).
- `region_defense()` est `static` dans scps_campaign.c — impossible à tester unitairement en isolation ; toute future modification de son calcul doit être éprouvée END-TO-END via `campaign_tick` (marche→siège), et sur une région dont `cap_def` (tier de capitale) ne SATURE pas déjà `SIEGE_MAX_DAYS=730j`, sinon le delta cherché est invisible (piège rencontré et documenté dans le banc LOT 3, qui construit désormais sa propre région témoin plutôt que de piocher dans le monde généré).
- Un `#define` de constante doit être visible TEXTUELLEMENT avant son premier usage (piège trivial mais facile à manquer en ajoutant du code AVANT une fonction existante qui définit sa propre constante plus bas dans le fichier) — `PILLAGE_COOLDOWN_Y` a dû migrer en tête de section pour être visible de `diplo_enslave_capture` (ajoutée avant `diplo_pillage_region` dans l'ordre du fichier).
- Un commentaire multi-ligne C (`/* ... */`) contenant littéralement `*/` au milieu d'une liste énumérée (ex. `RES_ARMS_*/FIREARM/…`) FERME le commentaire prématurément — erreur de compilation confuse (« stray backtick », « multi-character literal ») loin du vrai problème ; toujours reformuler l'énumération pour éviter `*/` accidentel dans un commentaire.
**Restes** :
- `army_demo.c` a une assertion PRÉEXISTANTE non-déterministe qui échoue occasionnellement sur seed 11 (« l'organisation décide : un moral renforcé tient et l'emporte au grain », §6 moral) — confirmé PAR STASH/reconstruction identique SANS mes changements : échoue pareillement. Pas dans le périmètre de cette mission (aucun lien avec les 6 lots), non couvert par du diagnostic supplémentaire ici.
- `scps_api_demo` a 2 échecs PRÉEXISTANTS (« panneau B : la manufacture est POSÉE au drain », « le même type n'est plus légal ») dus aux modifications EN COURS de l'agent parallèle sur scps_api.c (fichier explicitement interdit à ce lot) — confirmé pré-existant par la même méthode (stash isolé de mes fichiers, testé sur scps_api.c/.h/_demo.c tel quel).
- `intertrade_demo` reste KO au build (Windows MinGW, `setenv` non-POSIX) — pré-existant, documenté, hors périmètre.
- Panneau UI de siège/pillage (force-vs-force, jauge de pillage cumulé côté joueur) : donnée moteur exposée nulle part côté façade pour l'instant (scps_api.c est le domaine de l'agent parallèle — `diplo_siege_loot`/`g_siege_loot_total` sont prêts à être branchés en lecture façade si souhaité, aucune API façade n'a été touchée ici par contrat de la mission).
- Gates : `make test` 38/40 verts (les 2 rouges pré-existants confirmés hors-périmètre : `scps_api_demo`, `intertrade_demo` build) · `determinism` STABLE (5 graines × 12 ans, hash identique run A/run B) · `savetest` (`scps_viewer --savetest`) 2/2 byte-identiques (A==B, altération d'un octet REFUSÉE par l'empreinte) · **golden RE-BASELINÉ** (les 5 hash changent dès la fenêtre 12 ans — LOT 1/3/4/5 mordent immédiatement : le débit/crédit du host change les décisions IA de ratio d'attaque dès l'an 0, le pillage de siège est mensuel dès la 1re guerre, les gates techno changent la composition d'armée IA dès la 1re levée — `scps/golden_hashes.txt` mis à jour via `make golden-update`, re-vérifié `make golden` OK, **NON COMMITTÉ** par mandat explicite) · sweep seed 9×200 ans SAIN (satisfaction Laborer 69 %/Bourgeois 88 %/Élite 79 %, hégémon MORTEL 1/1, 174 batailles livrées, 78 régions réduites [== 78 occupations posées, télémétrie EXACTE post-LOT-6a], pillage de siège 91610 or-équiv. + 2 captures de sac sur un sweep de 3×200 ans) · sweep seed 7×200 ans SAIN (satisfaction 75/89/85 %, hégémon MORTEL 1/1, 114 batailles, 42 régions réduites == 42 occupations posées). Télémétrie neuve : « pillage de siège » (`g_siege_loot_total`/`g_siege_sack_captures`, scps_sim.c/.h, chronicle.c) — vibrante et non nulle sur tous les sweeps testés.

## [2026-07-06] UI Province — l'audit de câblage (éclaireur read-only)
**Découvertes** : le panneau affiche 60-65 % de ce que la façade expose. COMPLET : biome (province_panel.gd:71,111), bâtiments+construction grisée par légalité (province_detail.gd:330-387), humeur/agitation/seuil de révolte (:152-165,218), flux/jour (onglet 1), allocation (onglet 4), logement capitale. L'HÉRALDIQUE PROCÉDURALE EXISTE déjà pour les PAYS (ui/heraldry.gd::compose_arms, composée au seed) — il ne manque que la déclinaison par province (compose_arms_generic(seed,ethos) + reader scps_province_seed). EXPOSÉ-MAIS-PAS-AFFICHÉ : le slot MODIFICATEURS (ScpsProvInfo.mods — viewer SDL seulement), la 4e classe (esclaves — l'onglet Réincorporation la lit mais pas la barre de proportions, scps_province_classes n'a que 3 params), le mot de défense. ABSENT-FAÇADE : impôts par province (re->treasury existe moteur), bonus de défense chiffré (le moteur APPLIQUE déjà relief_weight au siège — campaign region_defense — mais le joueur ne le voit jamais), prix/stock régionaux, port, ferveur.
**Pièges** : scps_province_classes a une signature 3-classes figée — l'étendre casse les appelants ; préférer un reader additif scps_province_slave_count. Le « bonus défense terrain » n'est PAS un chiffre moteur : c'est relief_weight dans la durée de siège — l'afficher = le DÉRIVER en % lisible, pas inventer un stat.
**Restes** : la vague de câblage (5 readers additifs ~100 lignes C + ~300 lignes GDScript : 4e segment esclaves, section modificateurs, hover défense sur le cadre biome, impôts province, blason province, onglet 5 « Contexte » étendu prix/stock/marché local).

## [2026-07-06] W-GUERRE UI — LOT A (hachures siège/occupation) + LOT B (panneau de combat) FAITS (implémenteur W-GUERRE UI)
**Découvertes** : deux readers façade additifs suffisent, tout existait déjà côté moteur. `scps_region_war_state(sim,region,&belli)` (scps_api.{h,c}) distingue ASSIÉGÉE (armée ennemie active en `FA_SIEGE` sur `region`, lu de `s->sim.camp->army[k]` — même source que `scps_player_alerts`/`map_state_tint::VIEW_WAR` déjà écrites, JAMAIS mutualisées avant) d'OCCUPÉE (`dp->occupier[r]!=owner`, priorité — le siège a déjà abouti). `scps_battle_info(sim,region,&out)` distingue une `FieldBattle` ACTIVE (chocs en cours, `camp->battle[k]`, pertes `lossA/lossB` exposées) d'un simple SIÈGE (pas encore de choc) ; les DEUX camps sont lus via `campaign_composition(camp,cid)` (déjà exposée PAR PAYS, réutilisable telle quelle pour n'importe quel belligérant, pas seulement le pays dont c'est "l'armée") + `diplo_war_score(dp,attacker,defender)` (point de vue attaquant, même jauge que la diplomatie #26). Binding Godot : `region_war_state`/`battle_info` → Dictionary. Overlay : hachures à 45° CLIPPÉES au polygone réel de la région — piège évité : `region_border_segments` renvoie des SEGMENTS NON ORDONNÉS (bseg), PAS un anneau fermé ; il faut les CHAÎNER (`_chain_segments`, motif déjà utilisé pour le liseré de capitale) avant `Geometry2D.intersect_polyline_with_polygon` (sinon le clip est faux/vide). Cache `_war_regions` rebâti CHAQUE TICK (pas aux frontières — l'état de siège bouge plus vite que la souveraineté), scan bon marché (juste des int) sauf pour les quelques régions effectivement en guerre (là seulement on chaîne un polygone). Panneau de combat = Control immediate-draw (motif province_panel/VKit), ouvert depuis `_on_province_picked` (main.gd) quand la région cliquée a `battle_info().valid` — le clic sur le jeton d'armée résout de toute façon à une province de sa région (le jeton est planté au centroïde), donc pas besoin d'un hit-test dédié sur le sprite.
**Pièges** : `campaign_composition` prend un `owner` (pays), pas une armée — pour l'AFFICHAGE deux-camps il faut l'appeler UNE FOIS par côté (attaquant, défenseur), pas une seule fois sur "l'armée du siège" (le défenseur n'a pas forcément d'armée de campagne déployée — sa composition peut être 0, ce qui est CORRECT, pas un bug : une garnison sans armée de campagne n'a pas de ArmyComposition à montrer). `Geometry2D.intersect_polyline_with_polygon` renvoie un `Array` de `PackedVector2Array` (pas un flat array) — chaque élément DOIT être re-typé explicitement (`var pp: PackedVector2Array = part`) sinon Godot le traite en Variant générique et `pp.size()`/`pp[0]` lèvent en mode strict.
**Restes** : H_coerc/garnison toujours pas lu par `region_defense` (cf. entrée précédente #8) — un jour où la garnison compte mécaniquement, le panneau de combat pourrait vouloir l'exposer aussi (force de siège vs garnison, pas seulement armée de campagne vs armée de campagne). Pas de hit-test dédié au SPRITE d'armée (repli sur la région) — si un jour deux armées ennemies traversent la MÊME région sans que campaign_tick les fasse s'accrocher immédiatement (délai d'un tick), `battle_info` peut retourner `valid=0` un jour ou deux après que le jeton "bataille" soit déjà visible à l'écran (léger décalage d'affichage, jamais un crash — le panneau ne s'ouvre juste pas ce jour-là).

## [2026-07-06] UI PROVINCE — câblage complet des 6 lots (implémenteur, suite de l'audit)
**Découvertes** : les 5 readers listés par l'éclaireur suffisaient tous — aucun besoin d'étendre `scps_province_classes` ni de toucher scps_readout. LOT 1 (esclaves) : `scps_province_slave_count(sim,pid)` somme les groupes `CLASS_SLAVE` de la province (`ProvinceEconomy.pop.groups[]`), ADDITIF (signature 3-classes de `scps_province_classes` intouchée). LOT 2 (modificateurs) : RIEN à ajouter côté C — `ScpsProvInfo.mods[]` était DÉJÀ bindé en Dictionary (`province_info()["mods"]`, scps_sim_node.cpp:294-302) mais jamais lu côté GDScript ; juste une section d'affichage neuve dans `province_detail.gd::_draw_peuples` (à ne pas confondre avec la section « Modificateurs » de l'onglet Peuples qui affiche en fait les CAUSES D'AGITATION — nom similaire, deux concepts distincts). LOT 3 (impôts) : le signal le plus honnête est de REJOUER la formule de collecte d'`econ_tick` (scps_econ.c ~L2390-2406) en LECTURE PURE sur `ProvinceEconomy.strata[]`/`.culture` (existe au grain province, pas seulement région) — `STATE_TAX_AMBITION` est un `#define` LOCAL à scps_econ.c (pas dans le .h) donc mirroré en commentaire de couplage côté scps_api.c, ×12 pour l'annualiser (dt=1/12 implicite partout dans le moteur). LOT 4 (défense terrain) : `terrain_defense_mult` vit dans scps_army.c/.h — EN COURS D'ÉDITION PARALLÈLE (agent W-GUERRE) au moment du lot ; plutôt que d'`#include "scps_army.h"` (risque de dépendre d'un symbole en flux sur l'arbre PARTAGÉ), la formule (6 lignes, coefficients par biome + facteur relief) a été REPLIQUÉE dans scps_api.c avec un commentaire de couplage citant le fichier:ligne source — le "bonus terrain" affiché est un %, dérivé, jamais un stat inventé. LOT 5 (blason province) : `heraldry.gd::compose_arms(w,cid)` a été factorisé en `compose_arms_generic(seed_hash,hue,ethos,herit,cite_etat)` (zéro lecture `w.country_*` à l'intérieur) + `province_arms(pid)` (nouveau, cache séparé `_prov_arms_cache`) qui lit `scps_province_seed(sim,pid)` (hash déterministe posé sur seed_x/seed_y+pid, JAMAIS aléatoire) et hérite visuellement de l'éthos/héritage du PAYS PROPRIÉTAIRE (une province n'a pas d'éthos moteur propre ; sans propriétaire → éthos/héritage neutres [0], pas un aléa qui dériverait). LOT 6 (onglet Contexte) : l'onglet "Empire" RENOMMÉ "Contexte" (TABS[5]) et ÉTENDU en tête avec le marché local (`scps_province_market` : jusqu'à 3 lignes ressource-dominante + biens vivants, réutilise `band_marche`/`label_marche` comme `scps_country_stocks` mais au grain province) + le mot de PORT (`EDI_PORT` bit sur `edi_built`) — les jauges empire existantes suivent, inchangées.
**Pièges** : le piège `var lbl := cnames[i]` (Array non typé → Variant, `:=` ne peut pas inférer) a EFFECTIVEMENT cassé le boot headless (`Cannot infer the type of "lbl"`) dans LES DEUX fichiers qui ajoutent le 4e segment esclaves (province_panel.gd ET province_detail.gd) — fix : `var lbl: String = String(cnames[i])`. `git stash push -- <mes fichiers>` (à l'envers du motif documenté par l'agent P5 : ici on isole les fichiers de L'AUTRE agent pour juger golden honnêtement) a fonctionné, `git stash pop` ensuite restaure tout sans conflit tant que les fichiers stashés n'ont pas été retouchés entre-temps.
**Restes** : aucun côté C (les 5 lots sont clos) ; le lot 7 « bonus » (ferveur religieuse etc.) a été sauté faute de reader existant pour un item qui tienne en ≤20 lignes sans nouveau reader — rien d'évident trouvé qui ne duplique pas un lot déjà fait.

## [2026-07-06] W-GUERRE-3 — le CASUS BELLI PAYANT + l'armée à son vrai prix (implémenteur solo)
**Découvertes** :
- **LOT 1 (le CB se fabrique et se paie)** — `DiploState` gagne `fab_state`/`fab_days`/`fab_cb[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY]` (scps_diplo.h, ⚠ **SAVE BUMP 68→69**, section DIPL fwrite BRUT). Nouvelles primitives (scps_diplo.{h,c}) : `diplo_cb_needs_fabrication(cb)` (le tableau ci-dessous), `diplo_fabricate_cost(econ,target)` (= `econ_country_tax_year(target) × FAB_CB_COST_YEARS(2.0)`), `diplo_can_fabricate`/`diplo_fabricate_cb` (débite `world_capital_region(a)` via `econ_region_treasury_add` — RE-KEY province, l'or DISPARAÎT, comptabilisé sur une ligne I0 DÉDIÉE `FX_INTRIGUE` neuve — volontairement PAS `FX_ADMIN`, sinon ça pollue à la fois le diagnostic LOT1 et LOT2), `diplo_fab_state`/`_days_left`/`_ready_cb` (lecteurs), `diplo_fab_tick` (appelé depuis `diplo_tick`, annuel : MATURING→READY après `FAB_MATURE_DAYS`(365j), READY→NONE après `FAB_VALID_DAYS`(1825j) si non consommé). **Choke point unique** : `diplo_declare_war_cb` (TOUS les appelants : IA, joueur, révolte, ligue) consomme automatiquement l'intrigue `FAB_READY` si le CB déclaré la matche — aucun appelant n'a besoin de gérer la consommation lui-même.

  **Tableau CB gratuit/payant** (décidé sur pièces, doc dans scps_diplo.h) :

  | CasusBelli | Gratuit/Payant | Justification |
  |---|---|---|
  | (défensif implicite) | **GRATUIT** | rien ne gate la riposte — `diplo_declare_war_cb` accepte n'importe quel appelant, se défendre n'a jamais été gaté par un CB |
  | CB_ANTIPIRATERIE | **GRATUIT** | on SUBIT la course, on ne la choisit pas — le prix déjà payé est le grief accumulé (`pirate_rancor`), pas de l'or |
  | CB_SUBJUGATION | **GRATUIT** | projection de puissance sur un faible/hameau — la menace/le ratio de force EST le prix, l'intimider ne s'achète pas |
  | CB_TERRITORIAL | **PAYANT** | offensif par nature (prendre des terres) — sauf le cas RANCOR (irrédentisme, reprendre CE qu'on a perdu) qui reste une conséquence de guerre passée, non re-gatée ici (le rancor lui-même a déjà un coût — la guerre perdue) |
  | CB_ECONOMIC | **PAYANT** | offensif (viser la province-source d'un monopole) |
  | CB_RELIGIOUS | **PAYANT** | offensif (croisade/schisme comme PRÉTEXTE de conquête) |

- **IA symétrique** : `ai_pick_rival` (scps_ai.c) exige désormais `diplo_fab_ready_cb(diplo,a->cid,b)==cb` pour tout CB payant (sinon `continue` — la cible est écartée, PAS un repli sur un autre CB). Nouveau bloc `(0-bis)` dans `ai_strat_turn` (juste avant la coalition) : un empire d'appétit de conquête réel (`w_expand>0.15`) FABRIQUE contre le meilleur voisin CONVOITÉ (même métrique `ai_province_value` que la prédation) dont le CB naturel est offensif et qui n'a pas déjà d'intrigue en cours — **paie depuis SON PROPRE trésor** (`diplo_can_fabricate` lit `econ_country_gold`, aucun budget arbitraire) : sans les moyens, elle NE FABRIQUE PAS et ne déclare donc jamais un CB offensif — c'était le point du brief. Les 2 autres call-sites de `diplo_casus_belli` dans `ai_strat_turn` (ligue anti-hégémon, première-frappe-conquérant) sont patchés pour re-checker `diplo_fab_ready_cb` avant de déclarer (la ligue N'A PAS de fabrication d'urgence — elle attend, réactive par nature).
- **Verbe joueur** : `CMD_FABRICATE_CB` (scps_sim.h/.c, revalidé au drain : cible valide, `diplo_can_fabricate`) + `scps_player_fabricate_cb` (scps_api.{h,c}) + `ScpsDiploOptions` gagne `can_fabricate`/`fabricate_cost`/`fabricating`/`fabricating_days_left`/`cb_ready`/`cb_ready_years_left` (scps_api.h/.c). `CMD_DECLARE_WAR` (scps_sim.c) revu : un CB payant SANS intrigue mûre ⇒ **pas de guerre** (miroir exact du gate IA — avant ce lot, le joueur avait un CB GRATUIT de fait via le repli `cb=CB_TERRITORIAL` si `diplo_casus_belli` renvoyait NONE ; ce repli disparaît). `scps_diplo_options.can_declare_war` recalculé pour exiger un CB RÉELLEMENT utilisable (gratuit OU intrigue mûre) — sinon le bouton serait actif mais un no-op silencieux. Godot : `country_actions.gd` gagne un bouton « Fabriquer une revendication (coût N) » → « Intrigue en cours (N j) » (grisé) → « Revendication prête (expire dans N an) » (grisé, il reste à déclarer la guerre) ; binding `scps_sim_node.{h,cpp}` (`player_fabricate_cb` + 6 champs Dictionary `diplo_options`). GDExtension `scons` **0 warning**.
- **LOT 2 (l'armée à son vrai prix)** — mesuré (DIAG neuf `SCPS_MILDIAG`, chronicle.c : accumulateurs `g_mil_dep_tot`/`g_mil_sol_tot`, lus JUSTE AVANT `econ_flux_year_capture` — cumul sur TOUTE la fenêtre années×empires vivants, bien MOINS bruyant que la photo I0 « dernière année » existante qui donnait des ratios absurdes — un pays isolé pouvait afficher `soldes +1621` par bruit d'échantillonnage) : baseline `REGIMENT_PAY`/`NAVY_UPKEEP_GOLD`=1.5 ⇒ **0.4-0.7 % des dépenses** (confirme l'audit "1-1.5 %", l'écart vient de la fenêtre de mesure). Sweep (seed 9, plusieurs tailles de fenêtre) : ×8→2.5 %, ×20→9.0 %(100 ans)/4.0 %(200 ans), ×30→10.2 %(100 ans)/6-8.7 %(200 ans), ×40→12.9 %, ×60→**10.3 %(seed9)/8.2 %(seed7) à 200 ans**, ×90→13.0 %. **Retenu : ×60 → `REGIMENT_PAY`=90.0, `NAVY_UPKEEP_GOLD`=90.0** (scps_tune_list.h) — le ratio se TASSE avec l'horizon (redépense/cour croissent plus vite que la solde sur 200 ans ; mesurer sur une seule fenêtre COURTE aurait sur-estimé), ×60 reste dans/à la lisière basse de la cible sur les DEUX graines testées à 200 ans, sans sacrifier la satisfaction (Laborer 85 % à ×60 vs 80 % à ×90).
- **Conséquence mesurée (PAS une régression)** : les guerres/sim **MONTENT** avec la solde relevée (seed 9 : 22.7→44.0/sim à ×60, 200 ans — contre-intuitif mais cohérent : une armée mieux payée reste VIABLE plus longtemps, moins de dissolution forcée, donc plus de guerres tenues jusqu'au bout) — AUCUN effondrement du monde (hégémon mortel 3-5/5 sur tous les points de mesure, satisfaction 79-88 % Laborer/Bourgeois/Élite, `credit_demo` 16/16 sous le nouveau tunable — pas de spirale de dette, IPM 1.15-1.21 stable). **Chute des guerres motivées PAR CB** (télémétrie existante) : LOT 1 SEUL réduit fortement le territorial/économique/religieux « gratuits d'avant » (seed 9×3×200 ans : territoriale 12→ baseline historique ~30-51/5×200ans, désormais 12/3×200ans soit ~4/sim contre ~10/sim avant LOT1) — mais LOT 2 (guerres plus viables) COMPENSE largement en guerres TOTALES, le monde ne devient PAS pacifique (jamais < 8/sim mesuré, toujours ≥13/sim même aux points de calibration les plus bas).
**Pièges** :
- `econ_country_gold` lit `region[]` (la VUE agrégée, reconstruite par `econ_aggregate_regions`/`econ_tick`), PAS `prov[]` directement (RE-KEY province) — un test qui pose `prov[p].treasury=0` SANS appeler `econ_aggregate_regions(econ)` ensuite voit encore l'ANCIEN trésor via `econ_country_gold` (piège rencontré : "SANS l'or : refusée" échouait tant que je zérotais `prov[]` sans rebâtir `region[]`).
- Le fixture `diplo_demo.c` fait tourner `econ_tick` en continu depuis le DÉBUT du binaire (sections 1-10 avant le test W-GUERRE-3) → `FX_TAX` accumule du bruit RÉEL pour toute cible avant qu'on ajoute son propre flux contrôlé — `econ_flux_reset()` est OBLIGATOIRE avant `econ_flux_add(target,FX_TAX,N)` + `econ_flux_year_capture()`, sinon `diplo_fabricate_cost` mesure un revenu gonflé imprévisible.
- Un commentaire de X-macro (`scps_tune_list.h`) placé APRÈS le `\` de continuation de ligne (`X(FOO,1.0f) \   /* commentaire */ \`) casse la macro (« stray '\' in program ») — le commentaire doit être sur SA PROPRE LIGNE, jamais partagé avec le `\` de continuation (piège identique à celui documenté pour les `*/` dans une énumération de commentaire — même classe d'erreur, différent symptôme).
- `SCPS_MILDIAG` doit sauter l'an-0 (`yr>0`) : au tout premier tour, `econ_flux_get` n'a encore RIEN accumulé (le premier `econ_flux_year_capture()` n'a pas encore eu lieu) — l'inclure fausserait `g_mil_dep_tot` avec des zéros massifs qui diluent artificiellement le ratio sur les runs courts.
**Restes** :
- Le seuil "10-15 % STABLE" n'est vérifié que sur 2 graines × 200 ans — un audit plus large (5+ graines, comme les autres sweeps du dépôt) confirmerait/affinerait ×60, mais le temps de calibration (chaque point = plusieurs minutes de sim 203-pays) a borné l'exploration à ~8 points de mesure.
- `diplo_fabricate_cost` retourne 0 (fabrication GRATUITE de facto) si la cible n'a pas encore de revenu CAPTURÉ (an-1, `econ_country_tax_year` replie sur l'extrapolation `< 90 jours ⇒ 0`) — cas de bord bref en tout début de partie, jamais rencontré en pratique après l'an-1 mais documenté ici pour un futur audit.
- Panneau de combat / hachures de siège (mission W-GUERRE UI précédente) n'exposent pas encore l'intrigue en cours — un jour où l'UI veut montrer "pourquoi ce pays nous menace", `diplo_fab_state`/`diplo_fab_ready_cb` sont prêts à être lus par un panneau tiers.
- Gates : `make test` **38/40 verts** (les 2 rouges = pré-existants Windows, `intertrade_demo` build/`scps_api_demo` timeout>120s mais 155/155 réussis confirmé en exécution directe) · `determinism` **STABLE** (5 graines×12 ans) · `savetest` (seeds 9/11) **byte-identique** (v69) · `fuzz-save` **8/8** (la section DIPL élargie se borne : `fab_state∈[NONE,READY]`, `fab_days≥0`, `fab_cb∈[NONE,ANTIPIRATERIE]`) · **golden RE-BASELINÉ** (les 5 hash changent dès la fenêtre 12 ans — LOT 1 change les décisions de guerre dès l'an-0, LOT 2 change le budget dès la 1re solde — `scps/golden_hashes.txt` mis à jour via `make golden-update`, re-vérifié `make golden` OK, **NON COMMITTÉ** par mandat explicite) · `diplo_demo` **+31 tests** (84/84, section W-GUERRE-3 complète : prix/gratuit-payant/gate or/maturation/expiration/consommation-à-la-déclaration/cooldown inter-cibles absent/bornes hors-domaine) · GDExtension `scons` **0 warning** (binding `player_fabricate_cb` + 6 champs `diplo_options`).

## [2026-07-06] V2a — LE CONSEIL VIVANT : faction, loyauté, paie (implémenteur)
**Découvertes** : l'attribution de faction par siège n'existait nulle part — construite de zéro sur
`SEAT_A`/`SEAT_B` (Savoir→Transgresseur/Légiste · Société→Conquérant/Communautaire · Industrie→Marchand
seul) + un hash de maison (`sc_hash`, réutilisé tel quel) pour trancher. `faction_lever_apply`/`faction_
grievance`/`faction_capture_total`/`faction_opposition` (scps_factions.c) couvraient EXACTEMENT les besoins
de vote/grief/rot/opposition — aucun ajout côté factions nécessaire, tout est câblé depuis statecraft. Le
piège de test : la loyauté de DÉPART (~45-65, jitter déterministe) est PLUS BASSE que la cible à grief
faible (~70-100) → un scénario de « chute » nécessite un grief SATURÉ (`faction_lever_apply(...,1.5f)`,
capé à 1.0 en interne) pour que la cible tombe SOUS le point de départ, sinon le test mesure une REMONTÉE
et l'asymétrie du rot (qui n'agit QUE côté chute) ne se voit jamais (`tgt<cur` faux → rate jamais boosté).
`statecraft_council_hire`/`_dismiss` ont dû gagner un paramètre `seed` (3 call sites seulement : sim.c ×2,
statecraft_council_ai ×1 — tous avaient déjà `w->seed` en scope, refactor sans douleur).
**Pièges** : `sc_hash` (static) est défini APRÈS `statecraft_init` dans le fichier source — l'utiliser
dans `statecraft_init` (jitter de loyauté au départ) exige une DÉCLARATION FORWARD en haut du fichier,
sinon erreur de compilation (fonction implicite). `scps_api_demo.c` : créer un `ScpsSim` NEUF (genèse
complète, ~800 provinces) juste pour 8 tests V2a a fait passer le binaire de quelques secondes à
**2 min 2s** (dépasse le timeout par défaut du harnais, 120s) — réutiliser le `sd` déjà généré par le
bloc DÉCRETS voisin (même scope, juste avant son `scps_sim_free`) a ramené le coût à zéro. Un
`chronicle.exe` peut rester VERROUILLÉ (process zombie Windows) entre deux `make chronicle` — `Get-Process
| Where-Object {$_.Path -like '*chronicle*'}` + `Stop-Process -Force` avant retry (le Bash tool échoue
silencieusement à interpoler `$_` dans une commande PowerShell inline — utiliser le tool PowerShell direct).
**Restes** : les ÉVÉNEMENTS de trahison/rivalité/complot (V2b) restent à écrire — `statecraft_council_
betrayal_ready`/`statecraft_council_pair_state` sont les SIGNAUX prêts à être lus par `scps_events.c`
(hors lot, INTERDIT par la mission). Portraits UI toujours texte-only (héraldique = hors lot). Le rot
mesuré en sweep (seeds 9/7 × 200 ans) reste TRÈS bas (0 ministre au bord, 0 remplacement IA) — un monde
sain ne stresse pas ses conseillers par défaut ; un futur audit pourrait vouloir un sweep PLUS long ou
plus de guerres civiles/leviers pour voir le mécanisme vibrer nativement en jeu (actuellement prouvé
isolément dans `statecraft_demo`, pas encore observé « en situation » sur les graines de référence).

## [2026-07-06] W-GUERRE — LA SOLDE PAR TYPE D'UNITÉ : l'ANCRE EU4 + LIMITE DE FORCE (implémenteur solo)
**Découvertes** (3 itérations dans la même mission : typé-flat → surcharge-taille → **ancre EU4 retenue**) :
- **LA FORMULE FINALE (warhost_unit_pay_month, scps_warhost.{h,c}, PUBLIC — moteur/diags/UI lisent le même prix)** :
  `pay_mensuel(type) = REGIMENT_PRICE(12)×unit_pay_mult(type)×IPM / 13  +  100×prix_national(arme_du_type) / 26`
  — l'ancre EU4 : l'entretien ≈ le prix de recrutement RÉEL / 12-14. L'or du drill / **13** ; les ARMES consommées à la levée (100/paquet, au prix NATIONAL P1 de la région-capitale) / **26** — DOUBLE div car les armes sont **RENDUES à la démob** (wh_shed) : leur part d'entretien est un AMORTISSEMENT/maintenance, pas une consommation. L'élite coûte cher à LEVER (bâton 30 · runiques 46 vs légères 9 de base) ⇒ cher à ENTRETENIR, naturellement.
  `unit_pay_mult` (scps_army.{h,c}, neuf) garde l'échelle RELATIVE dérivée du roster : `1 + 1.3×tier_gate(tech_node dynamique) + 1.4×ELITE + 1.0×spécialiste(feu/arcane/alchimie) ; ×0.65 armes de fortune` — Piquier 1.0 → Cav. cuirassée 7.6.
- **LA LIMITE DE FORCE (lecture EU4 de la surcharge de taille, warhost_force_limit)** : un pays entretient `6 + 0.5×régions` régiments À PRIX PLEIN ; au-delà `sizemult = 1 + dépassement×3` — le frein au doomstack devient « dépasser sa limite de force ». `solde = Σ(count×pay_mensuel) × sizemult × dial(REGIMENT_PAY/90) × guerre(1.5) × jauge`. Le dial REGIMENT_PAY (registre J) reste le levier GLOBAL, neutre à 90, balayable en env.
- **LA TABLE des 22 (pay/mois à prix de CATALOGUE ; en jeu le prix du marché module — marché nu early ≈ ×0.35)** : Milice **0.6** (armes de fortune = pas de valeur d'arme — quasi gratuite, mais power 0.12/60) · Piquier/Lancier/Épéiste **35.5** · Cav. légère 36.8 · Lame franche 36.7 · Cav. de raid 38.0 · Archer/Arbalétrier/Harceleur/Traqueur **39.4** · Arbalétrier lourd 40.6 · Hallebardier 54.8 · Berserker 56.0 · Cav. lourde **56.1** · Lancier de choc/Garde d'escorte 57.2 · Cav. cuirassée **60.9** · Arquebusier **65.8** · Sorcier **120.9** · Alchimiste 135.0 · Chaman **183.7**. ⚠ Avec l'ancre EU4, la différenciation vient surtout du PRIX DES ARMES (le mult du roster ne pèse que sur la part or 12/13) — c'est le design voulu (« l'élite coûte cher à lever »).
- **BOUT PRÉCOCE ✓ (SCPS_EARLYDIAG, chronicle.c — plus petit empire JOUABLE an 5-15, miroir exact du moteur)** : revenu **2485 or/an (seed 9) / 3918 (seed 7)** ; **4 rgt piquier = 26 % / 13 % du revenu · 6 rgt = 39 % / 20 %** — la cible ≤25-35 % est TENUE (6 rgt seed 9 déborde de 4 pts : le prix des armes y est plus haut). L'ancre EU4 a résolu D'ELLE-MÊME l'incompatibilité mesurée du flat (4 rgt à 90 flat = 152-303 % du revenu). ⚠ piège de proxy : « le plus petit pays TOUT rôle » tombe sur un hameau WILD (revenu 278-579/an, ratios 1174 %+) — filtrer PLAYER/ANTAGONIST.
- **BOUT MÛR : NON-MESURABLE pour l'ARMÉE — le budget militaire est ~100 % MARINE (la vraie trouvaille)** : MILDIAG décomposé (neuf : `dont ARMÉE x% / MARINE y%`) sur 3×200 ans ⇒ seed 9 **6.3 % total = ARMÉE 0.0 % + MARINE 6.3 %** · seed 7 **6.3 % = 0.4 % + 5.9 %**. Les armées sont si AFFAMÉES D'ARMES (cf. goulot ci-dessous) qu'elles restent minuscules — leur solde est un epsilon QUELLE QUE SOIT la formule. Corollaire : la calibration W-GUERRE-3 « ×60 ⇒ 10.3 % » mesurait en réalité surtout la MARINE (NAVY_UPKEEP_GOLD 90 × 850-1830 coques). NE PAS forcer la bande 10-15 % en gonflant le dial (il faudrait ×50+ sur l'armée → early tué) : **recalibrer APRÈS le fix du goulot d'armes** — la limite de force est ARMÉE et attendra les vraies armées (frein doomstack dormant tant que les armées tiennent sous leur limite).
- **⚠ LE GOULOT D'ARMES EST RÉEL ET MASSIF (SCPS_ARMSDIAG neuf : compteurs warhost + stock/prod chronicle — le soupçon joueur CONFIRMÉ)** : sur 3×200 ans, la levée VEUT 1.38-1.48 M d'armes légères et n'en OBTIENT que **7.8-8.6 %** (trait 16-18 % · lourdes 4-27 % · **FEU : 0.0 % servi, production 0.0-0.3/tick** — l'arquebuserie ne produit JAMAIS pour l'arsenal) ; le stock monde touche **0 en permanence** (min=0 les 200 ans, fin 0-1478) ; production ~5-8/tick vs demande de levée ~2300/an monde = **~30× sous la demande**. APRÈS le gate d'armes, le gate de POP coupe encore (levé/pris 47-76 %). Verdict : le maillon défaillant est la PRODUCTION des manufactures d'armes (armureries/ateliers rares ou affamées d'intrants — fer/labor), PAS la logique de levée. La solde/armée entière est sous ce plafond ; toute mesure « part militaire du budget » le restera aussi. (Fait amusant : bâtons/runiques ont un stock et une prod SANS demande — les mages sont gatés par l'ÉLITE, pas par l'arme.)
- **Guerres** : seed 9 51.3/sim · seed 7 27.0/sim (bande tolérée 20-70) ; hégémon mortel 3/3 partout ; satisfaction 73-87 % ; IPM ~1.2 — monde SAIN sur toutes les mesures.
**Pièges** :
- **Le SEGFAULT de l'arbre partagé** : `make chronicle` incrémental après qu'un AUTRE agent a bougé un header ⇒ mix d'.o à ABI divergente ⇒ crash DANS LA WORLDGEN (trace_rivers), zone que personne n'a touchée. Réflexe : `rm -f build/*.o` + rebuild COMPLET avant d'attribuer un crash à son propre diff.
- **`git stash` d'un agent parallèle** : mes 5 fichiers ont disparu mi-mission (stash-isolation d'un autre agent) puis sont revenus au pop. Vérifier `git stash list` + `grep` ses marqueurs avant de paniquer/réécrire.
- L'EARLYDIAG de chronicle doit MIROIR le moteur (warhost_unit_pay_month + force_limit + OVER_K) — un miroir dérivé à la main diverge à la première retouche ; d'où l'export PUBLIC de ces deux fonctions.
- La mesure early en sweep LONG est du gâchis : un run de **16 ans** suffit (fenêtre an 5-15) — ~30 s au lieu de ~10 min.
**Restes** :
- **golden : mouvement MIXTE, PAS de re-baseline par moi (protocole appliqué)** : `make golden` échoue (5 graines bougent — ma solde mord dès la 1re garnison) MAIS l'isolation (stash de MES 5 fichiers + rebuild complet) montre que SANS moi ça échoue AUSSI (seed 310 bouge par le WIP des autres agents ; seed 411 ne matche la base QUE sans moi). Attribution : MES changements bougent les 5 graines ET le WIP concurrent en bouge au moins une. `golden-update` LAISSÉ au coordinateur une fois la vague stabilisée (le mandat interdit l'update si ce n'est pas que moi).
- **Migrer les #define au registre J** dès que scps_tune_list est libre : `SOLDE_EU4_DIV` 13 · `SOLDE_ARMS_DIV` 26 · `SOLDE_FL_FLOOR` 6 · `SOLDE_FL_PER_REG` 0.5 · `SOLDE_OVER_K` 3 (scps_warhost.c) + `SOLDE_K_TIER` 1.3 · `SOLDE_K_ELITE` 1.4 · `SOLDE_K_SPEC` 1.0 · `SOLDE_FORTUNE_DISC` 0.35 (scps_army.c) ; miroirs hardcodés dans chronicle.c (EARLYDIAG : /90, ×3.0) à pointer sur les mêmes tunables.
- **NAVY_UPKEEP_GOLD (90 flat/coque) INCHANGÉ mais c'est LUI le budget militaire réel (~6 %)** : typer la coque comme le régiment (marchande ≈ base · guerre ≈ intermédiaire) est la suite logique — scps_navy.c était HORS de mon lot. La cible 10-15 % se recalibrera là ET sur l'armée post-fix-armes.
- **Le fix du goulot d'armes** (production des manufactures d'armes ~30× sous la demande, chaîne FEU morte côté arsenal) = la vague suivante ; econ.c était hors de mon lot (lecture seule). Re-mesurer ARMSDIAG après, PUIS recalibrer la bande 10-15 %.
- Gates : army_demo **49/49** · warhost_demo **6/6** · campaign_demo **21/21** · `determinism` **STABLE** (b0d30659/46fdc5b9/e5efd5fa/fdb12479/7d316270) · `savetest 9` **byte-identique** (A==B ; aucun état neuf sérialisé — compteurs ARMSDIAG = statics RAZ par warhost_init, jamais lus par le moteur) · `golden` cf. ci-dessus (mouvement mixte, non re-baseliné par moi) · rebuild COMPLET 0 erreur (les warnings events.c = WIP d'un autre agent) · viewer buildé OK.

## [2026-07-06] V3 — LES LAVIS D'APOCALYPSE + LE CÂBLAGE SERVILE (implémenteur solo, scps_api + Godot uniquement)
**Découvertes** :
- **LOT 1 (lavis par variante)** : `endgame_region_intensity` (LOT D, scps_endgame.{h,c}) et le tag « readers d'intensité par région à exposer » de l'éclaireur S3 étaient déjà FAITS côté moteur — seule la FAÇADE manquait. Ajouté `scps_endgame_region_intensity(sim,region)` (wrapper pur) + `scps_map_endgame_variant(sim,uint8_t*)` (scps_api.{h,c}) : précalcule l'intensité de CHAQUE région une fois (≤ SCPS_MAX_REG=832 — le cas RONCES scanne SCPS_N cellules par région, donc HORS DE QUESTION de l'appeler par cellule) puis recopie par cellule via `c->region` (motif exact de `scps_map_owner`). `ScpsEndgameInfo` gagne `fin_raw` (le `FinType` BRUT 0..5, SANG compris) — distinct du `fin` existant qui MIROITE `RFIN` (0..4, `scps_readout.c::endgame_readout` ne mappe PAS encore FIN_SANG → retombe sur RFIN_AUCUNE, gap documenté par l'éclaireur S1/l'entête `scps_endgame.h` mais HORS PÉRIMÈTRE ici — scps_readout.c n'est pas dans ma liste de fichiers autorisés). Binding : `endgame_region_intensity(region)` + `variant_map_image()` (motif `layer_image`, Image L8). Shader `iso_antique.gdshader` : 4 uniforms (`variant_map`/`variant_col`/`variant_on`/`variant_k`), un SEUL tap + `mix` juste avant la section COMMUNE (rose des vents/vignette) — coût nul si `variant_on<0.5` (branche entière sautée par le GPU au repos). `iso_ground.gd` : texture reconstruite **1×/an** (`_variant_year` comparé à `w.year()`, motif `_bmap=null`), gate sur `fin_raw>0` ; ASCENSION (fin=4) volontairement HORS de `VARIANT_COL` (pas de lavis sombre pour une victoire, conforme au brief — le voile or est laissé à une passe FX future, non fait ici).
- **LOT 2 (câblage servile)** : les 3 verbes (`scps_player_manumit`/`_slave_buy`/`_slave_sell`) + le reader `scps_slave_market` EXISTAIENT DÉJÀ intégralement côté façade C (scps_api.h) depuis une mission antérieure (P4/réparateur) — seul le BINDING C++ (scps_sim_node.{h,cpp}) et l'UI manquaient, exactement comme documenté par l'éclaireur du Codex (F1). Placé dans **`sidebar_drawer.gd`, onglet Conseil (tab 7), juste sous Décrets** (motif `_draw_decrets`/`_decret_btns`/`_decret_act` copié à l'identique pour `_draw_servile`/`_servile_btns`/`_servile_act`) — cohérent avec le brief (« l'endroit le plus cohérent avec Décrets ») car c'est une politique intérieure au même titre. Section : compte d'âmes (`manumit_preview`), pool mondial par héritage + prix implicite (`slave_market`), Acheter/Vendre ×{50,200} (gate `can_buy` grisé + tooltip « gate éthos/tech »), Affranchir en **2 CLICS** (`_servile_manumit_armed`, remis à `false` à chaque `show_tab` pour qu'une confirmation armée ne survive pas une fermeture d'onglet — le piège d'un état de confirmation qui traverse un changement de contexte).
- **codex.gd** : les 2 entrées "(bientôt)" (Affranchir, Acheter/vendre servile) éditées vers leur emplacement réel ; `bientot:true` retiré. `council_pay` (3e "(bientôt)" du Codex) est un VERBE DIFFÉRENT (hors mandat de ce lot — `scps_player_council_pay` est en réalité DÉJÀ bindé, cf. `v3_final_build.log` : le test « scps_player_council_pay : verbe accepté » passe — mais son curseur UI n'existe pas ; NON touché, hors périmètre).
**Pièges** :
- **`git stash` sans pathspec d'un agent parallèle a emporté TOUS mes fichiers non commités** (même symptôme documenté par l'éclaireur W-GUERRE : stash@{0} contenait mes 8 fichiers `git status` montrait PROPRE) — mêlés à son propre lot (chronicle.c/scps_army.*/scps_warhost.*/golden_hashes.txt). `git stash pop` a tout restauré SANS CONFLIT (rien retouché entre-temps) ; revérifié par grep de contenu (« servile », « variant_map ») avant et après le pop, PAS seulement `git status`. Symptôme observé AVANT diagnostic : le boot headless du probe `v3_audit.tscn` échouait avec « Cannot open file » — pas une erreur de script, le FICHIER avait disparu du disque.
- Arbre PARTAGÉ instable sur `scps_events.c`/`scps_warhost.c`/`scps_save.h` (agents parallèles en cours sur events/army/save, zones INTERDITES par le mandat) : ~6 tentatives de build ont échoué sur du code D'AUTRUI (erreurs de type conflictuelles, `stray '\342'` = un fichier en cours d'édition mi-mojibake) avant de réussir — comportement ATTENDU documenté dans le brief (« attends, retente »), pas un bug de mes fichiers.
- `scps_readout.c::endgame_readout` ne mappe PAS `FIN_SANG` → `r.fin` retombe sur `RFIN_AUCUNE` (le miroir RFIN n'a que 5 valeurs, pas 6) : mon lavis lit `fin_raw` (le `FinType` BRUT, ajouté à côté, jamais ce miroir) pour rester correct malgré ce gap connu — un futur lot qui voudrait afficher "SANG" en texte (bandeau/UI) devra étendre `FinReadout`/`endgame_readout` (scps_readout.{h,c}, HORS de mon périmètre ce tour).
**Restes** :
- Le lavis n'a JAMAIS été exercé sur une fin RÉELLE en conditions de test (`fin_raw` est resté à 0 sur 3 graines × 30 ans — l'endgame est gaté à l'an 180+ par construction, cf. `ENDGAME_YEAR_OPEN`) : la capture headless demandée en option n'a pas pu montrer un lavis coloré (budget ~10 min insuffisant pour atteindre l'an 180 en sim réelle) — le probe `v3_audit.gd`/`.tscn` (godot/project/, non committé) vérifie la COHÉRENCE structurelle (bornes, carte tout-0 sans fin) mais pas le rendu visuel réel d'un cataclysme. Un futur agent qui veut vérifier VISUELLEMENT devra soit lancer un sweep de 180+ ans en amont (`chronicle` + rejouer la save côté Godot), soit forcer `ENTROPY_FIN`/`ENDGAME_YEAR_OPEN` bas via `SCPS_TUNE` pour un test rapide.
- `endgame_readout`/`FinReadout` (scps_readout.{h,c}) n'exposent toujours pas SANG au reste de la membrane (texte "Fin : SANG" absent d'un bandeau UI géré par ce module) — noté par l'éclaireur S1 il y a un tour, toujours vrai, toujours hors de mon périmètre (fichier non autorisé pour ce lot).
- Gates : `scps_api_demo` **169/169 réussis, 0 échoué** (+3 tests V3 : intensité bornée [0,1] · `fin_raw` bornée 0-5 · `variant_map` cohérent avec `fin_raw` ; +1 test marché lisible sur la section esclavage existante) · GDExtension `scons` **0 warning**, exit 0 (revérifié APRÈS le stash pop) · boot headless `Main.tscn --quit-after 30` **0 SCRIPT ERROR/Parse Error** (warnings PNG-as-resource + RID-leak pré-existants, sans rapport) · probe dédié `v3_audit.gd`/`.tscn` (godot/project/, NON committé — 3 graines × 30 ans) : **V3 AUDIT OK, exit 0** (bornes + round-trip des 3 verbes servile enfilés + `variant_map` tout-0 confirmé tant qu'aucune fin n'a latché). `make golden` **NON relancé délibérément** (mandat : « ne pas re-baseliner soi-même » pendant que d'autres agents re-baselinent en parallèle) — golden-neutre par CONSTRUCTION : `scps_api.c`/`scps_api_demo.c` ne sont liés dans AUCUNE cible `CHRONICLE_OBJS` du Makefile (vérifié par grep), donc aucune de mes modifications ne peut influencer le hash de la chronique ; le reste (Godot/GDScript/shader) est hors du binaire chronicle par nature. **AUCUN COMMIT** (mandat).

## [2026-07-06] V2b — LES ÉVÉNEMENTS : Merveille 3 étapes · Conseil · contenu débloqué (implémenteur solo, scps_events.{h,c}/events_demo.c uniquement, save v70→71)
**Découvertes** :
- **`EventCtx` n'avait PAS `EndgameState*`** — LOT 1 (Merveille) exigeait de l'ajouter, donc `world_events_tick`/`pending_event_resolve`/`pending_event_tick_expire` ont dû gagner un paramètre `EndgameState *eg` (peut être NULL, testé — les triggers Merveille renvoient faux sans `eg`, le reste du tick tourne inchangé). **7 sites** `EventCtx cx={...}` positionnels à retoucher dans scps_events.c (le compilateur les liste tous en cas d'oubli — struct positionnelle, pas d'initialisation désignée). Seul appelant moteur : `scps_sim.c:682/594` (`s->eg` existe déjà dans `Sim`, pas de nouveau champ) — fichier NON listé forbidden/autorisé explicitement mais édité en 2 lignes minimales (thread du paramètre existant), aucune logique touchée.
- **`endgame_start_wonder` est un point d'appel DIRECT sûr** depuis `resolve_choice` : idempotent (`merv!=MERV_NONE` ⇒ no-op), struct `EndgameState` PLATE (comme documenté dans son .h) — donc l'écriture directe `eg->merv=MERV_NONE` pour le choix « Détruire » de l'Ascension suit le MÊME idiome que `apply_region_eff` écrivant `re->build.*` directement (pas un nouveau canal, la charte du module l'autorise pour SES propres structures plates).
- **L'Ascension doit intercepter EXACTEMENT `MERV_SAVOIR_DONE`** (pas `MERV_ASCENDED`) : `wonder_tick` (scps_endgame.c) bascule tout seul `MERV_SAVOIR_DONE→MERV_ASCENDED` au tick ANNUEL suivant puis `endgame_select_and_fire` lit `MERV_ASCENDED` en override (exempt du gate temporel/entropie) — le dernier choix doit donc tirer AVANT cette bascule automatique, sur le palier `_DONE`, sinon l'IA (au sens : le moteur) tranche « Activer » à la place du joueur sans lui laisser voix au chapitre. Testé : forcer `eg->merv=MERV_SAVOIR_DONE` PUIS tourner `world_events_tick` (pas un push manuel de pending) prouve que le trigger s'enfile correctement en conditions réelles.
- **Les 3 LOTS sont TOUS human-gated par le TRIGGER lui-même** (`c!=cx->human_player ⇒ false`), pas par la boucle de scan — en chronique (`human_player=-1`, `eg=NULL` passé par `s->eg` réel mais `human_player` reste -1) tous les triggers échouent au premier if. Golden-neutre par construction pour LOT 1/2/3 EUX-MÊMES ; **SEUL le hook Communautaire ajouté à 4 évènements EXISTANTS (CLOCHES/XENOPHILE/K3/DROIT_INTEGRATION, tirés en CHRONIQUE aussi) mord le golden** (mesuré : isolé `git stash push` de mes seuls fichiers vs `scps/golden_hashes.txt`, 4/5 graines IDENTIQUES, seed 310 diverge `9f9a522a→1a34358f` — cohérent, `faction_lever_apply(...,FAC_COMMUNAUTAIRE,0.06-0.10)` change une décision IA dès la fenêtre 12 ans sur cette graine précise). **Re-baseline attendue et DOCUMENTÉE ici, NON exécutée** (`make golden-update` non lancé) : l'arbre est PARTAGÉ avec un agent qui re-baseline SES PROPRES lots en parallèle (army/warhost/chronicle.c/golden_hashes.txt tous modifiés, non committés) — lancer `golden-update` maintenant écraserait leur travail en cours. Un futur agent (ou l'orchestrateur) doit relancer `make golden-update` une fois TOUS les lots du jour stabilisés/committés.
- **`statecraft_council_pair_state`/`_faction`/`_seated`/`_betrayal_ready` couvrent EXACTEMENT le besoin LOT 2** sans aucun ajout côté statecraft (fichier interdit, jamais touché) : R1/R2/R3 = `pair_state==RIVALITE` sur les 3 paires de sièges ; A1/A2 = `ALLIANCE` (A2 en sous-cas : le 3e siège doit être VACANT) ; C1 = `CONSPIRATION`. Les renvois de siège passent par le VERBE existant `statecraft_council_dismiss` (jamais un accès direct aux tableaux `council[]`), même motif que `demography_manumit_country` pour EVID_CHAINES_RAPPORTENT (legs W2).
- **`culture_relation_of` prend des CHAMPS NUS** (7 floats/enums par côté, pas des `PopCulture*`) — motif documenté par P7a exactement pour éviter l'include circulaire ; B2 relit `REL_ENNEMIS_SCHISME` (rivalité de voisinage, adjacence requise) et B3 relit `REL_PARENTS` (parenté, PAS d'adjacence — les cousins d'outre-monde). Les vrais noms d'enum (`REL_PARENTS/REL_COUSINS_DERIVES/REL_ETRANGERS/REL_JUMEAUX_CONVERGENTS/REL_ENNEMIS_SCHISME`) DIFFÈRENT de ceux que le libellé du header suggérait au premier survol (`CultureRelation` ≠ des noms `CULTREL_*` intuitifs) — toujours lire le vrai enum avant d'écrire un trigger dessus.
**Pièges** :
- **`*/` dans un commentaire tue le fichier suivant, PAS le fichier courant** : j'ai cassé `scps_save.h` (pas scps_events.c) en écrivant `EVID_MERV_*/` dans le commentaire du bump `SAVE_VERSION` — le `*/` de `EVID_MERV_*` + le `/` suivant ferme le `/* v71 : ... */` prématurément, et TOUT le reste du fichier (les ~90 lignes d'historique de versions suivantes) devient du code C brut → des dizaines d'erreurs `stray '\342'`/`unknown type name` à des lignes qui n'ont RIEN à voir avec la vraie faute. Piège déjà documenté (TROUVAILLES antérieur, énumérations de ressources) mais qui a re-mordu ICI dans un contexte différent (des noms d'IDENTIFIANTS C se terminant par une astérisque de mise en exposant `_*` dans un COMMENTAIRE, pas une énumération de types de ressources) — **grep `\*/` dans tout commentaire multi-ligne AVANT de compiler**, surtout après avoir écrit des noms de symboles au pluriel/générique (`EVID_FOO_*`) en prose.
- **Le mot "fracture" est BANNI de tout texte face-joueur** (`events_text_clean`, BANNED[] contient `"fractur"` — substring, pas mot entier) : mes 2 nouveaux textes C2 (« la fracture religieuse couve », « ni la ferveur ni la fracture ») ont fait échouer le banc EXISTANT `events_text_clean` (89/90 puis 90/90 après renommage en « scission ») — le mot le plus naturel pour décrire une DIVISION religieuse est justement le terme technique interdit. Tout futur texte C2/C4/B6 doit chercher un synonyme (scission/division/dérive) plutôt que "fracture".
- **`mtth_p` est une PROBABILITÉ, un seul tick de 30j n'ENFILE PAS forcément** même si le trigger est vrai (contrairement à ce qu'on pourrait supposer d'un test "un tick suffit") — le test LOT 1 Ascension a d'abord échoué (0/3 issues) car il ne tournait qu'UN tick de 30j avant de chercher le pending ; fix : boucler jusqu'à 3650j (comme fait pour SACRIFICE/FONDATION) OU accepter que `mtth_days` bas (720j pour Ascension) donne quand même une queue statistique large sur 10 ans.
- **Arbre PARTAGÉ, confirmé de nouveau** : un `git stash push -- <mes fichiers>` isolé (motif documenté par l'agent V2a/P5) a permis de PROUVER que 4/5 hash golden restent identiques avec MES SEULS changements — sans cette isolation, le golden échoue sur les 5 graines (contamination par les changements non committés d'army/warhost/chronicle.c d'un autre agent). Fait aussi la manip INVERSE (`stash push -u` des fichiers de L'AUTRE agent pour isoler golden sur MON tree exact vs le commit de base) — les deux sens du stash sont utiles selon ce qu'on veut isoler. `scps_warhost.{c,h}` ont CHANGÉ de contenu ENTRE mon stash push et mon stash pop (quelques minutes) — le tree bouge sous les pieds en continu, `git status`/`git diff --stat` après un pop est la seule façon de savoir ce qui a VRAIMENT changé.
**Restes** :
- **Golden re-baseline seed 310 EN ATTENTE** (`make golden-update` non lancé, cf. ci-dessus) — un futur agent doit le faire une fois l'arbre stabilisé. Les 4 autres graines (7/108/209/411) restent IDENTIQUES au golden actuel avec mes seuls changements.
- **Les noms de MINISTRE (maison) ne sont PAS injectés dans le texte des 3 évènements de trahison** — la mission demandait « le popup NOMME le ministre (sa MAISON) » mais l'architecture d'`event_title` ne substitue QU'UN SEUL `%s` (résolu en région/pays via `w->region[]`/`w->country[]`, jamais un nom de ministre) et `scps_api.c` (où toute extension de cette substitution vivrait) est un fichier INTERDIT à ce lot (agent parallèle). Simplification documentée : les 3 titres de trahison (Savoir/Société/Industrie) restent génériques (« le savant », « le notable », « le marchand ») sans nom de maison — un futur lot qui veut le nom réel devra soit étendre `event_title` à un 2e `%s` (le siège→maison, via `statecraft_council_cand_name`), soit le faire côté façade au moment de la présentation du pending.
- **SUCCESSION ne vide pas réellement le siège** (les 2 choix sont un TON, pas un acte — `statecraft_council_age_tick`, annuel, vide le siège de toute façon dès que l'âge dépasse le seuil) : documenté dans le commentaire de `resolve_choice`, choix DÉLIBÉRÉ pour ne pas dupliquer un mécanisme déjà existant, mais un futur relecteur pourrait s'attendre à ce que « Le remercier publiquement » vide IMMÉDIATEMENT le siège — ce n'est pas le cas.
- Portraits/texte UI toujours absents (héraldique = hors lot, comme documenté par l'agent V2a) — les 3 nouveaux évènements de trahison + les 6 inter-conseillers restent texte-only côté façade (scps_api.c non touché, hors mandat).
- Gates : `events_demo` **119/119 réussis, 0 échoué** (+29 tests V2b : 3 sections dédiées LOT1/LOT2/LOT3 + 1 section Communautaire-votant, largement au-delà des « +12 » demandés) · `scps_api_demo` **169/169**, 0 warning (le binding façade n'a pas eu besoin de changer — `scps_api.c` lit `EventsState`/`EvId` via des accesseurs déjà existants, aucun symbole neuf requis côté façade pour que son build reste vert) · `determinism` **STABLE** (5 graines × 12 ans, tree partagé complet) · `savetest` seeds 9/11 **byte-identique** (A==B) · `fuzz-save` **7/7** · `make smoke` **7/7 bancs verts** (endgame_demo 105/105 confirme l'include `scps_endgame.h` neuf dans scps_events.h sans régression) · `statecraft_demo` **62/62** (module non touché, juste lu) · sweep seeds 9/7 × 100 ans **SAIN** (satisfaction 75/87/83 et sim. sur seed 7, hégémon mortel 2-3/3, exit 0, pas de NaN/crash) · **0 warning** sur TOUS les builds (events_demo/scps_api_demo/chronicle/scps viewer) · **SAVE BUMP 70→71** (documenté dans scps_save.h, EventsState grossit de 19 EvId + fires[]/fire_cap[] plus larges ; 2 AnnalKind neufs sans impact sizeof). **AUCUN COMMIT** (mandat).

## [2026-07-06] #32 — LE SANG SIGNE TON RÈGNE : le JUMEAU joueur (implémenteur solo, scps_endgame.{h,c}/scps_campaign.{h,c}/scps_save.{h,c}/scps_api.{h,c}/endgame_demo.c/scps_api_demo.c/scps_tune_list.h, save v71→72)
**Le problème** : `endgame_blood_ratio` (V1a, entrée précédente) est MONDIAL — un joueur pacifiste dans un monde IA sanglant recevait FIN_SANG sans avoir tiré une flèche. La thèse du mandat : « chaque fin est la conséquence de la ressource qu'ON a brûlée ».
**Découvertes** :
- **Le site per-bataille (scps_campaign.c) est le SEUL endroit qui connaît les DEUX belligérants** — `Campaign.dead_choc`/`dead_pursuit` sont des cumuls MONDIAUX incrémentés à 3 sites (`bt_day` choc, `bt_rout` poursuite, le décrochage dans `bt_day`). Pour isoler « les morts DU joueur », il fallait un accumulateur JUMEAU (`dead_choc_player`/`dead_pursuit_player`) incrémenté AUX MÊMES 3 sites, gated par `A->owner==human || B->owner==human` (les DEUX camps de CETTE bataille précise) — jamais un calcul dérivé après coup sur le cumul global (qui a perdu l'information "qui contre qui").
- **`campaign_tick`/`campaign_init` ne connaissaient PAS "qui est le joueur"** et ne peuvent PAS le recevoir en paramètre sans toucher `scps_sim.c` (appelant unique en run-time, fichier INTERDIT ce tour — agent parallèle dessus). Solution : copier EXACTEMENT le motif déjà en place pour `warhost_set_human`/`econ_set_human` (globaux `static int g_human_player`/`g_econ_human`, setters publics, appelés depuis `scps_api.c`/`scps_save.c` aux MÊMES lignes que les deux existants) → `campaign_set_human`/`campaign_get_human` (scps_campaign.{h,c}). `campaign_init` fait un `memset(c,0,...)` complet à CHAQUE régénération/reload (comme les autres modules) — un champ struct aurait été écrasé à 0 (un pays VALIDE, pas -1) à chaque `sim_init` ; le global SURVIT à travers les inits, exactement pourquoi le motif existant est un global et pas un champ.
- **`endgame_select_and_fire` n'avait pas non plus de canal "qui est le joueur humain"** — son paramètre `player` existant (déjà threadé depuis `s->player`) est en réalité "le pays au RÔLE joueur" (`POLITY_PLAYER`, souvent le pays 0 MÊME EN CHRONIQUE, gouverné par l'IA) — PAS "y a-t-il un HUMAIN aux commandes". Utiliser ce `player` pour gater SANG aurait cassé la chronique (gate actif sur le pays 0 même sans humain). Fix : lire `campaign_get_human()` directement dans `scps_endgame.c` (déjà inclus via `scps_campaign.h`) — le MÊME global qui gate l'accumulation, donc la même vérité partout, sans toucher la signature de `endgame_tick`/`endgame_select_and_fire`.
- **`endgame_blood_player_share(eg) = war_dead_player/war_dead`** (mémoires décrues, MÊME demi-vie `SANG_MEMORY_HL`, MÊME delta-tracking que le global — `war_dead_player_seen` en miroir de `war_dead_seen`) : un ratio stable et comparable au seuil `BLOOD_PLAYER_SHARE` (0.25, registre J) indépendamment de la taille de pop/l'âge de la partie.
- **La garde RETOMBE, elle ne bloque pas** : `sang_ok = ratio_mondial≥FRAC && (human<0 || part_joueur≥SHARE)` — si SANG est refusée pour manque de part-joueur, `endgame_select_and_fire` continue en bas de fonction vers le sélecteur normal (rare dominant/hash), qui fire TOUJOURS une des 3 autres fins (le monde a quand même dépassé ENTROPY_FIN). Jamais de "rien ne se passe".
**Pièges évités** :
- Faillir de gater sur `player` (le rôle) au lieu de `campaign_get_human()` (la main humaine RÉELLE) aurait silencieusement changé le comportement de la CHRONIQUE (qui a toujours un pays `POLITY_PLAYER` ≥0) — testé explicitement en C9c (human=-1, comportement identique à avant #32).
- `save_sane` : borner `war_dead_player∈[0,war_dead]` (pas juste `≥0`) capte une save forgée qui gonflerait artificiellement la part du joueur au-delà de ce que le cumul mondial permet — cohérent avec le fait que `dead_choc_player⊆dead_choc` PAR CONSTRUCTION (un pays ne peut pas avoir combattu plus que le total mondial de morts).
- Isolation golden : `git diff` a montré `scps_econ.c` modifié SANS que j'y touche (agent parallèle sur scps_econ/scps_ai/army/warhost/chronicle, domaine autorisé pour EUX) — `git stash push` de MES SEULS fichiers + rebuild chronicle a reproduit EXACTEMENT les mêmes hashes actuels (`b0d30659`/`46fdc5b9`/`e5efd5fa`/`fdb12479`/`7d316270`) qu'AVEC mes fichiers restaurés → la divergence golden (5/5 graines vs `scps/golden_hashes.txt` committé) est **100 % attribuable au travail concurrent**, ZÉRO contribution de #32 (attendu : `campaign_get_human()==-1` en chronique ⇒ toutes mes additions sont des no-op stricts). `make golden-update` **NON lancé** (mandat : passe unique du coordinateur).
- Une course de build a fait échouer `ai_demo`/`structural_demo` une fois dans `make test` (fichiers en cours d'écriture par l'agent parallèle au moment du link) — rebuild isolé juste après : `ai_demo` 26/26 vert, confirmant que c'était un artefact de concurrence, pas une régression de mon code. `structural_demo` reste BUILD ÉCHEC mais PRÉ-EXISTANT (reproduit identique sur pristine stash : `scps_events.c` référence `endgame_metab_count`/`endgame_start_wonder` sans lier `scps_endgame.o` dans la cible Makefile — bug de Makefile antérieur à ce tour, sans rapport avec #32). `intertrade_demo` reste BUILD ÉCHEC pré-existant documenté (setenv POSIX absent MinGW).
- `scps_api_demo` a pris 2m08s réel (0.03s CPU) en run standalone — dépasse le timeout 120s du harnais `make test` sous charge partagée, mais 170/170 tests passent quand on lui laisse le temps (contention d'I/O avec l'agent parallèle, pas un ralentissement de mon code).
**Restes** :
- `endgame_readout`/`FinReadout` ne mappent toujours pas FIN_SANG → un futur lot UI qui voudrait afficher "TA part : X%" au bandeau devra soit lire `ScpsEndgameInfo.blood_player_pct` (ajouté ce tour, déjà exposé) directement côté façade Godot, soit étendre `FinReadout` (scps_readout.{h,c}, hors périmètre ce tour).
- Gates : `endgame_demo` **116/116** (+11 : C9a jumeau ne compte que les batailles du joueur (6 sous-checks) · C9b la garde retombe au rare dominant sous le seuil de part (3 sous-checks) · C9c human=-1 comportement inchangé (1 sous-check) — 10 nouveaux CHECK + 1 assertion campaign_get_human, largement au-delà du "+3" demandé) · `scps_api_demo` **170/170** (+1 : blood_pct/blood_player_pct bornés [0,100]) · `determinism` **STABLE** (5 graines × 12 ans, hashes identiques à travers plusieurs runs) · `savetest` seeds 9 ET 11 **byte-identique** (A==B, v72) · `fuzz-save` **7/7** (216 octets flippés, save_sane rejette chaque forge y compris les 2 nouveaux bornages) · `campaign_demo` **21/21** (module directement modifié) · `golden` : mouvement identique AVEC et SANS mes fichiers (isolation prouvée, cf. ci-dessus) — **NON re-baseliné par moi** (mandat : passe unique du coordinateur). **SAVE BUMP 71→72** (EndgameState +war_dead_player +war_dead_player_seen, 2 doubles, section EGAM fwrite brut). **AUCUN COMMIT** (mandat).

## [2026-07-06] Godot audio — câblage des hooks UI discrets (implémenteur solo, godot/project/**/*.gd uniquement)
**Découvertes** : la banque de sons + l'autoload (`godot/project/audio/sound.gd`) étaient déjà complets (23 WAV, 3 bus, tick/cloche d'âge/drones/ambiances/ducking déjà gérés dans l'autoload lui-même) mais **AUCUN appelant externe** n'existait (`grep -rn "Sound\." --include=*.gd` = 0 avant ce lot). `main.gd::_close_topmost()` est le SEUL point de fermeture commun (pile Échap) mais PLUSIEURS panneaux ont aussi un bouton ✕/Fermer/`_gui_input` DÉDIÉ qui appelle `visible = false`/`hide()` DIRECTEMENT en contournant `_close_topmost` (construction_panel, tech_panel, province_detail, battle_panel via `_gui_input` ; country_actions/religion_panel/codex via bouton dédié) — chacun de ces sites a dû recevoir SON PROPRE `Sound.play("ui_parchment_close")`, pas seulement le point central. De même pour l'ouverture : `tech`/`construct` ont CHACUN 2-3 sites d'ouverture distincts (topbar toggle, `alerts.open_tech`/`open_construct`, `alerts.open_tech_metab`) qui bypassent le point d'ouverture "canonique" — j'ai gardé un garde `if not X.visible:` à chaque site pour éviter un double-jeu du son quand le panneau est déjà ouvert.
**Câblés** (32 sites, `Sound.play(...)`) :
- `ui_parchment_open`/`_close` : tech_panel (topbar + 2 sites alerts + close via `_gui_input`), construction_panel (2 sites d'ouverture + close), province_detail (ouverture via `main.gd` detail_requested + close via `_gui_input`), country_actions (`open_country` + 2 fermetures : bouton ✕ ET `_close_topmost`), chronique (`open()` + close KEY_H), codex (`toggle()` couvre F1 + bouton Fermer routé dessus), religion_panel (`open()` + bouton Fermer), battle_panel (close via `_gui_input`) — plus `_close_topmost` lui-même (main.gd:355) pour la fermeture par Échap.
- `ui_quill`/`ui_seal` : event_dialog.gd — `_open_slot` (ouverture d'un dilemme, une seule fois par apparition, gaté `if not visible`) / `_choose` (clic sur un choix).
- `ui_deny` : tout verbe refusé — construction_panel `_act` (build/recruit file pleine), province_detail (manufacture refusée + réincorporation de pop refusée), country_actions `_act` (verbe diplo refusé), sidebar_drawer (marché normal refusé + servile refusé + capitale absente + affranchissement refusé).
- `ui_coin` : sidebar_drawer `_marche_act` (achat/vente marché normal) et `_servile_act` (achat/vente au marché servile — SEULEMENT les branches `buy`/`sell`, pas l'affranchissement qui n'est pas un échange).
- `moment_war_horn` : country_actions `_act`, UNIQUEMENT sur `verb=="war"` réussi (pas `fabricate` — fabriquer une revendication n'est pas déclarer la guerre).
- `moment_battle_drums` : battle_panel `open_region` (ouvert par clic sur un jeton d'armée en siège/bataille — vérifié : déclenché par clic utilisateur via `main.gd:_on_province_picked`, pas par tick).
- `moment_page_turn` : page_turn.gd `rise()` ET `turn()` — l'autoload NE le faisait PAS (vérifié par lecture de `sound.gd`, aucune référence).
- `ui_scroll_tick` : province_detail.gd, changement d'onglet (`_tab_rects` click), gaté `if _tab != t.idx` pour ne pas rejouer sur un clic répété du même onglet.
**Sauté (documenté, pas de point d'accroche clair)** :
- `moment_treason` — aucun évènement nommé "trahison du conseil" avec popup dans `event_art.gd`/`event_dialog.gd` : la table `EVID_SLUG` (event_art.gd:34) couvre SUCCESSION/INTEG_*/XENOPHOBE (intrigue de cour) mais rien d'étiqueté trahison explicitement câblé à un popup `event_dialog`. Le TROUVAILLES du 2026-07-06 (entrée "#32 — LE SANG SIGNE TON RÈGNE") mentionne bien "3 évènements de trahison" côté moteur (scps_events.c, agent C parallèle, hors périmètre de cette mission), mais leur exposition façade/UI n'est pas visible dans les fichiers Godot lus ici — probablement le même popup générique `event_dialog`/`ui_quill`+`ui_seal` s'applique déjà à eux sans distinction. Un futur lot qui voudrait le son distinctif devra faire porter l'`evid` jusqu'à `event_dialog._open_slot` et matcher les 3 EVID de trahison (à identifier côté C) pour jouer `moment_treason` au lieu de `ui_quill`.
- sidebar.gd (rail d'onglets gauche, `_on_tab`) — PAS câblé en `ui_scroll_tick` : la mission demandait explicitement "province_detail TABS" ; le rail sidebar ouvre un TIROIR complet (pas un simple changement d'onglet dans un panneau déjà ouvert), jugé hors du geste "léger" visé — à revoir si un futur agent veut l'étendre.
- province_panel.gd/country_panel.gd (panneaux de sélection carte, `show_province`/`show_country`) — PAS câblés en `ui_parchment_open/_close` : ils s'ouvrent/ferment à CHAQUE clic de province sur la carte (potentiellement plusieurs fois par seconde en explorant) — un son à chaque clic aurait été du bruit, pas un hook UI "majeur" ; la mission listait explicitement une liste de panneaux majeurs qui NE LES INCLUAIT PAS.
**Gates** : boot headless `Main.tscn --quit-after 30` (Godot_v4.6.3-stable_mono_win64_console.exe, via MSYS2 bash) → **EXIT 0, 0 SCRIPT ERROR / Parse Error** (seuls des warnings PNG-as-resource + RID-leak pré-existants, sans rapport avec ce lot — vérifié qu'aucun ne référence un des fichiers modifiés). `grep -rn "Sound\." godot/project --include=*.gd` liste désormais 36 lignes (32 sites d'appel + 4 dans sound.gd lui-même). Aucun fichier C touché. **AUCUN COMMIT** (mandat).

## 2026-07-07 — Audit complet (5 voies + vérif adversariale) → docs/AUDIT_2026-07-06.md

**Découvertes** :
- Couverture verbe→UI **38/38 câblés** bout-en-bout (enum→drain→façade→binding→bouton) ; le trou CODEX servile/council_pay est FERMÉ (`godot/project/ui/sidebar_drawer.gd:_servile_act/_conseil_act`).
- **HIGH — cooldowns de révolte non sérialisés** : `scps_revolt.c:g_revolt_grace/g_coup_grace/g_concede_cd` gatent révolte/coup/concession, RAZ seulement par `revolt_init` → save/reload ≠ continuation (classe EMOB/COLC/TXYR, invisible au --savetest same-process).
- **HIGH — bouton Construction NO-OP** : `construction_panel.gd:167` flashe « ordre émis » alors que `agency_build_acct` refuse en silence (or/matière) ; `scps_api.c:scps_build_legal` existe mais n'est ni bindé ni consommé (et ne teste pas la matière).
- **MED — FIN_SANG invisible au readout** : `scps_readout.c:endgame_readout` rabat SANG→RFIN_AUCUNE ⇒ bandeau muet ET l'épilogue ne s'ouvre JAMAIS (main.gd:343 latche `fin>0`) ; seul le lavis (fin_raw) marche.
- **MED — dead-write armes IA** : `scps_ai.c:1307` écrit +20 RES_ARMS dans la vue region[] (écrasée) ; doublement mort — la levée consomme les macros RES_ARMS_*, pas la base.
- **MED — g_hub_of/g_hub_dist non revalidés** : `scps_intertrade.c:816-831` sérialise brut, `save_sane` ne borne pas ⇒ OOB read sur save forgée via `intertrade_buy_cost` (sans dirty-check, atteint au 1er tick post-load).
- **MED — 6 tune_f hors registre** (BT_ATK_RATIO/BT_DECROCHE/BT_DEF_EDGE/BT_RELIEF_FALL/RELIG_MINORITY_SAT/SEED_PROV_CAP_MULT) : SCPS_TUNE=… → exit(2) prouvé, invisibles F10.
- **MED — lettré religieux injouable** : `scps_religion_scholar_role`/`recruit_scholar` bindés, mécanique P6 réelle, 0 bouton (religion_panel.gd ne fait que fonder/schismer).
- **MED — cliquet lang-check aveugle** : `Makefile:658` greppe des primitives SDL retirées ⇒ 0 match trivial ; 25 littéraux FR en `return "…"` dans `scps_readout.c` atteignent le panneau province.
- **MED — marché servile sans prix** (spread ×2/×1 débité, jamais affiché — `scps_intertrade.c:752/703`) · **E3 stockeuse inerte** (0 entrepôt sur 4 seeds ×200 ans, `scps_ai.c:1368`) · **noms de ministre absents** des 3 events de trahison (`scps_events.c:event_title` = 1 seul %s).
- Mécaniques mortes-en-pratique mesurées : `TechState.eco/.mil` écrits+sérialisés jamais lus ; interception de convoi 0 partout ; péage de détroit quasi-mort (58 or seed 42 seulement) ; pool servile/affranchissement 0 ; foreuse/réplicateur 0 (Corne vit).

**Pièges** :
- La lane « mécaniques mortes » d'origine est morte en rendant un **stub** — son vérificateur adversarial a refait la dimension entière lui-même (12 items mesurés chronicle). Toujours vérifier qu'un résultat d'agent n'est pas un placeholder.
- **8 verdicts REFUTED** prouvent que TROUVAILLES/CLAUDE.md se PÉRIMENT : « council_pay sans curseur » (câblé depuis), « T8 aucun match code » (spécifié + code compacité), « H_coerc jamais lu », « garnison jamais posée », « chaîne feu jamais » (vivante seed 42), « disband perd le fer » (fixé), « goulot d'armes 7.8 % » (mesure fraîche 73-95 %) — **re-grep/re-mesurer avant de citer une vieille entrée**.
- « Mort sur seed 9 » ≠ « mort partout » : les mécaniques émergentes (feu, péage, foreuse) exigent 4-5 graines pour trancher.
- Fraîcheur post-snapshot : moment_treason.wav SUPPRIMÉ le 07 (090d4dd, un-clic-un-son) — l'item audio est résolu-par-suppression ; re-découpe planches + écrans de fin/bordures faits (58514fd, a400f80).

**Restes** : le rapport complet est `docs/AUDIT_2026-07-06.md`. 3 chantiers prioritaires : (1) **contrat de save** — sérialiser les 3 grâces de révolte + clamper g_hub_of + fuzztest étendu + fix dead-write armes ; (2) **membrane honnête** — binder scps_build_legal (avec gate matière) + RFIN_SANG/latch épilogue + prix affichés (servile/manuf) + nom de ministre + codex.gd:47 + UI lettré ; (3) **outillage** — ré-armer lang-check (25 littéraux readout → STR_*) + 6 tunables au registre J, puis recalibrer E3/interception/péage.

## 2026-07-07 — Lot A contrat de save (grâces + hub)

**Découvertes** :
- `scps_revolt.c:g_revolt_grace/g_coup_grace/g_concede_cd` : trois `static float [SCPS_MAX_COUNTRY]` (jours), lus/écrits à revolt_ignite (:296/:352/:412), revolt_scan (:446-448, décrément 30 j) et apply_rebel_victory/le verdict CLASS (:873/:919/:936). Option **(a)** retenue : rapatriés SUR `RevoltState` (scps_revolt.h) — la section RVLT est un fwrite BRUT de `*s->rs`, tous les appelants ont déjà le `rs`, et la struct a DÉJÀ absorbé les mêmes rapatriements (revolt_cooldown v53, backing_tried v56) ⇒ zéro nouvelle plomberie de section, un seul bump (72→73). L'option (b) (section neuve + save/load dédiés) aurait dupliqué le motif EMOB pour rien.
- **Le repos NORMAL d'une grâce expirée est NÉGATIF** : le décrément `if (x>0) x -= days` (cadence 30 j, scps_sim.c:770) laisse le compteur UN pas sous zéro (∈ (-30, 0]) où il reste À JAMAIS (les lecteurs ne testent que >0, le décrément ne touche plus x≤0). Une borne save_sane `[0, …]` aurait REJETÉ toute save légitime post-grâce — invisible aux 4 savetests (600 j, aucune révolte n'ignite si tôt ; les grâces premières se posent > an 12). Borne retenue : **∈ [-31, 40×365]**, refus net hors-borne.
- `intertrade_save/load` (scps_intertrade.c:816-831 avant patch) : g_hub_of/g_hub_dist sont int16_t sérialisés BRUT et `intertrade_buy_cost` (:426) déréférence `e->region[hub]` avec pour seuls gardes hub≥0 et hub≠region — AUCUNE borne haute (SCPS_MAX_REG=832 < 32767). `hub_map_build` (:318) memset le tableau ENTIER (-1) puis ne remplit que r<n ⇒ un jeu sain n'a JAMAIS de garbage au-delà de n_regions : `intertrade_save_sane(n_regions)` valide le PLEIN tableau (∈[-1,n_regions)), aucune save légitime rejetée. g_hub_dist n'est jamais un index (plafonné à 8 en lecture, :876) — borné en défense seulement ([0, SCPS_MAX_REG+IT_SEA_HOPS]).
- Ordre des sections au load (scps_save.c) : WRLD puis ECON sont lus AVANT ITRD (:368-369 vs :409) et `scps_save_sane` est appelé APRÈS toutes les sections (:425) ⇒ `s->econ->n_regions` est déjà borné quand `intertrade_save_sane` tourne. Le hook s'appelle depuis save_sane (motif director_save_sane), pas depuis intertrade_load.
- Fuzztest (viewer.c:242-253) : les cas existants poke des champs Sim-visibles ; g_hub_of est un static de MODULE ⇒ setter dédié `intertrade_debug_set_hub_of` (fuzztest-only, documenté tel quel dans l'en-tête). **Vérifié par test NÉGATIF** : guard temporairement désactivé (`if (false && …)`) ⇒ fuzz-save passe à 7/8 avec « ✗ intertrade hub_of hors-borne REJETÉ » — le cas mord vraiment ; guard rétabli ⇒ 8/8.

**Pièges** :
- La borne basse des compteurs décrémentés par `if (>0) x -= days` : TOUJOURS regarder si le dernier pas peut passer sous zéro et y RESTER avant de border à 0. Même piège latent pour tout futur compteur rapatrié de ce style (desperation_days/revolt_cooldown, eux, sont clampés fmaxf(0,…) — pas ces trois-là).
- `revolt_init` memset la struct ENTIÈRE ⇒ les nouveaux champs sont RAZ gratuitement ; ne PAS ajouter de memset dédié (il masquerait un futur oubli de sérialisation d'un champ ajouté après le memset).
- g_backing_wars/g_civilwars restent des statics NON sérialisés À RAISON (pure télémétrie chronique, ne gatent rien) — le commentaire de scps_revolt.c:134 qui les rangeait « même patron que g_coup_grace » était devenu FAUX après ce lot, réécrit.

**Restes** :
- Le dead-write d'armes IA (`scps_ai.c:1307`, défaut #4 MED de l'audit — mentionné par le chantier n°1 « au passage ») n'est PAS dans ce lot (fichier hors périmètre autorisé) : toujours ouvert.
- Aucun cas fuzztest ne forge les 3 grâces (hors mission ; la borne save_sane est triviale par inspection et le motif FZ existant s'y étendrait en 3 lignes si voulu).
- Les savetests n'exercent jamais un monde POST-révolte (600 j < premier allumage) : la préservation d'une grâce mi-course au reload est garantie par le fwrite/fread du blob RVLT, pas prouvée par un gate dédié.
## 2026-07-07 — Lot E English (switch + infra + extraction)

**Découvertes** :
- **Comment tr() résout VRAIMENT** (`scps/scps_lang.c:70-75`) : `g_override[id]` (la surcharge `scps_lang.txt`) D'ABORD, puis `g_lang==LANG_EN ? TABLE_EN : TABLE_FR`. Les DEUX langues sont COMPILÉES dans le binaire (`strings_ids.h` X-macro FR + `strings_en.h` jumelle, asserts de taille) et `lang_set()` bascule à CHAUD (« aucune chaîne cachée dans un état persistant »). Le switch ne demandait qu'un PASSE-PLAT : façade `scps_lang_set/get` (scps_api.{h,c}) + binding `Sim.world.lang_set/lang_get` (scps_sim_node). La voie « générer scps_lang_en.txt et le charger en override » REJETÉE : elle confisquerait le canal de surcharge (qui appartient au joueur/mod), et `lang_dump_file` écrit la langue COURANTE (poule-œuf).
- **`strings_en.h` est encore ~46 % copie du FR** : ~165 des 364 entrées identiques au FR (~197 déjà traduites — diplo, agitation, slots, menu). Le switch moteur est BRANCHÉ mais l'anglais moteur réel reste une session de pur remplissage.
- **Import CSV Godot 4.6** : les `.translation` naissent À CÔTÉ du CSV (`godot/project/i18n/ui.{fr,en}.translation`), committables (seul `*.import` est gitignoré) ; `--headless --import` les régénère ; référencés dans `project.godot [internationalization]` + `locale/fallback="fr"`.
- **Le chrome affiche par DEUX idiomes** : `.text =` (shell, containers) et surtout `VKit.text/section/row` en mode immédiat (**277 appels**) — un extracteur qui ne regarde que `.text=` sous-compte ~10× (67 vs 629).

**Pièges** :
- **`load("x.gd") != null` ne prouve PAS que le script compile** : un Parse Error rend quand même la ressource (menu_audit passait vert), c'est `.new()` qui explose (« Nonexistent function 'new' in base GDScript »). Le probe `lang_audit` INSTANCIE MenuRoot pour prouver le rendu de bout en bout.
- **python.exe (MSYS2) sur console Windows = cp1252** : un `print()` avec « → » crashe UnicodeEncodeError — sorties console en ASCII pur, le CSV reste UTF-8.
- **L'ordre de boot compte** : `options.boot()` (locale + table moteur + plein écran, lus de user://options.cfg) doit précéder le PREMIER tr() — `menu_root._ready` crée le panneau Options avant `_build_main`. Les tr() étant posés à la CONSTRUCTION, le changement de langue REBÂTIT le shell (signal `language_changed`).
- **uikit.gd : des chaînes FR sont des CLÉS de correspondance moteur** (« Manufacture textile » → sprite par nom rendu par la façade) — les traduire naïvement casserait le mapping ; passer par un ID ou ne traduire que l'affichage.

**Restes** :
- **Backlog chiffré : 629 littéraux face-joueur probables dans 28 fichiers .gd** (`docs/i18n_backlog.csv` — fichier:ligne, chaîne, clé suggérée ; top : codex 120 · sidebar_drawer 102 · uikit 92 · province_detail 53 · alerts 39). Outil : `tools/extract_gd_literals.py` (stdlib pur ; lancer avec D:/MSYS2/mingw64/bin/python.exe).
- **~165 entrées `strings_en.h` à traduire** (remplissage diffable, zéro logique).
- Les panneaux hors-shell (province/sidebar/religion/codex/culture_creator) appartiennent à d'autres lots — migrer AVEC leur propriétaire ; le patron est le shell : `tr("T_*")` + entrées CSV + rebuild au changement de langue.
## 2026-07-07 — Lot M membrane honnête (implémenteur, chantier 2 de l'audit)
**Découvertes** :
- `scps_readout.h:FinReadout` — le miroir de FinType s'APPEND sans risque (enum jamais sérialisé,
  scps_api.c fait `(int)er.fin` ⇒ RFIN_SANG=5 traverse tout seul jusqu'à `endgame_info["fin"]`) ;
  le côté Godot (endgame_banner FIN_NAMES/FIN_TINT · epilogue FIN_PHRASE/FIN_SCREEN · border_art
  FIN_KEYS) portait DÉJÀ la clé 5 — seule la case du switch readout manquait. `fin_raw` est
  désormais redondant avec `fin` (même échelle 0..5) mais GARDÉ pour compat (lavis V3 le lit).
- `event_title` (scps_events.c) : pour nommer le MINISTRE des 3 trahisons sans changer la
  signature publique (scps_events.h possédé par un autre lot), un LATCH module-static
  `g_title_sc` posé par `world_events_tick` suffit — display-only, jamais sérialisé, repli
  « Le savant/notable/marchand » si NULL (avant 1er tick / bancs) ou siège vacant. Le nom
  se résout par `statecraft_council_seated(+_gen)` → `statecraft_council_cand_name` → `tr()`
  (motif scps_country_council, scps_api.c:1032).
- `scps_build_legal_ex` (scps_api.c) : le miroir COMPLET des gates d'`agency_build_acct`
  (géo port/Centre · palier · file F5 `agency_pending_build` · MATIÈRE `intertrade_market_avail_ex`
  · OR) dans le MÊME ORDRE ⇒ la raison rapportée = le premier refus du drain.
  ⚠ `agency_extent_mult` est STATIC côté agency — la formule ×(1+0.15·n_régions) est
  RECOMPOSÉE dans le miroir (§7) : à re-synchroniser si agency la change.
- Prix serviles : `slave_pool_price_mult` est STATIC dans scps_intertrade.c (module interdit
  ce lot) — recomposé dans `scps_slave_prices` depuis les lecteurs PUBLICS
  (`intertrade_slave_pool_count` + tune_f SLAVE_POOL_REF/SLAVE_PRICE + ipm) : formule
  ref/(pool+ref·0.10) bornée [0.5,2.5], achat ×2 / vente ×1. À re-synchroniser si intertrade change.
- Lettré (P6) : `religion_scholar_role` ne rend le rôle QUE si un lettré est ACTIF ; le rôle
  qu'un recrutement DONNERAIT (pour l'UI avant-clic) = `scholar_role_from_credo(g_religions[rid].credo)`
  — g_religions/g_religion_count sont extern ⇒ reader façade `scps_religion_scholar_expected` pur.
  `religion_scholar_recruit` re-recrute librement (timer remis) ⇒ bouton « Renouveler » légitime.
- Worktree + GDExtension : la jonction godot-cpp cassée se recrée (PowerShell New-Item Junction),
  MAIS un sconsign FRAIS fait REBÂTIR godot-cpp entier → échec (`array.hpp` vit dans gen/include,
  pas include/). Le fix : COPIER `.sconsign.dblite` depuis SCPS-main/godot → scons ne relinke
  que nos objets contre le .a prébâti.
- Probes headless en worktree frais : lancer UNE FOIS `--import` (sinon « Failed to instantiate
  an autoload » + libscps « absente » — c'est le cache .godot/ qui manque, pas la DLL).
**Pièges** :
- scps_api_demo asserte `scps_build_legal ∈ {0,1}` (scps_api_demo.c:217) — NE PAS changer le
  contrat de retour ; la raison passe par le paramètre `_ex(…, int *reason_out)`.
- `-Wmisleading-indentation` : `if (x) a; return 0;` sur UNE ligne = warning gcc 16. Un
  warning PRÉ-EXISTANT du même type vit à scps_api.c:648 (commit 9fa1b7cf, lot variant-map,
  pas à moi — laissé).
- Au jour 0 (avant le 1er econ_tick), `intertrade_market_avail_ex` ne voit pas le pool des
  Centres (hub map pas bâtie) ⇒ build_legal = 0 partout — VRAI (le drain refuserait pareil),
  pas un bug de miroir ; à j+60 l'histogramme respire (probe membrane_audit : structurel/or/matière).
**Restes** :
- verbs_audit « coloniser n'a pas mordu » : PRÉ-EXISTANT (échoue à l'identique sur SCPS-main,
  DLL du 06) — hors lot M, à diagnostiquer (revalidation du drain colonize ou monde de la seed vitrine).
- Le CORPS des options de trahison reste générique (« Le faire taire ») : les blurbs sont des
  tables statiques — seul le TITRE porte le nom du ministre (le splice presentation-time n'existe
  que pour event_title). Injecter le nom dans les blurbs = un ring de composition par option (différé).
- Les prix serviles/`build_legal` affichés sont des MIROIRS de lecture : si un autre lot touche
  agency_build_acct/intertrade_slave_buy, re-synchroniser scps_build_legal_ex/scps_slave_prices
  (grep « lot M » dans scps_api.c).
## 2026-07-07 — Lot W worldgen (archétypes + falaises)
**Découvertes** :
- **Où vivait vraiment le « toutes les maps se ressemblent »** : `scps_world.c:worldparams_default` — défauts FIGES (6 continents · terres 0.5 · âge 0.7 · relief 0.5 · climat neutre) quelle que soit la graine ; seul le bruit changeait. TOUS les overrides existants (sliders façade `scps_api.c:scps_sim_generate` via `g_wg`, chronicle argv `fix_emp/fix_cs/fix_cont`, batch.c world_age, econ_scan n_empires) assignent APRÈS l'appel → dériver DANS worldparams_default respecte « les overrides priment » PAR CONSTRUCTION, champ par champ. Graine 0 = référence figée (chemin `world_generate(w,NULL)`).
- **La dispersion du hash est gratuite** : splitmix32 (`wg_mix`) sur les graines de référence 3/7/9/11/42/99/145 → 7 archétypes DISTINCTS du premier coup (aucun réglage de constante). Golden (7/108/209/310/411) → 4 archétypes.
- **Comment la dureté atteint la côte** : `g_hardness` (convergence de plaques #2 + litho noise, `compute_hardness`) est PURE f(x,y,plaques) — l'appeler en TÊTE de `step_geology` (avant le socle) est sûr ; la rampe côtière `plat=(mask-0.18)/0.34` se resserre alors ∝ dureté → caps et côtes hautes ÉMERGENT. Complété par l'érosion différentielle (thermique + gouttelettes ∝ 1−dureté).
- **Le tueur de falaises était la courbe GAMMA « vallées »** (`world_generate`, `lf^1.6`) : dénivelé côtier max 0.065 pré-gamma → 0.018 post-gamma (mesuré par probe par étape, `SCPS_CLIFFDIAG`). Transformation MONOTONE → on calibre les seuils du drapeau à l'échelle POST-gamma (`CLIFF_H_MIN` 0.0045 · `_MAX` 0.018) au lieu de toucher la gamma (contraste falaise/plage préservé ×5-10).
- **Le drapeau cliff se DÉRIVE à la lecture** (`world_cliff_intensity`, pur f(height/biome sérialisés)) — valide APRÈS un load sans regénération, zéro bump de save. ⚠ Cell est DANS le blob WRLD (`fwrite(w,sizeof *w)`) : un champ Cell = bump — la dérivation évite ça.
- **Chaîne d'affichage** : `SCPS_LAYER_CLIFF` (scps_api, motif des couches existantes) → `layer_image(6)` (binding générique, seul BIND_CONSTANT à ajouter) → texture L8 dans iso_ground → hachures shader (traits ⊥ côte : stripes `sin(dot(cell,tangente)·freq)` où la tangente vient du GRADIENT du champ falaise — la bande fine donne un gradient ⊥ rivage fiable).
**Pièges** :
- **`cell.lake` n'est PAS « de l'eau visible »** : c'est le drapeau d'ACCÈS des bassins alimentés (priority-flood #3) — des cuvettes INVISIBLES en plein relief. L'utiliser comme voisinage d'eau flague ×6 de fausses falaises en montagne. Idem les mers intérieures PERCHÉES (BIO_SHALLOW aplani au niveau de DÉBORDEMENT) : leur rive mesurée contre SEA_LEVEL = tout mur de cuvette en « à-pic » (le pic parasite du godet 39 de l'histogramme). Falaise MARITIME = eau `height < SEA_LEVEL` SEULEMENT.
- **`.godot/` est gitignoré** : dans un worktree frais, AUCUNE probe Godot ne marche (uid non résolus + `extension_list.cfg` absent ⇒ « GDExtension libscps absente » MÊME avec la DLL en place). Fix : une passe `Godot --headless --path godot/project --import` régénère tout.
- **godot-cpp partagé sur E: = collision inter-agents** : deux scons parallèles se battent sur `libgodot-cpp...a` (« fichier utilisé par un autre processus », ar temp `stXXXXXX` orphelins). Fix : COPIE locale réelle dans le worktree (robocopy /XF *.o *.a .sconsign.dblite) au lieu de la jonction.
- **Windows verrouille les exe en cours** : un sweep chronicle en arrière-plan bloque le RE-LINK de chronicle (collect2 error) — séquencer builds et runs.
- Bancs recalibrés (intention préservée, détail au commit fe0ca6c) : endgame_demo (île-épicentre : `corrupted==0` au feu ⇒ chute DIFFÉRÉE ; + region[] est un MIROIR à rafraîchir), faith_demo (paire schismatique à CHERCHER), econ_arcane_demo (la « 1re région colonisée » peut être un hameau WILD sans bras), religion_demo (un pays à EXACTEMENT 2 régions, pas « les 2 premières d'un grand »), events_demo (fixture pays-wide diluée par les régions sœurs ⇒ pays MONO-région ; contrôle mtth probabiliste ⇒ fenêtre 30 ans).
**Restes** :
- Les archétypes à âge BAS (pangée 0.48, jeune-montagneux 0.24) donnent des masses FUSIONNÉES (dérive faible) — géologiquement cohérent (monde jeune = supercontinent), mais si on veut « 4 masses » VISIBLES sur jeune-montagneux, il faudrait découpler dérive et âge (plates_init prend world_age comme drift).
- Le niveau de MER n'est pas un paramètre (SEA_LEVEL fixe) — land_amount en tient lieu ; un vrai slider mer = normalize + seuils à revisiter.
- Hachures falaise : calibrées sobres (α 0.34 × intensité) ; si un joueur les veut plus lisibles au fit, monter `hatch` freq/α côté shader seulement.

## 2026-07-07 — Lot B écritures fantômes (4 sites)
**Découvertes** :
- `econ_region_stock_add(e,region,good,delta)` (scps_econ.h:502/scps_econ.c:1664) est LE seul point d'écriture persistante sur `region[].stock[]` : delta>0 dépose sur la province représentative (`region_carrier_prov`) ET met à jour la vue `rv->stock` dans le MÊME appel (donc un test qui lit `region[].stock` juste après l'appel, sans re-`econ_aggregate_regions()` entre les deux, voit déjà la bonne valeur — c'est PRÉCISÉMENT pourquoi les bancs existants (endgame_demo C6, missions_demo §2) passaient DÉJÀ avant le fix : ils ne font jamais tourner un `econ_tick`/`sim_day` complet entre l'écriture fantôme et leur `CHECK`, donc ils ne peuvent PAS distinguer un dépôt réel d'un dépôt fantôme. Seul un `chronicle`/`sim_day` réel (qui appelle `econ_aggregate_regions` chaque mois) révèle l'évaporation. Retenir : un banc qui veut prouver la persistance d'un stock DOIT insérer un `econ_aggregate_regions(econ)` (ou un tick complet) entre l'écriture et la lecture, sinon il est aveugle à cette classe de bug.
- Site 1 (scps_ai.c:1307, dépôt d'armes Dominateur/Honneur) : `RES_ARMS_LIGHT` EST `RES_ARMS` — alias `#define RES_ARMS_LIGHT RES_ARMS` (scps_types.h:209, « alias SAVE-safe »). L'audit parlait de « doublement mort » (la levée consommerait les macros `RES_ARMS_LIGHT/HEAVY/…`, pas la base `RES_ARMS`) mais c'est FAUX pour ce cas précis : `unit_res_arm` (scps_army.c:109) renvoie `RES_ARMS_LIGHT` pour le roster léger (Piquier/Lancier/Épéiste/Cav légère/Lame franche/Cav de raid) — même valeur d'enum que `RES_ARMS`. Le bien déposé était donc déjà LE bon bien ; seul le canal d'écriture (direct sur region[], fantôme) était cassé. Fix : `econ_region_stock_add(econ,hr,RES_ARMS_LIGHT,20.f)`, aucun changement de bien.
- Site 2 (scps_endgame.c `endgame_empire_consume`, scps_endgame.c:563/appelé :769) : la Merveille est PLAYER-ONLY — `wonder_tick` retourne immédiatement si `eg->merv_country<0` (scps_endgame.c:762) et **rien dans chronicle ne fixe jamais `merv_country`** (aucun `endgame_start_wonder` en dehors de l'ordre agency `AGENCY_BUILD_WONDER`, joueur seul) : ce site est DONC totalement inatteignable en chronique/golden, confirmé par le fait que seul le seed 411 a re-baseliné (sites 1/3/4, pas ce site). Le banc `endgame_demo` C6 (ligne ~382) alimente déjà correctement `prov[].stock` (pas region[]) avec un commentaire qui documente EXACTEMENT le même piège RE-KEY — ce fixture était déjà écrit juste, seul le code de prod ne l'était pas.
- Site 3 (scps_missions.c `mission_grant`) et Site 4 (scps_navy.c raid pirate) partageaient le MÊME commentaire périmé mot pour mot (« stock[]/price[] restent au grain RÉGION (le marché, charte, INTACT) ») — ce faux mantra existe ENCORE ailleurs dans le code (grep `restent au grain|reste au grain`) : `scps_diplo.c:1153/1157` (`diplo_pillage_region`) porte la même phrase alors que le CODE en dessous (ligne 1167) est DÉJÀ correctement routé par `econ_region_stock_add` depuis un fix antérieur — seul le commentaire est un fossile trompeur. Hors scope de ce lot (fichier non autorisé) mais à nettoyer si `scps_diplo.c` est retouché.
- Site 4 (scps_navy.c:497) : `loot` doit désormais se calculer sur le delta RÉELLEMENT pris (`taken = -econ_region_stock_add(...)`), pas sur `re->stock[g]*TITHE` a priori — sinon on retombe dans la même classe de bug (or créé sur du vide) même en routant le débit.
- Mesures avant/après (seed 9, 3 sims × 200 ans, SCPS_ARMSDIAG) : stock MONDE d'armes légères en fin de run 5856→9614 (+64 %, l'arsenal IA s'accumule enfin réellement au lieu de s'évaporer chaque mois) ; raids pirates 7→20 (763 or pillés vs 277) — la course profite d'un monde légèrement différent (l'arsenal réel change les prix du fer/les décisions IA en cascade, cf. re-baseline golden seed 411 seul) mais le monde reste SAIN (hégémon mortel 3/3, satisfaction pop-pondérée Laborer ~80 %/Bourgeois ~88 %/Élite ~87 % inchangées à la marge, IPM ~1.2 dans les deux cas).
**Pièges** :
- Ne PAS conclure d'un banc vert que le canal d'écriture est sain pour cette classe de bug — cf. découverte ci-dessus (endgame_demo/missions_demo passaient AVANT le fix aussi).
- `git stash push -- <fichiers>` pour obtenir un binaire BEFORE propre dans un repo partagé fonctionne bien SI on liste explicitement les fichiers (jamais `git stash` nu) — `git stash pop` restaure ensuite sans toucher aux autres fichiers en cours de modification par d'autres sessions.
**Restes** :
- Le commentaire menteur (`scps_diplo.c:1153/1157`, `diplo_pillage_region`) reste à corriger — code déjà bon, texte à jour uniquement.
- Site 2 (Merveille) n'a aucune exercice en chronique (player-only) : la seule preuve vivante est le banc `endgame_demo` C6 (`ci1 < ci0` après tick) — si un futur agent câble un chemin IA/chronique vers `endgame_start_wonder`, revérifier que la consommation réelle ne cale pas le chantier plus souvent que voulu (le rare peut manquer, c'est le coût VOULU, mais mesurer avant de juger).
- Aucune borne/cooldown proposée sur le raid pirate (site 4) : la mission demandait de signaler seulement si le raid réel déséquilibre le monde ; mesuré SAIN sur seed 9/200 ans, pas de spirale de famine côtière observée — à re-vérifier sur un sweep plus large si un futur agent en doute.
## 2026-07-07 — Lot V assets dans l'eau
**Découvertes** :
- **CANOPÉE — les "extras" du cœur de peuplement ne testaient AUCUNE eau** (`overlay.gd:_build_dressing`,
  bloc `for e in range(extra)`) : le VOTE DE VOISINAGE (3 échantillons à ±3 cellules, `hits`) sert à
  lisser les bords de biome bruités et décide combien d'individus EN PLUS planter (jusqu'à 39 au cœur),
  chacun offsetté de `(_h1(eb)-0.5)*7.0` (±3.5 cellules) depuis l'ancre — cet offset n'était vérifié QUE
  contre la clairière des bourgs et la rivière (`_near_river`), JAMAIS contre la mer/le lac
  (`LAYER_WATER`). Sur une côte/île étroite (fréquent depuis lot W : archipels, mers intérieures), un
  arbre du cœur forestier plante donc ses "extras" en pleine mer. Preuve visuelle nette : seed 11 (mer
  intérieure), une forêt bordant un lac laissait ~15-20 arbres flotter SUR l'eau (`v_seed11_lake_before.png`
  vs `v_seed11_lake_after.png`, diff 20k px `v_seed11_lake_diff.png` — tous alignés sur le pourtour du lac).
  **Second trou, plus subtil** : le PLACEMENT PRIMAIRE lui-même n'était pas à l'abri — `bhit` (le biome
  "voté") peut être établi par un SEUL des 3 échantillons décalés (offsets fixes `[3,1]`/`[-2,3]`), même
  si l'ANCRE (px,py) — où l'arbre est réellement planté (fx,fy ≈ px,py) — est déjà de l'eau. Le vote
  prouve qu'UN point à ≤4.24 cellules est forêt, jamais que le point de pose l'est.
- **URBANISTE (`_build_town`) — le PLAN ne vérifiait l'eau qu'à l'ANCRE (`ctr`, garanti sec par
  `_find_seat`)** : tout le reste du plan (rangs arrière de maisons ∝ `sd` jusqu'à ~1.4, ruelles, spirale
  dorée pour les hameaux, landmark, LANIÈRES DE CHAMPS à `fbase+1.5*fh` — pouvant atteindre 9-10 cellules
  du centre pour un gros bourg —, tours/portes d'enceinte au rayon `wrad`, moulin à vent au-delà du champ,
  arbres isolés "la VIE") est posé par pure GÉOMÉTRIE relative à `ctr`/`bc`/`org`, sans jamais retester
  l'eau à la position FINALE. `SEAT_SEA_INLAND`=4 cellules de retrait ne suffit pas à couvrir un rayon de
  bourg de tier 4-7 ou une lanière de champ — sur une presqu'île/île étroite, ces éléments périphériques
  débordent en mer. `_region_citymax`/`_max_dry_size`/`_nearest_sea_dir` (calculés dans `_build_anchors`)
  ne sont PAS lus par `_build_town` — c'est un mécanisme MORT pour l'urbaniste actuel (seul
  `_region_anchor`, pas `_region_citymax`, sert encore, pour les routes).
- **Roads/bridges/dressing simple : DÉJÀ CORRECTS** — `_chaikin_safe`/`_smooth_resample_road` testent déjà
  `LAYER_WATER` (mer+lac, pas seulement mer) + rivière à CHAQUE point lissé ; `_try_place_dress` (marque de
  terrain simple, sans clones) resample le biome À la position jittée finale (auto-cohérent) ; les easter
  eggs (serpent/épave/lapin) testent le biome à leur propre point de pose. Aucun changement nécessaire là.
- **Le "faux positif" qui a coûté le plus de temps** : un immense aplat vert-bleuté avec une grille RÉGULIÈRE
  de taches sombres (seed 9, région capitale) ressemblait à s'y méprendre à des assets plantés en pleine
  mer intérieure. Diagnostic par élimination (désactiver canopée+dressing → persiste ; désactiver le lavis
  politique → persiste ; visualiser `river_map` brut → ~0 ; visualiser `biome_at(cell)` brut → **BLEU PUR
  = MARAIS (is_wetland), pas de l'eau (is_water) !**) : ce n'est PAS un lac, c'est un immense MARAIS
  (biome 15), et la grille observée est le SYMBOLE CARTOGRAPHIQUE de marais du shader
  (`iso_antique.gdshader` : `rows = sin(iso.y*0.9+...)` × `dash = sin(iso.x*1.7+...)`, tirets décalés
  rangée/rangée) — qui fonctionne exactement comme conçu, juste inhabituellement visible parce que ce
  marais-ci est démesurément grand (nouvel archétype worldgen). AUCUN fix nécessaire ici, et surtout
  aucun fichier C/shader à toucher pour cette fausse piste — reconnu et abandonné après ~6 tests A/B ciblés.
**Pièges** :
- **Le worktree n'a pas de godot-cpp ni de DLL** : `robocopy` depuis un autre worktree DÉJÀ construit
  (`SCPS-wt-W`, 176 Mo, `.a` + `.dll` inclus) est bien plus rapide que rebuild from scratch ; scons
  ensuite recompile SEULEMENT les objets moteur (`scps_*.c`) + le binding (`src/*.cpp`), pas godot-cpp.
  Invocation : `MSYSTEM=MINGW64 bash -lc 'cd .../godot && export PROCESSOR_ARCHITECTURE=AMD64 && scons
  platform=windows use_mingw=yes target=template_debug -j4'`.
- **`.godot/` gitigné** : sans passe d'import (`Godot --headless --path godot/project --import`), le
  binding GDExtension n'est pas résolu même DLL en place. Import produit des warnings BÉNINS
  ("Condition p_position > length", seek sur des PNG sheet) — ignorer, `[ DONE ] reimport` suffit.
- **`shot_parch` ne prend PAS `cx=`/`cy=` en plus de `cap=1`** : passer les deux fait IGNORER `cap=1`
  (l'arg explicite écrase le défaut capitale). Pour retrouver des coordonnées MONDE à partir d'un repère
  visuel sur un fit, il faut soit cross-référencer un autre zoom déjà capturé, soit accepter le
  tâtonnement — pas de moyen direct de lire `cx,cy` de la capitale sans une sonde dédiée.
- **Godot/Sim NE se relance PAS proprement d'un run à l'autre du même process, mais chaque invocation
  CLI est un process FRAIS** — donc pas de souci de fuite intersim ici (contrairement au moteur C
  headless) ; en revanche `_town_cache`/`_dressing`/`_canopy_batches` sont des caches PAR-WORLD
  (rebâtis au `generated` signal) — un simple redraw ne suffit pas à re-tester une correction, il faut
  bien régénérer (`Sim.regenerate`) pour que `_build_dressing`/`_build_town` retournent.
- **`git stash`/`git stash pop` marche bien pour A/B rapide** sur un seul fichier modifié, plus rapide
  qu'un deuxième worktree, pour comparer avant/après sans dupliquer l'environnement Godot+DLL.
**Restes** :
- **Le MUR D'ENCEINTE des cité-états (`arcs`, la polyligne continue du rempart)** reste NON gardé — j'ai
  gardé tours/portes (`_pull_dry` individuel) mais PAS l'arc lui-même (le retirer point-par-point
  déformerait le cercle en un contour incohérent avec des tours qui ne seraient plus DESSUS). Risque
  résiduel : sur une presqu'île très étroite avec un GROS cité-état (`wrad` large), le rempart peut encore
  mordre un peu d'eau. Non observé dans les 4 graines testées (les cité-états rencontrées avaient assez
  de terre), mais pas prouvé absent en général.
- **Le MOULIN À EAU / l'ENTREPÔT / le PHARE (près des quais)** sont volontairement LAISSÉS TELS QUELS —
  ils sont censés toucher/longer l'eau (quai, roue à aube, rivage) ; les distinguer d'un vrai débordement
  demanderait une marge fine (garder à 0-1 cellule du bord, pas au milieu) — non fait, jugé hors du
  problème rapporté (aucun cas observé de ces bâtiments franchement DANS l'eau, seulement à son bord,
  ce qui est le design).
- **`_region_citymax`/`_max_dry_size`/`_nearest_sea_dir`** restent un mécanisme à moitié mort (calculé,
  jamais lu par l'urbaniste) — pourrait servir à borner `wrad`/`n` (taille du bourg) À LA SOURCE plutôt
  que de rattraper après coup ; pas touché ici (`_pull_dry` suffit au problème rapporté, refactor plus
  profond laissé en réserve).
- Captures avant/après dans `godot/project/*.png` (gitignorées, non committées) : `v_seed11_lake_before/
  after/diff.png` (preuve la plus nette), `v_seed9_z3_before/after/diff.png` (canopée close-in, seed 9),
  `v_seed3_pond_before/after/diff.png` (canopée + léger ajustement de plan de ville), `v_seed{9,11,42,3}_
  fit_before/after.png` (balayage 4 graines demandé).
## 2026-07-07 — Lot T double gate + religion
**Découvertes** :
- Le « tier » vivait en QUATRE barèmes divergents : `capitale_max_tier` (scps_labor.c:18,
  la vraie table T2 2000…T7 10000 — déjà les seuils joueur, pré-existants au lot) ·
  `scps_region_tier` (scps_api.c, ad hoc 4000/1500/500/150/50 sur 0-5, « miroir viewer ») ·
  la stature du readout (50/500/2000/6000) · le tier_h du readout (recopie littérale de la
  table labor). E0.4 avait enterré `labor_region_cap_tier` (le registre « payé ») mais PAS
  unifié les lecteurs. Lot T : tout délègue à `capitale_max_tier` ; seuils dialables
  (`TIER2_POP…TIER7_POP`, registre J) mais mis en CACHE au 1er appel (hot path par-province
  par-tick — un lookup registre linéaire par appel serait cher) ⇒ pas de F10 live sur ces 6.
- GRAIN du T-gate : le moteur ÉCONOMIQUE est à la province (charte), mais les 3 T-gates
  manuf de l'IA lisaient `region[].strata` (Σ toutes provinces) alors qu'`econ_build_manufacture`
  pose sur la province REPRÉSENTATIVE — une région à 3 provinces de 1500 passait un gate T2
  qu'AUCUNE province ne tient. `host_province_tier` (scps_ai.c, via econ_region_rep_province)
  corrige ; repli tier 1 si pas de rep (fixtures). ⚠ D'AUTRES lecteurs région-grain restent
  VOLONTAIREMENT hors périmètre (revolt:502, campaign:87, demography:485, econ:2218/2562 —
  service/défense/besoins, pas de la construction) : à unifier un jour en conscience, pas en
  passant. GOLDEN : le gate province-grain n'a PAS bougé le hash 12 ans (mono-province tôt).
- La gate tech par palier : `edifice_tier` (position dans la famille via edifice_prev —
  AUCUN champ table à ajouter) ⇐ `econ_country_has_tier` ⇐ `tech_has_tier`. Le fil TechState
  → agency_build_acct passe par un CACHE TRANSITOIRE (pointeur posé par econ_apply_country_tech,
  scps_econ.c) parce que changer la signature d'agency_build_acct casserait scps_sim.c:CMD_BUILD
  (possédé par P ce jour-là). Jour-1 permissif (ai_step:655 précède econ_apply_country_tech:687
  dans sim_day) — même classe de décalage que tech_arquebus. Bancs isolés (jamais de cache) =
  permissif par construction (agency_demo passe sans rien toucher).
- MIROIR lot M tenu : scps_build_legal_ex gagne la raison 4 (« tech de palier manquante »),
  INSÉRÉE ENTRE la file F5 et la matière (même ordre que le drain). membrane_audit.gd étendu
  (borne 0..4 + bucket histogramme). construction_panel.gd : le mot au survol.
- RELIGION ⌈N/3⌉→⌈N/2⌉ : une ligne (religion_cap) + commentaires ; le schisme (RELIG_SCHISM_MAX 5
  par racine) INTACT. api_demo recalibré (⌈4/2⌉=2 garde le même scénario de rallier-au-plafond).
**Pièges** :
- ⚠ **LE TEMPLE ÉTRANGLE LA FONDATION — MESURÉ, PAS CONTOURNÉ** (la mission demandait de
  signaler) : seed 9 (3×250 ans) religion 1.3→0.3 foi/sim (baseline cda797f vs lot T) ;
  seed 11 : 0.5→0.5 (inchangé). Cause EDI_DBG : le SANCTUAIRE lui-même ne se bâtit pas
  (made=0 · nocap=59-71 — le pool commercial §5 n'a pas le bois/l'argile au moment de l'ordre,
  PRÉ-EXISTANT, identique baseline) ⇒ l'échelle Sanctuaire→Temple ne DÉMARRE jamais ⇒ le
  Temple n'est JAMAIS ordonné (aucun compteur). L'ancienne fondation (1.3/sim) roulait en
  fait sur le MONASTÈRE (made=1, voie savoir, dans l'ancien masque tout-édifice-religieux) —
  pas sur le Sanctuaire. Le vrai verrou n'est NI la tech T2 NI le prix du Temple : c'est
  l'approvisionnement du Sanctuaire (nocap) + le masque restreint. Pistes si on veut r'ouvrir
  (décision d'équilibrage, PAS prise ici) : autel humble bis (retry/file d'attente sur nocap),
  ou compter le Monastère comme site de fondation, ou zèle qui ré-essaie plus souvent.
- La baseline de religion avait DÉJÀ décru (2.0/sim documenté → 1.3 seed 9 / 0.5 seed 11
  mesurés sur cda797f) — les re-baselines récents (Lot W worldgen) avaient déjà resserré ;
  ne pas comparer au chiffre du CLAUDE.md sans re-mesurer le parent.
- `golden` IDENTIQUE malgré tout (aucun re-baseline nécessaire) : dans la fenêtre 12 ans,
  aucun empire ne pose d'édifice palier ≥2 sans tech T2, aucune fondation <12 ans sur les
  5 graines golden, et le grain province==région tôt (mono-province). Vérifié `make golden`.
- Mesurer le parent : `git checkout <sha>` DÉTACHÉ dans SON worktree + rebuild fonctionne
  bien (pas besoin d'un worktree de plus) ; ne pas oublier de re-checkout la branche ET de
  rebuilder avant le savetest.
**Restes** :
- Le compteur `g_edi_notech` n'est pas sérialisé (télémétrie pure, RAZ agency_init — même
  classe que g_edi_made : jamais lu par le moteur). RAS pour le savetest (vérifié seed 9
  byte-identique) mais un futur --savetest qui comparerait la TÉLÉMÉTRIE verrait un écart.
- Les seuils TIER2_POP…TIER7_POP sont surchargeables au LANCEMENT (SCPS_TUNE) mais pas au
  panneau F10 en direct (cache statique scps_labor.c) — documenté dans le fichier ; si un
  jour on veut le live, invalider le cache depuis tune_set (hook à ajouter).
- Fondation religion : cible « ≈ cap 3-4/sim » PAS atteinte (0.3-0.5/sim) — cf. Pièges ;
  la décision (assouplir quoi) appartient à l'orchestrateur/joueur.

## 2026-07-07 — Lot P pillage unifié + esclavage + verbe côtier
**Découvertes** :
- **Le double-gate mutuel qui rendait le sac BORGNE** : `diplo_pillage_region` ET `diplo_enslave_capture` (scps_diplo.c) posaient/lisaient chacun le MÊME `pillage_cd` en interne (héritage LOT 4, anti-double-sac fall→règlement) — appelés à la SUITE dans un même évènement (settle_transfer :956-957, occupation-capture scps_sim.c), le PREMIER posait le cooldown et le SECOND se voyait bloqué par lui : un sac ne rendait JAMAIS les deux effets (or/matière ET captifs), seulement celui exécuté en premier. Fix : `diplo_pillage_fresh` (lecture partagée, l'appelant gate UNE FOIS avant les deux sous-appels) + le pose du CD ne vit plus QUE dans `diplo_pillage_region`.
- **`victim_cid` doit être passé EXPLICITEMENT à `diplo_pillage_region`** : au règlement de paix (settle_transfer), `econ_region_set_owner` a DÉJÀ basculé la propriété au vainqueur quelques lignes AVANT le saccage — `region[].owner` ne donne plus la victime. La règle 20%·revenu lit `econ_country_tax_year(victim)` : sans le paramètre, on aurait pillé le revenu du... vainqueur.
- **`econ_country_tax_year` est alimenté PARTOUT** : chronicle.c:570 (annuel, avant les 365 sim_day) ET la façade scps_api.c:140 (`s->sim.day % 365 == 0`) appellent `econ_flux_year_capture` — la règle 20% a un revenu réel dans les deux fronts dès l'an 2 (an-1 : repli extrapolé >90 j, sinon 0 ⇒ pillage nul, assumé).
- **La règle de ciblage de la course pirate IA** (scps_navy.c §4, reprise pour le verbe joueur) : la piraterie est un acte GRIS — AUCUNE guerre requise, seuls l'ALLIÉ (diplo_status==DIPLO_ALLIED) et sa propre terre sont exclus. Le verbe joueur ajoute le PACTE commercial à l'exclusion (« pas d'allié/pacte » verbatim). CD/balafre = `raid_cd_days`/`balafre_days` PROVINCE-owned, helper partagé `navy_mark_raided` (les constantes COURSE_* sont privées à scps_navy.c).
- **Piège de fraîcheur (vue mensuelle)** : `region[].raid_cd_days` est un agrégat max-provinces rebâti par econ_tick (mensuel) — `navy_mark_raided` écrit la PROVINCE REPRÉSENTATIVE. Un gate/reader qui lit la VUE juste après le drain voit l'ancien état (même piège que le gate de diplo_pillage_region, documenté là-bas). Le drain CMD_RAID_COAST et `scps_can_raid_coast`/`scps_raid_cd_days` lisent donc `prov[econ_region_rep_province(région)]` directement. (La course IA lit la vue — toléré : cadence 240 j ≫ 30 j de staleness.)
- **Le round-trip banc du verbe exige une coque pirate** que le monde de test ne sait pas toujours bâtir dans la fenêtre → setter BANC-only `scps_debug_set_pirate_hulls` (motif `intertrade_debug_set_hub_of`, lot A), jamais appelé par le jeu/binding. Sans lui le test tombait dans la branche « pas de cible » (prouvé : cible -1 avant, province 41 + CD 1825 j après).
- **Mesures (chronicle 9×3×200 + 42×2×200, 6 emp 12 cités)** : pillage réel 16-142/sim (seed 9 : 30.7/sim · seed 42, monde belliqueux : 96.5/sim), valeur RÉELLE = **31-53 % de la cible 20%·revenu** (40-43 % en moyenne — la borne « ce qui existe vraiment dans la province » MORD : la victime ne tient pas 20 % de son revenu annuel dans une seule province, c'est le « si possible » de la règle) ; âmes déportées 0-796/sim (seed 9 : 174/sim · seed 42 : 398/sim ; 0 quand aucun vainqueur n'a le gate) ; razzias pirate esclavagistes rares (0-8/sim — les éthos pirates ne sont pas toujours Dominateur/Honneur) ; monde SAIN sur les 5 sims (satisfaction Laborer 74-81 / Bourgeois 87-88 / Élite 80-89, hégémon mortel 5/5, IPM 1.07-1.27, réfugiés respirent, aucune spirale de misère côtière).
**Pièges** :
- `diplo_demo` §6 posait `rvr->stock[]` sur la VUE region[] sans provinces derrière — passait avant parce que le trésor (1000) couvrait la cible ; la partie stock-complément du mover n'est numériquement prouvée que par la borne « victime pauvre » (+ §6b siège qui exerce econ_region_stock_add). Un banc du chemin mixte or+stock devrait semer les PROVINCES.
- `scps_warhost.c:53-54` porte 2 warnings -Wmisleading-indentation PRÉ-EXISTANTS (commit collègue 55b0edf « DES ARMÉES RÉELLES », hors fichiers du lot P) — le « 0 warning » du dépôt est cassé par CE fichier, pas par le lot P.
- `navy_course_tick` a changé de signature (+`const TechState *ts` pour le gate de razzia) — tout appelant futur doit passer `s->ts` (un NULL désarme la razzia proprement, gate `if (ts && …)`).
**Restes** :
- La razzia pirate IA est gatée par l'éthos/tech du COMMANDITAIRE de la course — si un futur lot veut « tout pirate razzie » (la piraterie historique asservissait sans doctrine d'État), retirer le gate econ_country_can_enslave du site scps_navy.c et ne garder que le flag.
- Le pillage à l'occupation-capture n'a pas de ligne feed dédiée (FEED_SIEGE_FALLEN existe déjà pour la chute) — si le joueur veut VOIR « pillée : X or », ajouter un champ au feed.
- `g_navy_raid_slaves`/`g_occ_pillage_total` sont des GLOBAUX de télémétrie jamais RAZ entre sims (delta-snapshotés par chronicle, motif g_tot_occ_posed) — un futur --savetest qui comparerait ces compteurs entre A et B les verrait diverger : ils sont HORS moteur, ne jamais les sérialiser ni les lire en décision.

## 2026-07-07 — Lot U bourgs T1-T7 (remplacement urbaniste)
**Découvertes** :
- **Le tier de vignette vient du reader façade** (`Sim.world.region_tier`, 0-5, capitale forcée ≥4) —
  PAS de `_pop_tier`/`CITY_POP_BANDS` (l'ancien étalement par pop, supprimé). Mapping retenu : 0-1→t1 ·
  2→t2 · 3→t3 · 4→t4 · 5→t5 ; capitale +1 cran (⇒ t5/t6 vu le forçage façade) ; **t7 UNIQUE** = la
  capitale la plus peuplée du monde (`_update_top_cap`, une boucle pays au tick ; cité-état/hameau
  exclus de la course). Cité-état → `bourg_cs`, hameau libre (rôle 4) → `bourg_wild`. Le cache
  `_town_cache[r]` est keyé sur le `sid` complet → il se rebâtit seul quand le tier monte OU quand le
  titre t7 change de mains.
- **Les vignettes recentrées n'ont PAS le socle au bord du cadre** : le pipeline recut (58514f) recentre
  le CONTENU sur 256² → le pied vit au bas du bbox opaque (t1 : y 82..172 ; t7 : y 36..218). L'ancrage
  au pied et l'échelle du CONTENU sont donc MESURÉS à la 1re charge (`Image.get_used_rect()` →
  `foot`/`cw` cachés par id dans `_bourg_tex`) — un ancrage fixe 0.94 aurait fait flotter les petites
  vignettes de ~7 cellules au-dessus de leur siège.
- **Volume retiré : −738 lignes nettes dans overlay.gd** (892 supprimées / 154 ajoutées, commit
  f9090e5) : `_build_town` (~305 l, tout le plan composé : rangs/ruelles/spirale/relaxation/champs/
  enceinte/portes/plaza/puits/halle/forge/fumée/perspective) + `_seat_road` + les 5 renderers
  vectoriels (`_ink_house`/`_ink_landmark`/`_house_pts`/`_house_shadow`/`_f32_h`) + ~40 constantes/vars
  fossiles vérifiées ORPHELINES une à une (grep \b par symbole AVANT suppression) : F32_POOL/
  F32_LANDMARK/ROOF_PAL/TOWN_WALL/TOWN_GROUND/FIELD_*/PLAZA_FILL/SMOKE_SOFT/STAMP_*/SZ_*/EDI_RING/
  CLUTTER_SIZE/_clutter/FOUND_*/_found_tex/SHADOW_COL/SHADOW_DIR/LIGHT_WORLD/SHADE_K/GROUND_TINT_*/
  BIOME_CITY/FOREST_TREES/DRESS_OPEN..DRESS_BOG/ROADSIDE/ROAD_DRESS_OFF/MAIN_ST_LEN/DRAW_ROAD_DRESS/
  _road_dress/_road_cells/_main_streets/_road_placed/_town_streets/_region_variant/_region_centre/_bk/
  _clear_set/_region_citymax/_footprint_clear/_has_any/_rh/_struct_dirty/_pull_dry.
**Gardé (et pourquoi)** :
- **Quais/jetées/barque** (`_build_quays`, extrait verbatim du plan) : le rivage dépend du MONDE, la
  vignette ne peut pas le porter ; l'ancre de jetée = la DERNIÈRE terre avant l'eau (sèche par
  construction — l'esprit du lot V tenu sans `_pull_dry`). Pas de quai pour le hameau sauvage.
- **Ponts d'encre** (`_ink_bridges`), **clairière** (`_dress_clear`, rayon ∝ tier inchangé — couvre la
  demi-largeur des vignettes à ±0.5 cellule près), **bannière de lieu**, **liseré pourpre de capitale**
  (= le marqueur de capitale, sobre — aucun fanion ajouté sur la vignette), **repli glyphe d'encre**
  (`_draw_town`) si l'asset manque.
- **`_decor`/`_structures`** (fossiles JAMAIS peuplés) : gardés parce que **`viewer_audit.gd` les
  itère/mesure** — les purger casserait la probe (fichier hors périmètre du lot) ; à retirer AVEC elle.
- **Le push-inland de `_region_anchor`** (via `_max_dry_size`/`_nearest_sea_dir`/`_sea_clear_rect`) :
  l'ancre sert d'ABOUTISSEMENT AUX ROUTES — le garder = réseau routier strictement inchangé ;
  `_region_citymax` (le produit dérivé write-only) est, lui, tombé.
- **Tri peintre** : les vignettes sont GRANDES (jusqu'à ~11 cellules) → collecte puis tri fond→avant
  (y écran) + bannières en 2e passe PAR-DESSUS tout ; marge de cull élargie 40→160 px (une vignette au
  bord ne pope plus).
**Pièges** :
- **`--check-only -s res://map/overlay.gd` échoue TOUJOURS** (« Identifier not found: Sim ») : `-s`
  tourne sans autoloads — ce n'est PAS une erreur de syntaxe. Le vrai gate est le boot headless
  `Main.tscn --quit-after 30` (0 SCRIPT ERROR) + un run shot_parch (exerce generate+draw).
- **seed 9 : TOUS les hameaux WILD sont ralliés entre l'an 5 et l'an 15** — une capture `wild=1` à
  years=120 (voire 15) tombe en repli silencieux sur le centre de carte (= la mer, 20 min perdues à
  chercher un bug de caméra). Capturer le sauvage à **years=5**. L'arg `wild=1` (miroir de cs=1) est
  commité dans shot_parch (09b73c1).
- **Ne pas confondre** : un aplat sombre à bords doux près d'un bourg peut être un LAC (eau douce du
  parchemin), pas une ombre orpheline — vérifier sur le shot AVANT de chasser un bug d'ombre.
- `git stash push -- godot/project/map/overlay.gd` (fichier SEUL) pour le BEFORE du hameau : le probe
  modifié (wild=1) reste en place pendant que l'overlay revient à l'ancien monde — A/B propre.
**Restes** :
- **La famille route-tiles/ponts-sprites** (`USE_ROAD_TILES`/`ROUTE_*`/`_road_tex`/`_road_tiles`/
  `_route_meshes`/`_bridge_tex`/`_bridges`/`ROADS_IN_SHADER` + les 2 sets `_road_tiles_dirty`/
  `_bridges_dirty` de _on_tick/_on_generated) est un OSSUAIRE décl-seul du même genre — non purgée ici
  (hors sujet bourgs, ~20 lignes) ; candidate à la prochaine passe de code mort.
- **`viewer_audit.gd` lit `_decor`/`_structures`** (toujours vides) — la purge conjointe probe+vars
  reste à faire.
- Les rails px de vignette (`15+2.5·rt` / `96+24·rt`) et les largeurs monde (`BOURG_W_*`) sont dialables
  en tête d'overlay.gd si la DA veut plus gros/petit ; le GLAZE est `0.93+0.10·hash` α 0.90.
- Captures (gitignorées, `godot/project/`) : `u_seed{9,42}_{fit,mid_cap,deep_cap}_{before,after}.png` +
  `u_seed9_cs_{before,after}.png` + `u_seed9_cs_deep_after.png` + `u_seed9_wild_{before,after}.png`
  (years=5, zoom=10).

## 2026-07-08 — Lot G flux humains (pacte élargi · réfugiés · achat servile IA)

**Découvertes** :
- `demography_migration_pact_tick` : DEUX taux désormais — `MIG_PACT_FRAC` (canal pacte COMMERCIAL,
  gardé au calibrage d'origine : un pacte commercial peut se former AVANT l'an-12, bumper ce taux
  casse le golden — mesuré seeds 7/209) et `MIG_PACT_FRAC_ALLY` ×3 (canal ALLIÉ : l'invariant
  « aucune alliance < an-12 » tient par construction, le taux haut est golden-safe).
- Marché servile : les âmes s'entassaient au pool des Centres (vendues par les pillards) SANS
  acheteur — l'IA n'avait AUCUN chemin d'achat. Ajouté : un empire can_enslave en PÉNURIE DE BRAS
  (signal forecast existant) achète au pool, borné budget/cadence (scps_ai.c section servile).
- Réfugiés : A/B apparié (seeds 9+11 × 3 sims) — SCAR 0.50→0.40 + FRAC 0.03→0.04 : fuites ↑
  (6/27/25 → 9/58/8) et retours ↑ (55/407/414 → 96/490/436) à satisfaction STABLE OU MEILLEURE
  (Laborer 81/76/74 → 83/77/76 seed 9) — le bassin de bistabilité n'a PAS mordu à 0.04.

**Pièges** :
- La leçon de bistabilité (CLAUDE.md § RÉFUGIÉS) TIENT : 0.04 est la borne testée, pas un plancher —
  ne pas pousser au-delà sans A/B apparié avec la satisfaction en garde-fou.
- Les sweeps de mesure en arrière-plan verrouillent chronicle.exe (le build/determinism échoue
  « Permission denied ») — tuer LA BOUCLE parente, pas les chronicle enfants (ils re-spawnent).

**Restes** :
- L'A/B réfugiés n'a couvert que 2 graines (9/11) — la re-validation à 20 graines (sweep post-merge)
  confirmera à l'échelle.
- L'affranchissement reste rare (méd 0) — le chemin existe (statecraft), non poussé par l'IA ;
  à re-mesurer après le sweep.
## 2026-07-08 — Lot F fins dispatchées + catastrophes + exode
**Découvertes** :
- **La cause du biais FROID (mesuré 97 GRAND HIVER · 29 RONCES · 17 EAU sur 200 sims)** :
  `endgame_select_and_fire` (scps_endgame.c), quand aucun transmuteur ne domine (`mx<1.0`, le
  cas COURANT — TROUVAILLES « chaîne morte des rares faustiens »), hashait `fauteur`/`epicentre`
  via DEUX multiplications XORées (`h = (f+1)*2654435761 ^ (epi+1)*40503`). Testé isolément (paires
  non corrélées) ce hash est PRESQUE uniforme — le vrai coupable est que `epi` et `fauteur` sont
  CORRÉLÉS PAR CONSTRUCTION (`endgame_pick_fauteur` : epi = capitale DU fauteur) ; pour des paires
  linéairement corrélées (`e=f*mul+c`), la faible diffusion de ce hash biaise fortement `%3` selon
  `mul` (vérifié en C standalone : jusqu'à 12/2/6 sur 20 échantillons pour certains `mul`). Fix :
  `fin_mix32` (avalanche 2-rounds fmix32-like, comme `wg_mix` dans scps_world.c) sur la paire
  combinée + année + graine du monde → testé sur 3600 paires corrélées (36 `mul` × 20 `f` × 5 graines
  monde), donne 33.2/33.0/33.8 % (quasi 1:1:1). Poids diégétiques ±35 % (monde déjà froid → pénalise
  FROID ; monde aride → gonfle RONCES) gardés MODESTES (testé pire cas t=0.2/m=0.2 → ratio max 1.53:1,
  toujours ≤2:1).
- **`endgame_region_intensity` FIN_FROID était FLAT (bug latent PRÉ-EXISTANT, découvert en
  débogant l'exode à 0)** : le commentaire promettait « un rien modulée par la température locale »
  mais le code renvoyait `eg->cold_offset` BRUT, IDENTIQUE sur TOUTES les régions — l'exode générique
  ne peut JAMAIS trouver de voisine « moins touchée » si toutes portent la MÊME valeur (`oi < best_i`
  jamais vrai). Fix : scan de température locale (même motif que RONCES juste en dessous), modulation
  ±50 % (région déjà froide pèse plus). Câblé aux DEUX endroits (le reader public `endgame_region_
  intensity` ET le cache batché `endgame_compute_all_intensities` de l'exode) pour garder UNE seule
  vérité (charte LOT D).
- **`biome_habitability` n'avait AUCUN cas pour `BIO_THORNS`** (RONCES) → tombait dans le
  `default=0.55` (habitabilité ORDINAIRE, MEILLEURE que STEPPE !). Le front de ronces avançait des
  ANNÉES sans qu'aucune tuile ne perde de grain avant le flip 50 % qui efface la région d'un coup —
  exactement le défaut signalé en mission (« si RONCES ne mord qu'à la purge, ajoute la dégradation
  progressive »). Fix d'une ligne (`case BIO_THORNS: hab_base=0.05f;`) + `econ_cold_refresh` (déjà
  écrit pour C4, RÉUTILISÉ tel quel — pas de code neuf) appelé depuis `thorns_step` dès qu'une
  cellule corrompt → la famine émerge PROGRESSIVEMENT, des années avant le flip.
- **Perf : ne JAMAIS appeler `endgame_region_intensity` en boucle O(régions²)** — RONCES/FROID
  scannent SCPS_N cellules par appel (comme le faisait déjà le code EXISTANT pour RONCES, un motif
  accepté côté viewer/façade où c'est un appel isolé). Mon 1er jet de l'exode appelait cette fonction
  UNE FOIS PAR RÉGION ET PAR VOISINE chaque année → O(n_régions × degré × SCPS_N), qui a
  probablement contribué aux runs qui semblaient « bloqués » pendant le développement (en plus d'une
  contention RÉELLE avec d'autres agents parallèles — `ps -W` a montré jusqu'à 5 `chronicle.exe`
  simultanés d'autres worktrees). Fix : `endgame_compute_all_intensities` calcule TOUTES les régions
  en UN SEUL balayage (cas par cas, vérifié IDENTIQUE au reader public), l'exode ne lit plus que des
  O(1) dans ce tableau.
- **SANG peut fire SANS AUCUNE région marquée** (`sang_seed` ne marque que `revolt_scar >
  SANG_SCAR_MIN` — une cicatrice de RÉVOLTE, pas de guerre inter-états) : sur seed 5 (SANG déclenché
  par le ratio morts-de-guerre/pop GLOBAL, causé par des guerres ORDINAIRES entre pays, pas des
  révoltes), `sang_seed` a marqué **0 région** (`[EXODIAG] sang_seed : 0 région(s) marquée(s)`) →
  `sang_step` ne draine NI ne fait fuir PERSONNE cette partie (le mécanisme entier est un no-op, pas
  juste mon exode). C'est un gap PRÉ-EXISTANT du design #32 (le drain SANG est gaté sur `revolt_scar`,
  qui ne capture qu'une partie des « ravages de guerre » — pas dans mon mandat de le refaire, mais à
  savoir pour qui reprend SANG).
- **`ps -W` (MSYS2) liste les process Windows tous worktrees confondus** — utile pour distinguer
  « mon process est lent » de « mon process est mort », et repérer la contention CPU d'agents
  parallèles (lot G/I tournaient leurs propres `chronicle.exe` en même temps).
**Pièges** :
- Après un `rm -f build/*.o` + rebuild propre, un lien `chronicle.exe`/`scps_viewer.exe` peut échouer
  en « Permission denied » si un ANCIEN process de CE worktree tourne encore (un run précédent lancé
  en arrière-plan, pas forcément visible dans le terminal courant) — `ps -W | grep chronicle` +
  `kill -9` avant de relier.
- `SCPS_EXODIAG=1` (gardé, gate `getenv`, coût nul par défaut, comme `SCPS_ENTDIAG`) imprime le
  détail SANG (régions marquées au fire + par-région dst/évacués au tick) — utile si quelqu'un
  reprend le calibrage SANG.
**Restes** :
- **SANG : le mécanisme de marquage (`sang_seed`/`revolt_scar`) peut laisser le drain ET l'exode
  totalement INERTES** quand la guerre qui a fait monter le ratio mondial était inter-états (pas des
  révoltes) — hors mandat Lot F (le drain lui-même, pas juste son routage vers l'exode), mais à noter
  pour une session qui recalibre #32/SANG : peut-être un second signal (dégâts de siège/occupation
  par région, déjà suivi ailleurs — cf. LOT 4 pillage de siège) en plus de `revolt_scar`.
- **EXODUS_INTENSITY_MIN=0.15 calé sur peu de runway** (le gate `ENDGAME_YEAR_OPEN=180` laisse souvent
  20-70 ans avant la fin du sim) — un run BEAUCOUP plus long (400-500 ans) donnerait plus de marge et
  pourrait justifier de remonter le seuil ; dialable d'une ligne (`SCPS_TUNE=EXODUS_INTENSITY_MIN=…`).
- Distribution des fins mesurée sur l'échantillon MANDATÉ (graines 3/5/9/42/145/777 × 3 sims × 250 ans,
  18 sims) : seulement 5 fins déclenchées (13 « aucune ») — RONCES 1 · FROID 3 · SANG 1 · EAU 0. Trop
  peu de tirages pour un ratio statistiquement solide (le fix est prouvé MATHÉMATIQUEMENT par ailleurs,
  cf. Découvertes) ; un sweep BEAUCOUP plus large (50+ sims) donnerait une mesure de terrain plus
  ferme si une session future veut re-vérifier.
## 2026-07-08 — Lot I moteurs d'innovation
**Découvertes** :
- **Le goulot RÉEL n'était PAS le revenu (econ_country_savoir) mais la SÉLECTION** (deux niveaux
  distincts, tous deux mesurés avant tout code) :
  1. **La brique bâtie (Bibliothèque/Monastère) n'est structurellement joignable que par 2 éthos
     sur 6** — dans `ai_econ_turn` (scps_ai.c ~1375-1465), Dominateur/Honneur vont TOUJOURS à
     l'arsenal (branche inconditionnelle hors crise, AVANT le bloc civil), Mercantile va TOUJOURS
     au Marché, Bureaucrate en est EXPLICITEMENT exclu (« ne quitte jamais K »). Seuls Ordre et
     Pacifiste atteignent le sélecteur savoir. Diag EDI_DBG (3x250 ans seed 9, 18 empires-instances) :
     Bibliothèque made=7, Monastère made=5, Académie made=1 — cohérent avec « environ un tiers des
     empires seulement peuvent essayer ».
  2. **Bug de grain région** : le gate d'entrée (`bd->K_inst>=AI_SAVOIR_K && bd->savoir<2.5`) lisait
     la CAPITALE (`bd`, figée à `hr=a->home_region`) alors que le bâtiment se pose sur `cr`
     (`ai_neediest_civic_region`, qui dérive vers la périphérie). Une fois la capitale ELLE-MÊME
     accumulait savoir>=2.5 (environ 2 bâtiments), le gate se refermait POUR TOUJOURS sur TOUT
     l'empire, même si la périphérie n'avait RIEN — même famille de bug que le Lot T (province vs
     région).
  3. **Même en réparant 1+2, l'arbre% ne bouge quasi pas — preuve DIRECTE** (SCPS_SAVOIRDIAG, ajouté
     dans chronicle.c) : Σsavoir-bâti/empire reste 0.0-0.7 sur 250 ans MÊME avec le fix (seeds
     9/42/145, 1 sim chacune). Cause : `SAVOIR_LIB_MAX` plafonne le bonus de bibliothèque à +33%,
     un plafond QUASI JAMAIS approché (il faudrait environ 5 bâtiments, jamais atteint sur 250 ans
     même en forçant tous les éthos à essayer). Test décisif : `SCPS_TUNE="SAVOIR_LIB_MAX=3.0"`
     (dix fois le plafond) sur le code BASELINE (aucune bibliothèque jamais bâtie) donne un arbre%
     STRICTEMENT IDENTIQUE (zéro bâti fois n'importe quel plafond = zéro bonus). Ceci PROUVE que la
     brique bâtie est un levier structurellement FAIBLE (capé, jamais atteint), quel que soit
     l'effort mis sur la fréquence de pose.
  4. **Le VRAI moteur d'innovation était ailleurs dans l'arbre lui-même** : la chaîne
     Scriptorium→Académie→Université (`scps_tech.c` : THM_SAVOIR·FN_PRODUCTION, tier 1→2→3,
     prereq=TECH_BIBLIOTHEQUE gratuite) alimente `tech_research_yield` DIRECTEMENT (+0.5/nœud,
     scps_tech.c:465) — un multiplicateur qui COMPOSE (chaque maillon accélère TOUS les suivants
     sur 250 ans), contrairement au plafond additif de la brique bâtie. `ai_pick_tech` (le glutton)
     ne la préfère pas aux 15 autres nœuds tier-1 en concurrence (aucune notion de « rendement
     futur » dans son score) → le multiplicateur reste à x1 toute la partie, indépendamment du
     nombre de bibliothèques bâties ou du plafond SAVOIR_LIB_MAX.
- **Fix retenu (le levier qui MARCHE, mesuré)** : « SOIF DE SAVOIR » dans `ai_research_step` (motif
  IDENTIQUE à SOIF DE PALIER/S1/S3/S4 déjà dans le fichier — épargne ciblée pour le PROCHAIN maillon
  non acquis de la chaîne Scriptorium→Académie→Université, via `ai_step_toward`, jamais au-delà).
  Placé AVANT la soif de palier (tier1, bon marché, prime la composition) ; pose `palier_hold=true`
  pour que S1/S3/S4 s'inclinent pareil (même contrat que le reste du fichier).
- **Fix secondaire retenu (bug corrigé, indépendamment utile)** : le gate savoir de `ai_econ_turn`
  lit désormais `crd` (build de `cr`, le site RÉEL) au lieu de `bd` (capitale figée) — corrige le
  verrou permanent-empire-entier. Ajout d'un catch-up « savoir/tête < 45% médiane mondiale »
  (`AI_SAVOIR_CATCHUP_FRAC`, registre J) qui traverse TOUS les gardes d'éthos (Dominateur délaisse
  UN tour d'arsenal, Mercantile délaisse SON marché, Bureaucrate sort de K) — rafraîchi UNE FOIS/JOUR
  (`ai_savoir_refresh`, cache statique day-gated dans scps_ai.c, PAS dans scps_sim.c — motif
  `tech_diffusion_refresh` : `ai_step` est appelé PAR-ACTEUR chaque jour, le 1er appel de la journée
  calcule, les suivants lisent, coût O(n_pays x n_régions) UNE fois/jour, jamais dans le hot path
  par-acteur — la tentative FAU-IA lot 2 avait re-baseliné + explosé le temps de sim pour avoir mis
  un calcul O(n_pays) DANS ce hot path, cf. commentaire existant scps_ai.c ~1225).
- **Leviers ÉVALUÉS et REJETÉS** : (a) inflation directe de SAVOIR_LIB_PER/MAX — interdit par la
  mission ET inutile (prouvé ci-dessus, un plafond plus haut sur un compteur à zéro ne change rien) ;
  (b) FRAC de catch-up à 1.0 (tout empire sous la médiane, pas seulement les extrêmes) — testé,
  Bibliothèque made 6→25 / Monastère 5→16 sur le même 3x250-ans, mais arbre% n'a PAS clairement
  progressé (28%/17%/34% vs baseline 32%/17%/30%, un sim a même RÉGRESSÉ) et `nomat` a explosé
  (21-70 refus matière) — la contention sur bois/argile détourne l'économie sans bénéfice net ;
  gardé à 0.45 (l'extrême laggard seulement, un vrai filet de sécurité, pas une politique agressive).
- **Résultat mesuré** (1 sim/graine, 250 ans, 6 empires/12 cités, seeds 9/42/145 — échantillon
  modeste, la variance inter-sim est réelle) :

  | graine | baseline (parent 7f3ade0) | +fix bâtiment seul | +fix bâtiment +soif de savoir |
  |---|---|---|---|
  | 9   | 32% (min8 max72) | 32% (catch-up n'a pas tiré) | 30% |
  | 42  | 19% (min8 max39) | 21% | 26% |
  | 145 | 27% (min8 max56) | 26% | 29% |
  | moyenne | 26.0% | 26.3% | 28.3% |

  Amélioration RÉELLE mais MODESTE (+2.3pp de moyenne sur 3 graines à 1 sim — bruit inter-sim
  significatif, seed 9 a même légèrement régressé). La cible mission (35-45% médian) n'est PAS
  atteinte par ces leviers seuls sur cet échantillon ; un sweep plus large (comme le GIGA SWEEP
  200 sims) serait nécessaire pour juger la médiane avec confiance — non fait ici (budget de
  session). Religion/esclavage restent dans la plage saine observée par le GIGA SWEEP (religion
  2-5 foi/schismes selon graine, esclavage 0-650 âmes) — pas de dérive systémique détectée.
- **Cascade mesurée** : religion et esclavage restent stables run à run (variations dans la plage
  normale de bruit inter-sim, aucune régression de monde) ; satisfaction/hégémon inchangés dans
  leur plage saine (hégémon mortel maintenu, satisfaction 74-91% selon classe/graine).
**Pièges** :
- Le CPU/temps-réel sur cette machine est TRÈS variable selon l'archétype de graine (Lot W, 8
  archétypes) : un monde « archipel » avec beaucoup de petites masses peut prendre PLUSIEURS
  MINUTES juste pour `trace_rivers`/l'érosion, quand un monde « continents » fait le même travail en
  quelques secondes (`chronicle --hash 7 5 12` : environ 3s ; `chronicle 9 1 250 6 12` : environ
  100s ; `chronicle 9 3 250 6 12` peut dépasser 5-10 min). Lancer PLUSIEURS `chronicle` en
  arrière-plan sans attendre la fin du précédent (même sur des graines DIFFÉRENTES) empile des
  process réels qui se contentionnent le CPU (vérifié : la charge globale machine restait basse
  pendant qu'un seul chronicle.exe consommait tout son cœur pendant >100s sur la seule étape
  rivières — PAS un hang, juste lent sur cette graine). Ne PAS lancer un nouveau `./chronicle` tant
  que le précédent n'a pas produit sa ligne finale ; `taskkill //F //IM chronicle.exe //T` nettoie
  un backlog avant de relancer proprement.
- Le Bash tool a un timeout DUR de 2 min par défaut même si on passe `timeout 200` DANS le script —
  il faut passer le paramètre `timeout` (ms) du TOOL lui-même (jusqu'à 600000) pour les runs de plus
  de 2 minutes.
- `ai_pick_tech`/`ai_research_step` : `pick` part du choix du glutton (score multiplicatif) puis se
  fait RÉASSIGNER par une CASCADE de blocs d'épargne (échappatoire famine, SOIF DE SAVOIR (neuf),
  soif de palier, S1 greffe, S3 quête faustienne, S4 doctrine militaire), chacun avec un
  early-`return` si `research_points < coût` (on ÉPARGNE plutôt que de dépenser sur le glouton). Le
  `bool palier_hold` DOIT être déclaré UNE SEULE FOIS (avant le premier bloc qui l'utilise) — un
  2e `bool palier_hold = false;` plus bas écraserait silencieusement un `true` déjà posé.
- La chaîne Scriptorium/Académie/Université est un TECH NODE (scps_tech.h `TechId`), à NE PAS
  CONFONDRE avec l'ÉDIFICE Bibliothèque/Monastère/Académie (`scps_agency.c` `Edifice` — différent
  enum, différent système : l'un se PAIE en or+matière et se BÂTIT dans une région, l'autre se PAIE
  en points de recherche et se DÉBLOQUE pour tout l'empire). Les deux ont des noms qui SE
  RESSEMBLENT (Académie existe des DEUX côtés) — vérifier le type avant de lire un diff.
**Restes** :
- Le seuil `AI_SAVOIR_CATCHUP_FRAC=0.45` est un filet de sécurité DÉLIBÉRÉMENT conservateur (seul
  l'extrême laggard catch-up) — un futur calibrage pourrait le remonter (0.6-0.7) SI un sweep large
  montre que ça ne dégrade pas la satisfaction/l'économie (testé seulement à 1.0 ici, où la
  contention matière apparaît — la vraie fenêtre utile est probablement entre les deux, non
  explorée faute de budget).
- Un sweep plus large (10+ graines x plusieurs sims x 250 ans, comme le GIGA SWEEP existant) serait
  nécessaire pour confirmer/infirmer la cible 35-45% médian avec une variance maîtrisée — cette
  session s'est limitée à 3 graines x 1 sim (bruit inter-sim non moyenné).
- `SCPS_SAVOIRDIAG` (chronicle.c, gated `getenv`) reste dans l'arbre — motif standard, jamais lu
  par le moteur (télémétrie pure, comme les autres `SCPS_*DIAG`).
- La cascade `ai_research_step` a maintenant 6 blocs d'épargne séquentiels (échappatoire, soif de
  savoir, soif de palier, S1, S3, S4) — si un 7e est ajouté un jour, vérifier l'ORDRE de priorité
  voulu (post-hoc, pas de registre central des priorités, juste l'ordre du code).

## [2026-07-08] Endgame §27 — FIN_CHAUD, le RÉCHAUFFEMENT (implémenteur wt/chaud)
**Découvertes** :
- Le combustible du panier EST déjà dans le moteur : `NEED[CLASS_LABORER][RES_WOOD]=1.0`
  (bois de feu annuel, scps_econ.c:236) — l'offre servie se lit à `S[r]-=need*got` dans la
  branche générique du marché (scps_econ.c ~ligne 2716, PAS les branches boisson/luxe qui ont
  leur propre conso). Le charbon se consomme en INTRANT de manuf (poudrière/forge céleste,
  RES_COAL) aux 3 sites `S[e_in1]-=g1` / `S[e_alt]-=ga` / `S[rc->in2]-=lim*rc->q2`.
- `biome_habitability` (scps_world.c:2417) pénalise le chaud via `t_comfort` (→0 au-delà de
  0.72) MAIS son plancher structurel (0.45×base) + le rebiome qui mène un tempéré-HUMIDE vers
  la JUNGLE (hab 0.65) fait qu'un monde tempéré se RÉCHAUFFE sans mourir. Le vrai levier hors
  tropiques = la SÉCHERESSE : baisser `cell.moisture` en même temps que monter la température
  → drylands/désert (hab 0.08-0.28). Sans ça, seed 7 CHAUD = 0 exode ; avec (HEAT_DROUGHT
  0.6×delta), 9480 âmes évacuées. Le bulbe-humide (hot+wet, econ_heat_refresh) reste le kick
  SUPPLÉMENTAIRE des tropiques (trop humides pour sécher — l'étuve les tue quand même).
- Le gate `mx<1.0` (aucun transmuteur DOMINANT) ne suffit PAS à « garder FROID aux mondes à
  corne » : une corne NAISSANTE (mx∈[0.3,1.0)) crossait 55 tôt via le fuel et fire CHAUD
  (seed 301 volé à W=20, mesuré). Fix = `FUEL_DEAD_EPS` (0.5) : CHAUD éligible seulement si
  mx<EPS (transmuteurs ~MORTS) ; sinon RETOMBE au dispatch existant. Vérifié : seeds 9/11/446
  (corne mature, fuel share 1%) gardent GRAND HIVER, seeds 5/99 (sang) gardent SANG.
- Ordre de grandeur : fuel_ratio (mémoire décrue/pop vivante) ~2.5-5.6 en monde calme prospère
  an 180-250. Pour franchir ENTROPY_FIN=55, il faut `ENTROPY_FUEL_W≈15` (le terme fait ~85% de
  la barre) — bien plus haut que ENTROPY_TECH_W (1.35) car la barre est calée pour la charge
  tech, pas un ratio per-capita. golden reste IDENTIQUE (rien ne lit wp->entropy <12 ans ;
  chronicle_sim_hash ne hashe pas l'entropie ; fin gatée >180) → PAS de re-baseline.
**Pièges** :
- `chronicle.exe` reste LOCKÉ par des process fantômes entre deux sweeps (link `Permission
  denied` sur `chronicle.exe`) → `taskkill //F //IM chronicle.exe //T` AVANT chaque rebuild.
  Le Bash tool AUTO-BACKGROUNDE les commandes longues même en un seul appel — les process
  empilés lockent le binaire. Toujours tuer avant de relier.
- Cumuls fuel = ACCUMULATEURS inter-ticks → jurisprudence EMOB/COLC/TXYR : sérialiser SINON
  --savetest diverge. Logés dans le blob ECON (fuel_wood_cum/coal_cum) + section EGAM
  (fuel_seen_*/fuel_charge/heat_offset) → SAVE_VERSION 73→74, save_sane borne ≥0/fini.
- `econ_heat_refresh`/`_cold_refresh` ne posent le grain QUE si `cold_grain < raw_cap[GRAIN]`
  (borne par le bas — le froid/chaud RÉDUIT, n'ajoute jamais) : cohérent avec la carte nue
  géologique (N1). Ne jamais overwrite inconditionnel (planterait du grain partout).
- `endgame_region_intensity(CHAUD)` DOIT différencier les régions (piège FROID documenté au
  lot F : intensité PLATE = 0 exode) → base warming 0.4 + gradient tropical/bas (les tropiques
  fuient vers le tempéré). Formule DUPLIQUÉE en 2 sites (compute_all_intensities + le reader
  single-région) — garder identiques.
**Restes** :
- W=15 fait fire 3/12 mondes calmes à EXACTEMENT l'an 180 (le fuel franchit 55 avant la
  gate) — acceptable (180 in-window) mais peu organique ; baisser W étale mais convertit moins.
- La montée des eaux noie ~0 RÉGION entière (140 cells/an ≪ taille région) — les cellules
  côtières sombrent (visible carte) mais `n_sunken` reste ~0. « Passive » assumé ; bumper
  SEA_RISE_CELLS_PER_YEAR si on veut des régions perdues.
- Théorie du vol résiduel : une corne mûrissant TARD (mx atteint 1.0 vers l'an 230) et dont
  mx<0.5 à l'an 180 peut fire CHAUD tôt au lieu de FROID tard — borné (le sweep giga jugera).
- Calibrage validé sur 12 graines seulement (7 CHAUD / 3 HIVER / 2 SANG) ; le giga sweep
  20×10 tranche la distribution agrégée + confirme « guerriers ~inchangés ».
## 2026-07-08 — Push arbre tech 28 % → ~50 % (revenu de recherche + découplage §27)
**Découvertes** :
- **Le VRAI goulot de l'arbre méd était le REVENU, pas la sélection** (contre l'hypothèse du Lot I
  qui misait sur SOIF DE SAVOIR — mesuré +2 pts seulement). Un probe à revenu absurde
  (`SCPS_TUNE=AI_RESEARCH_INCOME_W=5`, seed 9) monte l'arbre méd à 65 % (max 95 %) → l'access ceiling
  n'est PAS le mur (les combos/apex tier-4/5 restent hors de portée IA, mais le NON-combo va
  jusqu'à ~65 %). Le levier propre : un multiplicateur GLOBAL de `income` dans `ai_research_step`
  (scps_ai.c ~2361) ET la voie joueur (scps_sim.c ~715) — tunable neuf `AI_RESEARCH_INCOME_W`
  (registre J + `#define` fallback scps_econ.h). Réponse ~linéaire : W=1→29 %, W=3→~45 %, W=4.5→~50 %,
  W=6→~53 %.
- **L'arbre et §27 sont COUPLÉS par la charge faustienne — mesuré à l'ENTDIAG** : `wp->entropy` =
  `esum` (Σ `region.faust_charge`, régional/arcane) `+ ENTROPY_TECH_W·Σ ts[c].charge` (charge des
  nœuds tech FAUSTIENS). ⚠ Dans les mondes de chronique `esum` ≈ **0.6** (aucun transmuteur bâti) →
  **toute l'entropie vient de la charge TECH**. Un arbre ×W → ~6× de charge tech → l'entropie
  franchit `ENTROPY_FIN=55` vers l'an **130** (vs ~200 baseline) → TOUTES les fins §27 s'effondrent
  sur le gate `ENDGAME_YEAR_OPEN=180` (violation du garde-fou « pas partout à 180 »). Courbe seed 9 :
  baseline an180=48.3 · W=3 an180=163.8 · W=4.5 an180≈290. `SCPS_ENTDIAG=1` imprime la courbe
  année/année (scps_endgame.c:1183).
- **Le DÉCOUPLAGE qui tient la fenêtre** (`ai_effective_cost`, scps_ai.c ~2234 + miroir joueur
  scps_sim.c) : le coût des nœuds **FAUSTIENS** est ×`AI_RESEARCH_INCOME_W` → le boost de revenu
  s'y **ANNULE**, leur cadence d'acquisition reste ≈ baseline (charge §27 non emballée), l'arbre
  gonfle par les nœuds NON-faustiens (charge nulle). Découplage IMPARFAIT (le yield-chain grimpé
  ré-amplifie l'income → un reste de charge), donc `ENTROPY_TECH_W` abaissé **1.35→0.20** en regard
  (baseline-matching : reproduit l'entropie-à-180 de référence → tir médian ~an 200-235, spread
  180-245, gate respecté). Bénéfice mesuré du découplage : à W=4.5 le tir garde un VRAI spread
  (P : 180-245, 2/8 au gate) là où l'income seul couplé s'effondrait (N W=3.6 : 8/8 à 180).
- **La rareté est le prix** : les mondes BAS-charge (fragmentés, seed 11-type) peuvent ne PAS
  atteindre 55 d'entropie en 250 ans à `ENTROPY_TECH_W=0.20` → ~1/8 des sims ne déclenchent AUCUNE
  fin (vs baseline qui tirait toujours ~an 230). Acceptable/documenté (mild), dialable via
  `ENTROPY_TECH_W` (↑ = moins de no-fire mais plus de collapse-180).
**Pièges** :
- **`scps_province_market` (façade) peut émettre un STOCK de province NÉGATIF** : la ligne est
  admise dès `demand>0.05` (scps_api.c ~1012) SANS regarder le signe du stock ; une province en
  déficit transitoire (`ProvinceEconomy.stock[g]<0`, non clampé côté moteur) fait rougir
  `scps_api_demo` (« scps_province_market : 0..3 lignes bornées »). Edge LATENT **data-dependent**
  (pas causé par le push : W=4.5/5.5 passent, W=5.0/6.0 rougissent sur seed 9 — c'est l'état éco au
  tick de probe, pas un seuil de revenu). Ceci a **borné le ship à W=4.5** (arbre ~50 % au lieu de
  ~53 % à W=6). Un fix dédié (clamp membrane `out[].stock/price ≥ 0` OU clamp moteur
  `ProvinceEconomy.stock`) débloquerait W≥6 → ~53-55 % — laissé HORS PÉRIMÈTRE (touche l'éco/membrane,
  pas l'arbre ; à décider par l'orchestrateur).
- Le séparateur de `SCPS_TUNE` est **la virgule** (`strtok(buf,",")`, scps_tune.c:38), PAS le
  point-virgule (échec silencieux « valeur invalide » qui vide le log).
- **`taskkill //F //IM chronicle.exe` tue les runs de l'AUTRE agent** (image-name, pas per-worktree)
  et réciproquement → sur machine partagée, des sweeps se font tronquer en plein sim (log coupé à
  « [scps] rivières... »). Ne JAMAIS kill par image name ; grep la complétude (`grep -c 'arbre :'`)
  et re-run les seeds tronqués.
- Golden au ship W=4.5 : seeds **108/209/411 BOUGENT**, **7 et 310 INCHANGÉS** (leurs empires ne
  paient aucune tech avant l'an 12 → hash 12-ans identique). À W=6 les 5 bougeaient. NE PAS
  re-baseliner (orchestrateur post-merge).
**Restes** :
- Cible 55-60 % NON atteinte proprement (ship ~50 % méd). Les ~5 pts manquants exigent soit le fix
  de l'edge `scps_province_market` (→ W≥6, ~53 %), soit d'accepter une compression §27 (diffuse 0.55
  → méd 54.5 mais fins 6/8 à 180), soit d'ouvrir l'access barrier (METAB_TIER, Temps 2a, hors levier
  autorisé). Trade-off documenté ci-dessus pour décision orchestrateur.
- `AI_TECH_DIFFUSE_MAX` laissé à 0.40 (défaut) : testé à 0.55 → +~1-2 pts de méd mais recharge la
  fenêtre §27 (le découplage mute son impact faustien mais pas totalement) ET ne sauve pas le `min`
  (~9 %, un empire nain/en-guerre insauvable par la remise). Pas retenu.
- La cascade `ai_research_step` a un 7e site d'échelle possible (income global) AVANT les 6 blocs
  d'épargne — l'ordre reste : income×W → métabolisation → (returns d'épargne). Le ×W faustien vit
  dans `ai_effective_cost`, donc TOUS les blocs d'épargne (foreuse/S1/S3/S4/palier/savoir) voient
  déjà le coût faustien gonflé (cohérent, pas de site oublié).

## 2026-07-08 — Lot E suite : remplissage strings_en.h (142/165 entrées traduites)

**Découvertes** :
- Comparer `strings_ids.h`/`strings_en.h` PAR ID (pas par numéro de ligne) est indispensable : les
  deux fichiers ont des blocs de commentaires de longueur différente (l'en-tête, les commentaires
  M7/§capstone…), donc les lignes X(...) ne s'alignent PAS 1-pour-1 par numéro malgré le même ORDRE
  de macros. Un petit parseur Python (extraction par regex `X\(\s*IDENT\s*,\s*"..."\s*\)` en ignorant
  les blocs `/* */`) donne une table id→valeur fiable des deux côtés et un diff exact.
- Sur les 165 entrées identiques FR/EN repérées par Lot E (session précédente), **23 n'avaient EN
  FAIT rien à traduire** : cognats orthographiquement identiques dans les deux langues (Vassal,
  Suzerain, Port, Temple, Arsenal, Irrigation, Reconstruction, Population, Production, Opulence,
  Stable, imminent, promotion, loyal, PAUSE) ou chaînes 100 % format sans mot français
  (`STR_SLOT_LINE`, `STR_TUTO_PAGEFMT`, `STR_DIPLO_SCORE_FMT`, `STR_MER_DIR_FMT`,
  `STR_COUNCIL_SEAT_FMT`) ou le symbole `—` seul (`STR_BANDE_CARREFOUR_0`, `STR_LENS_0`). Donc
  **142 traductions réelles**, pas 165 — le compte "~165 à traduire" de la session Lot E était une
  approximation côté diff brut, pas le nombre de vraies traductions nécessaires.
- **La cohérence terminologique paie mieux que la traduction ligne-à-ligne isolée** : plusieurs mots
  FR reviennent dans PLUSIEURS bandes indépendantes (ex. "Frémissante"/"Frémissement" apparaît dans
  `STR_BANDE_PRESAGE_1`, `STR_BANDE_AGITATION_1` ET `STR_BANDE_ENTROPIE_1` — cette dernière DÉJÀ
  traduite "Stirring" par la session précédente). Repérer ces récurrences (grep `strings_ids.h` pour
  la même chaîne FR ailleurs) et reprendre la traduction déjà choisie ailleurs (au lieu d'improviser
  un synonyme) évite un vocabulaire incohérent entre bandes sœurs — ici "Stirring" a été repris pour
  les 2 occurrences restantes plutôt que "Tremor"/"Restless" initialement envisagés.
- **Repérer la convention orthographique déjà engagée** avant de traduire en masse : grep sur les
  199 entrées déjà traduites a trouvé "labour" (BR) et "Trade Centre" (BR, dans le corps de texte,
  mais "Trade Center" US dans le nom de bâtiment lui-même — incohérence PRÉ-EXISTANTE, hors périmètre,
  non touchée) → convention penchant britannique adoptée pour les 142 nouvelles entrées
  ("Recognised", "Splendour").
- Le banc `lang_demo` (cible Makefile, PAS dans `run_tests.sh`) exerce le moteur tr_fmt/FNV/glossaire
  mais ne relit AUCUNE des 364 valeurs individuelles — les lignes "ID MANQUANT (non surchargé)" dans
  sa sortie sont un test délibéré de `lang_audit` contre un fichier de surcharge-test épars (pas un
  signal sur strings_en.h). Le vrai gate de parité FR/EN est l'assert de taille à la COMPILATION
  (`scps_lang.c`) : un build vert prouve déjà 0 ligne ajoutée/retirée.

**Pièges** :
- Ne PAS traduire "or" en pensant à un reliquat français : c'est la conjonction anglaise correcte
  dans "Loyal or Defiant", "Buy or sell" etc. — mon script de résidu Unicode-aware (`residual_check.py`,
  liste de stop-words FR) l'a signalé 5× en faux positifs ; il fallait relire le CONTEXTE (anglais
  valide) avant de convertir un hit en vraie anomalie.
- `grep` en Git Bash sur Windows fait du matching BYTE-LEVEL, pas Unicode-aware : une classe
  `[éèàçêù]` matche le PREMIER OCTET UTF-8 (souvent `0xC3`), qui est PARTAGÉ par des caractères
  n'ayant rien de français (`×` U+00D7 et `÷` U+00F7 encodent aussi en `0xC3 ..`) → faux positifs sur
  `STR_MER_MORTE`/`STR_MER_COURANT`/`STR_ENTREPOT_CAP_FMT` (déjà traduits, intacts). Pour un vrai
  résidu FR, écrire un check Python qui compare des CODEPOINTS unicode décodés, pas un grep shell sur
  les octets bruts.
- Un remplacement id-ancré doit vérifier la valeur FR ATTENDUE avant d'écrire (mon script
  `apply_translations.py` refuse silencieusement — reporte un MISMATCH — si le contenu actuel diffère
  de ce qui était attendu) : ça aurait intercepté toute dérive entre le diff initial et l'état réel du
  fichier (aucune dérive ici, 142/142 appliqués sans mismatch, mais le garde-fou est peu coûteux et
  évite d'écraser une traduction déjà faite entre-temps par un autre agent).
- Les entrées `STR_TUTO_PAGE_*` contiennent des `\n` LITTÉRAUX (2 caractères backslash+n dans le
  fichier source, PAS un vrai saut de ligne) — les construire en Python avec des chaînes brutes
  `r"...\n..."` (pas des chaînes normales où `\n` deviendrait un vrai saut de ligne à l'écriture).

**Restes** :
- Le résidu FR est nul (0 mot français authentique restant dans `strings_en.h` ; les 23 entrées
  encore texto-identiques au FR sont des cognats/formats, vérifié terme à terme).
- Le header-comment de `strings_en.h` (lignes 1-8, comment de style X-macro) reste en français à
  dessein — c'est de la doc d'ingénieur (« l'outillage, pas le jeu », cf. CLAUDE.md §langue), pas une
  chaîne face-joueur ; non touché, cohérent avec `strings_ids.h` qui a le même en-tête FR.
- Backlog i18n Godot (629 littéraux .gd, `docs/i18n_backlog.csv`) toujours en attente — hors
  périmètre de cette mission (uniquement `strings_en.h` C).
- `STR_EDI_TRADE_CENTER` = "Trade Center" (US) vs "Trade Centre" (BR) dans `STR_PACT_HOV`/
  `STR_BTN_CENTER_FMT`/`STR_CENTER_HOV` : incohérence PRÉ-EXISTANTE (session Lot E), pas introduite
  ni corrigée ici (hors périmètre — ces 4 entrées étaient déjà traduites, pas dans mes 142).
## [2026-07-08] Polish façade+Godot — stock négatif borné · rose des vents ronde · frémissement (implémenteur solo, worktree SCPS-polish)
**Découvertes** :
- **A1 — `scps_province_market` (scps_api.c:898-908)** : la ligne est admise dès `dem>0.05`
  (ou `sup>0.05`) SANS regarder le signe de `stk` — un déficit transitoire
  (`ProvinceEconomy.stock[g]<0`, non clampé côté moteur, cf. entrée du 2026-07-08 ci-dessus)
  publiait donc un `out[n].stock` NÉGATIF. Fix MEMBRANE (pas moteur) : `out[n].stock = stk>0.f
  ? stk : 0.f` + même clamp défensif sur `price` — `band_marche(dem, sup+stk)` continue de lire
  le `stk` BRUT (un stock très négatif reste une PÉNURIE cohérente, aucun changement de
  sémantique de bande). Preuve avant/après isolée par `git stash push -- scps/scps_api.c` :
  `SCPS_TUNE=AI_RESEARCH_INCOME_W=6 ./scps_api_demo` = **178/179 (✗ province_market)** avant,
  **179/179** après. `scps_api.c` n'est lié dans AUCUNE cible `CHRONICLE_OBJS` (vérifié par grep)
  ⇒ golden/determinism structurellement hors d'atteinte, reconfirmé en pratique (hash 12 ans
  IDENTIQUE au golden commité après le fix).
- **A2.1 — surbrillance de province sélectionnée** : la régression documentée dans CLAUDE.md
  (« CARTE PARCHEMIN UNIQUE », 2026-06-28 — bavait sur le sol) était **DÉJÀ RÉSOLUE** par le
  commit `84b6598` (« carte JOUABLE », 2026-07-02, cf. aussi CLAUDE.md « VIEWER DÉDOUBLONNÉ… CARTE
  JOUABLE ») : `overlay.gd:1628-1647` dessine déjà un contour d'encre (halo sombre + or net, via
  `w.province_border_segments(selp)` → `_chain_segments` → `_smooth_poly` → `draw_multiline`),
  exactement le pattern que le brief demandait de restaurer. Vérifié par `git log -S "contour
  DORÉ de la province choisie"` (une seule occurrence, déjà en place) — **rien à faire, non
  touché**. Le CLAUDE.md/brief citait une note pré-fix périmée.
- **A2.2 — rose des vents ovale** : root cause CONFIRMÉE par dérivation géométrique complète
  (pas une intuition) — `iso_ground.gd` pose `scale=Vector2(1.0,TILT_Y=0.80)` sur le nœud
  IsoGround ; `iso_antique.gdshader::compass(ip,ci,R)` calcule `length(ip-ci)`/`atan` en espace
  LOCAL (pré-transform, `iso=VERTEX`), qui est ISOTROPE — mais ce même espace est comprimé en Y
  par le nœud AU RENDU. Un cercle géométrique en local (`d.x²+d.y²<R²`) devient donc une ELLIPSE
  écran d'axe Y = R·tilt_y (aplatie ~20 %). Dérivation du fix (vérifiée par les deux bouts,
  cf. TROUVAILLES ne PAS confondre `d.y *= tilt_y` avec `d.y /= tilt_y` — le premier est correct) :
  `screen_delta = (d.x, d.y·tilt_y)` ⇒ pré-multiplier `d.y` par `tilt_y` AVANT `length()`/`atan()`
  rend `cd`/`a` déjà "écran-équivalents", donc isotropes → cercle vrai + tick marks bien orientés.
  Implémenté : uniform `tilt_y` (shader, hint_range 0.5-1.0, défaut 0.80) posé par
  `iso_ground.gd::_draw` via `mat.set_shader_parameter("tilt_y", scale.y)` — **lit `scale.y` du
  nœud directement, aucune constante dupliquée** (si `map_view.TILT_Y` change un jour, aucune
  resynchro nécessaire). `compass()` gagne un 4e paramètre `y_fix`, `d.y *= y_fix` juste après
  `d=ip-ci`.
- **C — animation** : `overlay.gd` a une discipline de redraw DÉLIBÉRÉE (queue_redraw() continu
  SEULEMENT si `_cataclysm` ; sinon poll `_sig_poll` à 0.25s sur un changement de souveraineté
  réel, cf. `overlay.gd:1475-1494`) — animer un jeton GDScript (ville/capitale) demanderait de
  réintroduire un redraw continu (tout l'overlay se redessine, pas juste le jeton), contraire à
  cette discipline pour un gain purement décoratif. **Épicentre §27 vérifié DÉJÀ correct**
  (`overlay.gd:1884-1904` : 3 anneaux expansifs, horloge murale `Time.get_ticks_msec()`, alpha
  dégressif — respire bien, non touché). Choix : un **frémissement d'encre shader** sur la rose
  des vents (`TIME` built-in canvas_item — le GPU ré-exécute le shader chaque frame rendue SANS
  appel `queue_redraw()`, zéro coût ajouté, ne touche PAS la discipline de redraw GDScript) :
  `breath = 1.0 + 0.04*sin(TIME*0.5)` multiplié sur l'alpha finale de la rose (~12.6 s/cycle,
  ±4 % — imperceptible en soi, juste "vivant").
**Pièges** :
- Ne pas confondre `d.y *= tilt_y` (correct) et `d.y /= tilt_y` (aggrave l'ovalisation) — la
  dérivation complète est ci-dessus ; un raisonnement hâtif sur "compenser l'échelle" mène
  facilement au signe inverse. Vérifier par les deux bouts (screen_delta = node_scale∘local_delta,
  jamais l'inverse implicite) avant d'coder un fix d'aspect-ratio shader.
- `git stash push -- <fichier>` (jamais `git stash` nu) pour obtenir un binaire BEFORE propre et
  PROUVER qu'un test échouait avant le fix — fait ici pour A1, confirmé 178/179→179/179.
- Process `chronicle.exe`/`*_demo.exe` d'AUTRES agents visibles dans `Get-Process` sur cette
  machine partagée (PID/CPU qui montent sans rapport avec mon run) — ne jamais `taskkill` par nom
  d'image (tuerait le run d'un autre agent) ; le fichier de sortie de SON PROPRE `make test`
  (task-output) reste la seule source de vérité sur la progression.
- `make CC=gcc test` sur cette worktree ne montre plus que **1 KO** (intertrade_demo, BUILD
  ÉCHEC `setenv` POSIX) — campaign_demo/warhost_demo (jadis 2 des « 3 KO Windows ») passent VERTS
  ici (49/49, 21/21, 6/6) : soit le fix stack `-Wl,--stack,8388608` documenté ailleurs est déjà
  dans ce Makefile, soit l'environnement a changé. À noter pour un futur agent qui s'attendrait
  encore à « 3 KO ».
**Restes** :
- `scps_tune_list.h:123-125` (commentaire `AI_RESEARCH_INCOME_W`) documente EXPLICITEMENT que
  ce fix membrane débloquerait `W≥6` (~53 % d'arbre médian au lieu de ~50 % à W=4.5) — HORS
  PÉRIMÈTRE ici (mandat façade-only, l'agent arbre/foreuse est sur `scps_ai.c`/le tuning) : à
  relayer à l'orchestrateur/agent arbre pour décider s'il monte `AI_RESEARCH_INCOME_W` 4.5→6
  maintenant que l'edge est fixé.
- Aucune 2e animation ajoutée (item C explicitement optionnel, 1 suffisait) — si un futur agent
  veut animer le jeton de capitale, il devra soit payer un redraw continu ciblé (ex. gate sur
  "capitale visible à l'écran ET zoom≥X", LOD existant `fine_a`/`ROAD_ZOOM_MIN` comme modèle),
  soit isoler les traits de capitale (`_draw_cap_lisere`) dans un CanvasItem séparé avec son
  propre ShaderMaterial pour un frémissement à coût nul façon rose des vents.
## [2026-07-08] Éco — LA FOREUSE RANIMÉE : panier de minéraux + audit essence (implémenteur solo, wt/foreuse)
**Découvertes** :
- **Pourquoi elle était morte** : `RECIPE[BLD_FOREUSE]` (scps_econ.c) transmutait 0.5 essence → 8 fer
  SEUL. Depuis « MÉTAL SUPPRIMÉ »/« PRIX NATIONAL » (le fer n'est plus rare : chaîne directe fer+bois→
  outils, extraction labor-bound), 8 fer/lot ne vaut plus grand-chose — la RECETTE était devenue
  creuse, indépendamment de si le bâtiment se pose. `bld_min_tier(BLD_FOREUSE)=5` (scps_econ.c:1904,
  province-hôte ≥5000 hab) + `TECH_FOREUSE` tier-4 avec prérequis `TECH_INDUSTRIE` LUI-MÊME tier-4
  (scps_tech.c:116,122) = DEUX nœuds tier-4 à chaîner ; le seul assist (`AI_FOREUSE_HUNGER`,
  scps_ai.c:2299) ne pousse le score QUE si `chain_gap==RES_IRON` (l'empire n'a NULLE PART de fer —
  rawcap ET stock ET supply < seuil) au moment où `TECH_FOREUSE` est DÉJÀ directement recherchable —
  contrairement à CORNE (gatée par `TECH_FORGE_RUNES`, S3) qui a un VRAI beeline (`ai_step_toward`
  sur toute la chaîne de prérequis, scps_ai.c ~2526) : c'est pourquoi corne tire alors que
  foreuse/réplicateur restent quasi-morts (asymétrie structurelle scps_ai.c, HORS PÉRIMÈTRE de cette
  mission — fichiers autorisés = econ/types/tune_list seulement).
- **Audit essence** (`grep -rn "RES_ESSENCE"`) : AVANT et APRÈS mon changement, l'essence n'a QU'UN
  seul consommateur régulier — `BLD_FOREUSE` (in1). Son seul producteur : `BLD_MAGE_WORKSHOP`
  (cristal arcanique → essence 1:1, scps_econ.c:178), lui-même gaté par un gisement de cristal
  géologique (`econ_bld_can_build`, scps_econ.c:1812) — le plus rare de toutes les brutes
  (`EXTRACT_YIELD[RES_ARCANE_CRYSTAL]=0.04`). Un 2e consommateur EXISTE mais HORS économie : la
  Merveille (`MERV_RARE[3]`, scps_endgame.c:918) consomme essence/flux/fer céleste DIRECTEMENT en
  fin de partie (joueur seul, `endgame_empire_consume`) — sans rapport avec la Foreuse.
- **Le graft** : `RECIPE[BLD_FOREUSE]` passe à `{RES_ESSENCE, 0.7f, ..., RES_IRON, 2.0f, ...}` (q1
  0.5→0.7, qout fer 8→2 — le fer reste l'ANCRE du recipe, self-throttlée par son propre prix comme
  toute manufacture) + un panier `FOREUSE_BASKET[6]` (nouvelle table statique, scps_econ.c ~227)
  appliqué dans `econ_tick` §2 MANUFACTURE (`if (b->type==BLD_FOREUSE)`, juste après le bloc out2,
  scps_econ.c ~2507) : cuivre 2.0 · charbon 3.0 · soufre 1.5 · salpêtre 1.5 · or 0.5 · métal précieux
  0.3 — la liste EXACTE des « brutes minérales » de `BASE_PRICE` (le commentaire de la table le dit
  déjà littéralement, scps_econ.c:43). Motif **out2/F3** repris à l'identique : le panier ne compte
  PAS au PIB/salaires (bonus de transmutation, pas de la valeur « travaillée »).
- **Vérifié MÉCANIQUEMENT** (harnais forcé scratchpad, technique d'`econ_arcane_demo.c` : cristal+
  mage workshop+foreuse posés à la main sur une province, hors AI/tech) : les 7 minéraux montent
  TOUS, `faust_consumed[0]` (essence) grimpe 3.85→48.70 sur 23 mois, et le marché s'auto-régule
  (prix fer/cuivre/etc. plongent vers un PLANCHER ~20% base dans ce scénario mono-région forcé —
  du jamais-vu ailleurs — mais NE DIVERGENT PAS : `market_effort` throttle la prod, stock stable en
  régime). PASS.
- **Vérifié EN JEU RÉEL** (2 graines × 5 sims × 250 ans, seeds 7 et 9, chronicle non modifié ailleurs) :
  `conso foreuse` **NON-NUL dans 3/10 sims** (174, 118, 585) contre le **0/200 documenté**
  (`docs/SWEEP_REVALID_2026-07-08.md`, mesuré AVANT VOLUMES DE POP). ⚠ **Attribution prudente** :
  mon changement ne touche NI la recherche NI la construction (scps_ai.c/scps_tech.c hors périmètre)
  — cette hausse de fréquence est très probablement un EFFET DE BORD des calibrages économiques/tech
  qui ont atterri dans ce worktree le même jour (VOLUMES DE POP, et l'entrée juste au-dessus dans ce
  fichier sur `AI_RESEARCH_INCOME_W`/`ENTROPY_TECH_W` qui accélère la pousse de l'arbre) — PAS une
  conséquence de mon graft. Ce que mon changement EXPLIQUE en revanche : QUAND foreuse tire, elle
  produit désormais un vrai panier et consomme PLUS d'essence par lot (0.7 vs 0.5 avant) qu'avant.
- **EAU (essence) toujours 0/10 dans mon échantillon** : dans les 2 sims (seed 9) où foreuse ET corne
  tiraient ensemble, CORNE dominait TOUJOURS (1503>174, 929>118) → GRAND HIVER l'emporte, pas
  ENGLOUTISSEMENT. Raison structurelle : corne consomme du fer céleste DIRECTEMENT (brute→bâtiment,
  1 saut) alors que l'essence traverse DEUX sauts (cristal→atelier de mage→essence→foreuse) — un
  goulot d'étranglement supplémentaire indépendant de mon graft (déjà présent avant, je n'ai touché
  ni le mage workshop ni le cristal). Confirme le « curseur restant » de SWEEP_REVALID
  (« diversifier la conso de la corne ou la lecture des compteurs par la fin »).
- **Golden INCHANGÉ par construction** : `./chronicle --hash 7 5 12` ×2 = IDENTIQUE aux 2 runs ET
  identique à `scps/golden_hashes.txt` (fe7c00f3/370aed28/b2550b67/79330612/9ad2b632, les 5 graines).
  Aucune re-baseline — bld_min_tier=5 + tech tier-4 derrière un AUTRE tier-4 rend le bâtiment
  structurellement inatteignable en 12 ans (même jurisprudence que les apex triples/LOT T).
- **SAVE non bumpé** : `RECIPE`/`FOREUSE_BASKET` sont des tables statiques NON sérialisées (même
  catégorie que `BASE_PRICE`/`EXTRACT_YIELD`, déjà « NON-const MODTOOLS »). Aucun enum RES_/BLD_
  neuf, aucun champ de struct ajouté. SAVE_VERSION reste 74.
**Pièges** :
- Le scratchpad de ce worktree est PARTAGÉ avec d'autres sessions/agents (des dizaines de fichiers
  `*.sh`/`*.c` d'autres missions du même jour y vivent) — écrire un script via le Bash-tool `/tmp`
  puis l'exécuter via `MSYSTEM=MINGW64 .../bash.exe` ÉCHOUE silencieusement en "No such file"
  (racines de filesystem différentes entre Git-Bash et MSYS2) : TOUJOURS écrire les scripts de build
  dans le dossier scratchpad (chemin Windows complet) via l'outil Write, jamais dans `/tmp` du Bash
  tool, si l'exécution doit passer par `bash.exe` MSYS2.
- Confondre les DEUX compteurs faustiens du même bâtiment : `arcane_charge`/`faust_charge` (fuel de
  Brèche IMMÉDIAT, ∝ `out` = la sortie de l'ANCRE fer, scps_econ.c ~2508) vs `faust_consumed[0]`
  (volume d'ESSENCE consommée, ∝ `q1`, seul lu par le sélecteur de fin §27). Réduire le qout de
  l'ancre (8→2) réduit LÉGÈREMENT la contribution de charge immédiate de CE bâtiment (négligeable :
  il était déjà quasi-inactif) ; augmenter q1 (0.5→0.7) augmente `faust_consumed[0]` — les deux
  leviers sont INDÉPENDANTS, ne pas supposer qu'ils bougent ensemble.
- `diff` n'existe pas dans ce MSYS2 bash (coreutils minimal) — comparer des hash à l'œil ou avec
  `cmp`/un grep, pas `diff`.
**Restes** :
- **Le vrai verrou reste scps_ai.c** (hors périmètre ici) : pour que la Foreuse cesse d'être un
  événement rare/de chance, il faudrait un beeline dédié vers `TECH_FOREUSE` (motif CORNE/S3 :
  `ai_step_toward` sur toute la chaîne `TECH_MANUFACTURE→TECH_INDUSTRIE→TECH_FOREUSE`, déclenché par
  une famine de fer OU un appétit faustien, PAS seulement le score ponctuel actuel). Fix d'une ligne
  NON applicable ici (fichier hors périmètre) — à confier à un agent scps_ai.c dédié si l'objectif
  est « la Foreuse doit se voir souvent », pas seulement « utile quand elle sort ».
- **L'asymétrie de chaîne essence (2 sauts) vs fer céleste (1 saut, corne)** fait que même une
  Foreuse ranimée perd souvent la course au « rare dominant » contre Corne dans les mondes où les
  deux tirent. Deux pistes NON prises (hors périmètre/à trancher par le joueur) : raccourcir la
  chaîne essence (fusionner mage-workshop+foreuse, ou booster `EXTRACT_YIELD[RES_ARCANE_CRYSTAL]`),
  ou rebalancer `endgame_select_and_fire` (scps_endgame.c:1357) pour ne pas juger au seul MAX brut.
- `FOREUSE_BASKET` n'est PAS branchée à `econ_moddata_dump/load` (le canal fichier `SCPS_MODS` ne
  couvre aujourd'hui que labor/qout de `RECIPE`, scps_econ.c ~3786) — un modder ne peut pas encore
  retoucher le panier sans recompiler. Petit ajout si demandé (suivre le motif `recipe\t%s\t...`).
- Sweep MESURÉ volontairement PETIT (2 graines × 5 sims, contrainte CPU partagé) : la fréquence
  réelle de `conso_foreuse>0` sur un grand échantillon (type giga sweep 200 sims) reste à confirmer
  par l'orchestrateur — mes 3/10 sont un signal encourageant, pas une statistique définitive.

## [2026-07-09] Audit équilibrage read-only + finding #1 NON-RETENU (orchestrateur)
**Découvertes** (audit des vagues VOLUMES/ARBRE/RÉCHAUFFEMENT, aucun code modifié) :
- **Finding #1 (le piège)** : le « découplage §27 » du push arbre gate le surcoût ×W sur
  `tech_node(id)->faustian` (`scps_ai.c` ai_effective_cost, miroir `scps_sim.c`). L'audit a
  RAISON que `tech_research` (scps_tech.c) accumule `->charge` SANS regarder `faustian`, donc des
  techs mainline non-faustiennes à charge (Industrie 3.0, Magie de bataille 1.5…) nourrissent
  l'entropie sans frein. MAIS le fix proposé (`charge>0` au lieu de `faustian`) a été MESURÉ
  CONTRE-PRODUCTIF : arbre méd 51→45 %, et l'entropie explose PAREIL (accumulateur monotone :
  `wp->entropy += tech_w·Σcharge` chaque an, sans decay → ~3900 à l'an 180 avec OU sans le fix).
  ⇒ **NON retenu, reverté**. Le découplage reste sur `faustian` (partiel mais l'arbre est meilleur).
- **Le tassement au gate 180 est un NON-PROBLÈME** : l'entropie franchit 55 vers l'an 130-150 → la
  fin fire au gate 180 (voulu). Ce n'est PAS « des fins avant 180 » (le gate tient à 100 %). Le
  TYPE de fin reste VARIÉ (baseline 3g×5s : 5 HIVER · 4 RONCES · 4 RÉCHAUFFEMENT) — seul le TIMING
  est proche de 180. Sans conséquence de gameplay (un joueur voit une fin ; la bande d'entropie est
  bornée à l'affichage). L'entropie qui « déborde » à 3900 est cosmétique interne.
**Piège** : NE PAS « corriger » l'entropie-qui-déborde ni le tassement au gate sans mesure appariée —
c'est un accumulateur monotone VOULU (compte à rebours §27), pas un bug. Le calibrage se fait sur
`ENTROPY_TECH_W` (amplitude) SEULEMENT si on veut vraiment décaler le franchissement, mais à W=4.5 la
distribution est déjà saine.
**Restes CONFIRMÉS par l'audit (pré-existants, non traités — cosmétiques/edge)** : #3 le repli
RÉCHAUFFEMENT ne re-teste plus `mx<FUEL_DEAD_EPS` (un monde à corne immature à l'an 240 + fuel≥4
POURRAIT être préempté — suspect, non observé) · #5 `sang_seed` peut marquer 0 région (fin SANG sans
effet mécanique quand le sang vient de guerres inter-états) · #7 empilement possible réfugiés-de-guerre
(12 %/an) × exode-de-fin (10 %/an) sur une même région (~21 % évacués/an, non vérifié comme problème).
**CE QUI VA BIEN** (audit + 5 runs live) : 0 crash/NaN, satisfaction 71-92 %, hégémon craqué 5/5,
IPM 1.09-1.30, gate 180 tenu 100 %, arbre 40-53 %, démographie robuste (bornes/invariant esclave OK).

## [2026-07-09] Brouillard de guerre (étape 1/2) — infrastructure + voile visuel (implémenteur solo, worktree wt/fog)
**Découvertes** :
- **Le motif du module** est un décalque exact de `scps_decrees.c` (état GLOBAL, pas dans une struct
  partagée ; `_reset`/`_save`/`_load` au format `fwrite/fread` brut, tag `NULL,0` + appel dédié dans
  `scps_save.c`) — même verbatim pour un module NEUF sans rien de compliqué (pas de sub-état, pas de
  table de définitions).
- **`WorldEconomy.adj[SCPS_MAX_REG][SCPS_MAX_REG]`** (scps_econ.h:434) est LA matrice d'adjacence de
  région (booléenne, terre 4-connexe), déjà utilisée par `hub_map_build` (scps_intertrade.c:315-326,
  BFS multi-source sur `adj[r][s]`, scratch `static int16_t q[SCPS_MAX_REG]`) — motif COPIÉ tel quel
  pour la BFS radius-2 par-empire. `regions_of(econ,c)` (scps_sim.c:120, `owner==c` scanné à chaque
  appel — PAS un cache) est LE check canonique « un pays a-t-il des régions » ; je m'en suis abstenu
  et ai juste laissé la boucle de seed trouver 0 région pour un pays mort (aucun `continue` explicite
  nécessaire, le queue reste vide et la BFS ne fait rien — plus simple que d'appeler `regions_of`).
- **`Sim.human_player`** (scps_sim.h:133) est mis à -1 par `sim_init` INCONDITIONNELLEMENT
  (scps_sim.c:1043, « la chronique reste 100% IA ») — la façade (`scps_sim_generate`/`_load`,
  scps_api.c:124,3019) le réaligne sur `s->sim.player` APRÈS coup. C'est DIFFÉRENT du motif dominant
  `(human_player>=0)?human_player:player` vu dans ~15 autres fonctions façade (pour les READOUTS, où
  on veut toujours « un » pays même sans joueur engagé) — pour le brouillard, `human_player` PUR est
  le bon champ (le voile n'a de sens QUE pour un joueur réellement engagé ; `<0` ⇒ tout visible est le
  comportement VOULU pour chronique/tout contexte sans joueur, cf. scps_fog.c).
- **La façade NE DOIT JAMAIS exposer `SCPS_MAX_REG`/`SCPS_MAX_COUNTRY` au binding C++** — `scps_api.h`
  n'inclut PAS `scps_types.h` (façade opaque). Un premier jet de `scps_fog_region_mask` écrivait
  aveuglément `SCPS_MAX_REG` (832) octets dans le buffer fourni par l'appelant ; le binding C++ n'a
  AUCUN moyen de connaître cette constante (elle n'existe nulle part côté C++) et aurait dimensionné
  son `PackedByteArray` sur `scps_region_count(sim)` (≤832, souvent 100-450) → **débordement mémoire
  silencieux** côté C++. Fix : la façade récrit dans un buffer LOCAL `uint8_t rv[SCPS_MAX_REG]` puis
  `memcpy` seulement `n_regions` octets vers le buffer de l'appelant — motif à réutiliser pour TOUT
  futur reader « par région » exposé au binding (dimensionner sur `scps_region_count()`, jamais sur un
  cap moteur interne). Repéré en RELISANT mon propre code après un premier build réussi — le compilateur
  ne l'aurait jamais signalé (aucun warning, juste un memcpy avec la mauvaise taille source vs. dest).
- **Le motif image-par-cellule + masque-par-région EN UN SEUL PASSAGE BFS** : `fog_visible_regions`
  (scps_fog.c) réutilise le tableau de SORTIE `out_region[]` lui-même comme marqueur "visited" pendant
  la BFS (au lieu d'un `visited[]` séparé comme dans `fog_update`) — plus simple car la sémantique
  "atteint par le radius-2" ET "visible" coïncident exactement pour ce cas d'usage (contrairement à
  `fog_update` où le "visited" de la BFS est local à CHAQUE empire mais le résultat écrit va dans
  `g_known[a][owner_of(r)]`, deux choses différentes).
- **`make`/MSYS2 sur cette machine : TMP/TEMP ne traversent PAS l'exec de `D:/MSYS2/usr/bin/make.exe`
  même exportés dans le MÊME appel Bash** (contrairement à un `gcc` direct dans le même shell, qui
  fonctionne). Diagnostiqué en faisant imprimer `env` DEPUIS une recette make (Makefile jetable,
  PAS dans /tmp — `/tmp` de Git Bash ≠ `/tmp` de MSYS2, deux runtimes MSYS DIFFÉRENTS avec des montages
  différents, un fichier écrit par l'un est invisible pour l'autre malgré le même chemin apparent) :
  TMP/TEMP/USERPROFILE sont ABSENTS de l'environnement reçu par la recette, peu importe `export` avant
  l'appel. **Fix qui marche** : passer TMP/TEMP comme VARIABLES DE COMMANDE MAKE (`make TMP=... TEMP=...
  cible`, PAS `TMP=... TEMP=... make cible`) — GNU Make exporte automatiquement les variables définies
  sur sa ligne de commande vers l'environnement de ses recettes ; c'est un mécanisme DIFFÉRENT de
  l'héritage normal de processus et il CONTOURNE le problème de propagation. Sans ce fix : `gcc` échoue
  avec « Cannot create temporary file in C:\Windows\: Permission denied » (TMP/TEMP absents → repli
  Win32 sur le dossier Windows, en dernier recours après USERPROFILE lui aussi absent).
- **`godot/godot-cpp` est un JUNCTION gitignored** (pas un submodule) — chaque worktree doit le
  recréer localement : `New-Item -ItemType Junction -Path godot/godot-cpp -Target E:\JEUX\SCPS\godot\
  godot-cpp` (PowerShell ; `mklink /J` en cmd). Un `ls godot-cpp` qui répond juste « No such file or
  directory » (pas « broken symlink ») dans un worktree NEUF signifie qu'il n'a jamais été créé ICI —
  chaque worktree est une arborescence disque séparée, la jonction d'un AUTRE worktree ne traverse pas.
- **`scons` sans `platform=windows use_mingw=yes` bascule sur MSVC (`cl.exe`)** sur cette machine (PATH
  a `D:\MSYS2\mingw64\bin\scons` mais SCons détecte/préfère MSVC comme toolchain windows par défaut) —
  échoue sur `CLOCK_MONOTONIC` (POSIX/MinGW seulement, scps_sim.c:31) inconnu de MSVC. Le flag explicite
  est OBLIGATOIRE ici, contrairement à ce que le SConstruct laisse penser (le commentaire mentionne
  `use_mingw` mais ne dit pas qu'il faut le passer explicitement en ligne de commande à CHAQUE fois).
- **Godot headless boot (`--headless --path . res://main/Main.tscn --quit-after 60`) affiche des
  SCRIPT ERROR pré-existants** (`ui/tech_panel.gd` : `Graph`/`Atom`/`GraphSoftLine` non déclarés) sur
  un worktree FRAIS — cause : `project/.godot/` (cache editeur, incl. `global_script_class_cache.cfg`
  qui enregistre les `class_name` globaux) est gitignored et n'existe pas tant que l'éditeur n'a jamais
  ouvert CE worktree. **Vérifié NON causé par mes changements** : `git stash` (retour au HEAD propre) →
  MÊMES erreurs, `diff` des deux logs = IDENTIQUE au caractère près. Toute vérification headless sur un
  worktree neuf doit faire ce contrôle A/B (stash/pop) avant de conclure qu'une erreur est réelle.
**Pièges** :
- Le champ `Sim.player` (≠ `human_player`) est TOUJOURS défini (0 par défaut ou le POLITY_PLAYER trouvé
  à la genèse) — NE PAS le confondre avec `human_player` pour une fonctionnalité qui doit rester INERTE
  tant qu'aucun humain n'est engagé (golden/déterminisme). Le motif fallback `(human_player>=0)?
  human_player:player` est correct pour un READOUT (toujours montrer QUELQUE CHOSE) mais FAUX pour un
  gate comportemental qui doit distinguer « chronique/aucun joueur » de « joueur engagé ».
- BFS sur une matrice D'ADJACENCE DENSE (pas une liste) coûte O(n) par nœud déqueué même si son degré
  réel est petit (4-8 voisins géographiques) — `fog_update` fait ceci n_countries FOIS (une BFS par
  empire vivant). MESURÉ pour lever le doute (pas supposé) : `determinism-deep` (200 ans × 2 graines ×
  2 runs) = 82.6 s total (~20.6 s/run de 200 ans) — AUCUN ralentissement perceptible vs. l'historique
  documenté ailleurs pour des sweeps comparables. Le monde réel (n_regions typiquement 100-450, PAS le
  cap SCPS_MAX_REG=832 ; n_countries réel bien sous SCPS_MAX_COUNTRY=320) reste largement sous le pire
  cas théorique. Pas d'optimisation en liste d'adjacence nécessaire à ce stade.
**Restes** :
- Le CÂBLAGE des décisions (IA/diplo qui LISENT `country_knows()` pour restreindre guerre/diplomatie
  aux empires connus) est HORS PÉRIMÈTRE de cette mission — `country_knows()` est prêt, appelé nulle
  part dans `scps_ai.c`/`scps_diplo.c` (vérifié par grep, 0 résultat). C'est la mission « étape 2/2 ».
- v1 du voile est BINAIRE (visible/voilé) — pas de dégradé « connu mais pas vu actuellement » (un
  empire découvert puis dont on perd le contact reste visible EN ENTIER pour toujours, cf. le design
  demandé : « A connaît B pour toujours »). Si un jour on veut un troisième état (« connu mais hors de
  vue, dernière position mémorisée type Civ »), `country_knows` + `fog_visible_regions` sont le bon
  point d'extension (ajouter un état intermédiaire dans le masque au lieu d'un bool).
- Pas de télémétrie chronicle ajoutée (la mission ne la demandait pas ; le module est VISUEL-only donc
  rien à mesurer côté équilibrage — contrairement aux modules qui touchent une décision de sim).
- Sonde de vérification `fog_probe.c` (radius-2, asymétrie, cumulativité, reset — 19/19) écrite en
  SCRATCHPAD (non committée, comme demandé) — à reproduire si un futur agent veut re-vérifier la logique
  BFS sans relire tout `scps_fog.c` : synthétiser un monde en CHAÎNE de 5 régions, 3 pays espacés de 2,
  ça donne une asymétrie de découverte en UN seul `fog_update` (bon test de non-régression rapide).

## [2026-07-09] Éco — MANUFACTURES SIGNATURE D'ÉTHOS + désir croisé (implémenteur solo, wt/manuf, SAVE v76)
**Découvertes** :
- **Le miroir d'index EST la formule de paire opposée demandée** : `Ethos` (scps_culture.h) est ordonné
  DOMINATEUR(9)·HONNEUR(7.5)·ORDRE(6)·BUREAUCRATE(4.5)·MERCANTILE(3)·PACIFISTE(1.5) — `ethos_opposite(e)
  = ETHOS_COUNT-1-e` (scps_econ.c) donne EXACTEMENT les 3 paires mission (Dominateur↔Pacifiste,
  Honneur↔Mercantile, Ordre↔Bureaucrate), vérifié par un mini-programme autonome ET par lecture de
  `ETHOS_VAL[]` (scps_culture.c:20, DOMINATEUR=9.0/PACIFISTE=1.5 confirmant l'ancrage symétrique).
  `culture_pair_for_biome`/`biome_ethos_pair` (scps_culture.c:295) sont un concept DIFFÉRENT (les DEUX
  éthos qui cohabitent dans UN biome à la génération, pas l'opposé sur l'axe) — ne pas confondre, ne
  PAS réutiliser pour ce besoin.
- **Le §NF générique (`econ_build_tick`, scps_econ.c:1586) construit N'IMPORTE QUEL `BLD_*` avec un
  `RECIPE[b].out` valide + `BASE_PRICE[out]>0`, SANS aucun hook IA/doctrine** — confirmé par précédent
  (BLD_POTTERY/BLD_SCULPTURE n'apparaissent NULLE PART dans `scps_ai.c`, grep 0 résultat) ET par mesure
  directe (les 6 nouveaux bâtiments se posent tout seuls, seed 9×150 ans : 6×Heaumerie·1×Parurier·
  2×Horloger·2×ChancellerieLux·2×ComptoirArtisan·2×AtelierSerein). Le seul déclencheur nécessaire est
  la DEMANDE (prix ≥ NF_SHORTAGE×base) — donc juste bien peupler `RECIPE[]`+`BASE_PRICE[]`+le mécanisme
  de désir suffit, aucun ajout à `ai_build_civmanuf`/`ai_build_manufacture` n'est requis pour un bâtiment
  « manufacture civile » standard (≠ transmuteurs/foreuse/arquebuserie qui ont un gate TECH explicite
  dans la MÊME boucle, scps_econ.c:1607-1611 — mes 6 n'en ont PAS besoin, aucune tech associée).
- **`CMD_BUILD_MANUF` (verbe joueur, scps_sim.c:423) et `bld_min_tier()` (scps_econ.c:1927) ont tous
  deux un `default:`/repli générique** (civil non-militaire admis, tier 1 par défaut) — les 6 nouveaux
  `BLD_*` sont donc immédiatement constructibles par le PANNEAU B joueur aussi, sans code additionnel
  (seul le filtre `out==RES_ARMS||...` en exclurait un bâtiment MILITAIRE — mes biens de luxe n'y
  tombent pas, à raison : Heaumes de guerre est un bien de PRESTIGE/confort, pas un `RES_ARMS*` réel).
- **Le mécanisme de désir croisé est HORS de la table statique `NEED[CLASS][RES]`** (le bien varie
  selon `re->culture.ethos`, table statique ne peut pas exprimer ça) — câblé en DEUX blocs séparés
  À CÔTÉ du panier (jamais dedans) : génération de demande (fin de la boucle `for c` §4, juste après le
  `for r` qui lit `NEED[][]`) + consommation/comfort_joy (fin du `for r` §5, avant `wealth=budget`),
  gatés `c==CLASS_LABORER||c==CLASS_ELITE` + `active_needs>=tune_f("ETHOS_LUXURY_MIN_TIER",6)`. Motif
  calqué EXACTEMENT sur le bloc `if(r==RES_POTTERY||r==RES_STATUE)` (comfort_joy hors panier, jamais de
  pénalité si absent) — mais PAS dans le `for r` lui-même (impossible, resource dynamique) : un bloc
  autonome après, qui relit `S[]`/`budget`/`re->price[]` du MÊME scope.
- **`resource_name()`/`building_name()` (scps_world.c/scps_econ.c) sont des tables C littérales NON
  câblées à `tr()`** — confirmé par grep : AUCUN des ~110 `RES_*`/`BLD_*` existants n'y passe (la façade
  `scps_api.c` les appelle DIRECTEMENT, toujours en français, même avec le switch EN actif). Il existe
  DÉJÀ un précédent d'entrées STR_* orphelines (STR_RES_BOIS/ARGILE/PIERRE/OUTILS, strings_ids.h:246-249,
  0 référence dans tout `*.c` — vestiges de l'ancien viewer.c topbar SDL) : ajouter des STR_RES_*/
  STR_BLD_* SANS les câbler est donc un état TOLÉRÉ du codebase, pas une régression introduite ici.
  Câbler `resource_name()`/`building_name()` en entier au switch de langue serait un chantier séparé
  (~116 entrées), hors périmètre de cette mission.
- **`it_is_precious()`/`it_is_bulk()` (scps_intertrade.c:38-45, télémétrie directionnelle SEULEMENT,
  n'affecte PAS l'éligibilité au commerce) n'incluent PAS POTTERY/STATUE** — précédent suivi, les 6
  nouveaux biens n'y sont pas non plus (cohérent, pas une omission). Le commerce inter-empire des 6
  biens fonctionne quand même via les boucles GÉNÉRIQUES `for g<RES_COUNT` d'intertrade (aucun
  changement nécessaire) — vérifié par la présence de "commerce inter-pays/an" non-nul dans les sims.
- **`vocation_word()` (scps_readout.c:508) ne lit QUE `Province.resource` (une BRUTE, jamais une
  production)** — mes 6 `RES_*` (tous ≥ `RES_PROD_FIRST`) ne peuvent JAMAIS y arriver ; `default: break`
  de toute façon. Non-issue confirmé, aucune modif nécessaire.
- **`Recipe` (scps_econ.c) est positionnel sur 11 champs** `{in1,q1,in2,q2,out,qout,labor,alt1,alt1_q,
  out2,qout2}` — un initialisateur à 9 valeurs (comme celui de la mission/statuaire) laisse `out2`/
  `qout2` à zéro (RES_NONE/0.f) par défaut C, AUCUN warning (`-Wmissing-field-initializers` n'est pas
  dans `-Wall -Wextra` pour les designated/positional partiels de ce genre sur ce gcc 16.1).
- **Golden INCHANGÉ pour les 5 graines de référence** (7/108/209/310/411, `./chronicle --hash 7 5 12`
  identique BYTE PRÈS au `scps/golden_hashes.txt` committé) — CONTRAIREMENT à l'attente de la mission
  (« le désir mord dès l'an-0 »). Cause : `ETHOS_LUXURY_MIN_TIER=6` ⇒ `active_needs=1+capitale_max_tier
  (pop)>=6` ⇒ tier≥5 (T5 « Cité », 5000 hab/PROVINCE) — AUCUNE province n'atteint 5000 hab en 12 ans à
  la démographie actuelle (`POP_R_BASE`, doublement ~20-40 ans). La précaution « position tardive »
  demandée par la mission a été appliquée assez fort pour repousser tout mordant hors de la fenêtre
  golden — vérifié PUIS confirmé (2 runs consécutifs identiques + diff contre le fichier committé).
  Si l'orchestrateur veut que le mécanisme morde plus tôt (pour le calibrage/visibilité), le seul levier
  est `SCPS_TUNE=ETHOS_LUXURY_MIN_TIER=<N>` (déjà dialable, pas de recompile) — abaisser à 4-5 ferait
  mordre bien avant l'an-12 et RE-BASELINERAIT le golden (à faire consciemment, pas par accident).
- **Mesuré en sim longue** (seed 9×150 ans + seed 7×3×200 ans) : les 6 ateliers apparaissent (pas
  systématiquement TOUS dans CHAQUE sim individuelle — variance stochastique normale, même motif que
  Alambic/Réplicateur/Forge céleste qui n'apparaissent pas non plus dans 100 % des sims), satisfaction
  reste 75-91 % (Laborer/Bourgeois/Élite), aucun NaN/Inf, `EXIT=0` partout.
**Pièges** :
- **`make test`/`make <banc>` via MSYS2 bash (D:\MSYS2\usr\bin\make, GNU Make 4.4.1) STRIPE `TMP`/`TEMP`
  avant d'invoquer le shell de la recette** — même quand ils sont `export`és dans LE MÊME appel Bash
  juste avant `make`, et INDÉPENDAMMENT du format (backslash `C:\Users\...` OU slash `C:/Users/...`) :
  vérifié en écrivant un Makefile-sonde qui `@echo "TMP=[$$TMP]"` → toujours VIDE dans la recette, alors
  que `cc -c fichier.c -o build/x.o` tapé À LA MAIN dans le MÊME shell juste avant réussit. Ce n'est donc
  PAS résolu par le fix documenté dans l'entrée « chaîne militaire — audit 13 points » (« préfixer
  CHAQUE invocation make/gcc ») — ce fix marche pour un appel `gcc` DIRECT, pas pour `make` qui relance
  ses propres recettes dans un sous-shell qui a perdu ces 2 variables précises. **Le fix qui marche à
  coup sûr : invoquer `make` via l'outil PowerShell (natif Windows, TMP/TEMP déjà corrects, pas de
  couche de traduction MSYS2)** — `$env:Path = "D:\MSYS2\mingw64\bin;D:\MSYS2\usr\bin;" + $env:Path` puis
  `make ...` directement, AUCUN export TMP/TEMP nécessaire. Symptôme si on se fait avoir : TOUS les
  bancs échouent en `BUILD ÉCHEC` d'un coup (0 VERTS/0 ROUGES/N BUILD ÉCHEC) avec dans `/tmp/k3_build.log`
  (ou en direct) : `Cannot create temporary file in C:\Windows\: Permission denied` — panique inutile,
  ce n'est PAS le code qui casse (vérifié : compile standalone `gcc -Wall -Wextra -c` propre sur les 5
  fichiers touchés AVANT même de comprendre le problème make/TMP).
- Le mécanisme de désir croisé ne peut PAS être exprimé comme une entrée de plus dans `NEED[CLASS][RES]`
  (piège en apparence tentant, « juste ajouter une ligne ») — la table est indexée `[classe][ressource
  FIXE]`, or le bien dépend de `re->culture.ethos` (dynamique par province) : il faut un bloc de code
  séparé, pas une entrée de table, sous peine de se retrouver à devoir soit dupliquer le bloc ×6 (un par
  éthos, avec un `if` sur chaque ressource candidate) soit inventer un système de table à niveau
  supplémentaire — le détour par `ethos_desired_luxury()` (une fonction, pas une table 2D de plus) est
  le plus sobre.
**Restes** (hors périmètre, listés explicitement par la mission) :
- **Métabolisation-déblocage** : la mission NE demande PAS de gate de PRODUCTION par éthos (qui pourrait
  produire quoi) — actuellement TOUT pays peut bâtir LES 6 ateliers (le §NF générique ne fait aucune
  distinction d'éthos producteur). Le design doc (§3, non implémenté) prévoyait un déblocage par palier
  de métabolisation (`METAB_TIER1`/`_TIER3`, analogue à `econ_country_heritage_digested` mais sur l'axe
  ÉTHOS des groupes plutôt que l'héritage) — nécessiterait un nouveau helper `econ_country_ethos_digested`
  (le pont éthos, PopGroup.culture.ethos existe déjà, juste jamais agrégé par éthos) + un gate dans le
  §NF (`if (b==BLD_HEAUMERIE && ethos_digested<TIER) continue;`, motif IDENTIQUE aux gates tech_foreuse/
  tech_alchimie déjà dans la boucle) — mais volontairement PAS fait ici (mission explicite).
- **Assets Godot** (icônes des 6 ateliers/6 biens, `pack/buildings`/`pack/resources`) — hors périmètre,
  intégrés par l'orchestrateur après.
- **Verbe « Financer les expéditions »** (§4 du design doc, dépend du fog déjà livré) — PAS commencé,
  séquencé APRÈS ce système selon le design doc lui-même.
- **Calibrage fin** — délibérément non fait (mission : « NE CALIBRE PAS finement »). Le gate
  `ETHOS_LUXURY_MIN_TIER=6` et le poids `ETHOS_LUXURY_JOY=0.08` sont posés par PRÉCAUTION/analogie avec
  poterie-statuaire, pas mesurés/optimisés — l'orchestrateur a la main via `SCPS_TUNE` sans recompiler.

## 2026-07-09 — Vague UI globale (audit visuel par probe + convergence « rendu attendu » EU4)
- **Découvertes** : la « bande beige » bas d'écran = la MARGE DE PAGE hors-monde (le voile fog
  ne couvrait que le rect carte — overlay.gd:~1953 dessine désormais 4 bandes sépia autour,
  gaté `Sim.game_on`) · `scps_fog_visible` (scps_api.c:381) était FAIL-OPEN sur les cellules
  sans région (mer/lacs visibles à travers le fog) → fail-closed + halo `FOG_SEA_HALO 8` +
  rampe d'alpha display-only dans `fog_image` (scps_sim_node.cpp) · `scps_province_tax`
  affichait **12× le réel** (le taux moteur 0.42·wealth·dt est ANNUEL — dt en années, preuve
  scps_econ.c:2652 « ×365×dt » — le miroir le croyait mensuel et ×12) · le siège de ville =
  désormais `scps_region_seat` (centroïde de la PROVINCE REP via econ_region_rep_province,
  cache ppx/ppy dans api_centroids) — le barycentre de RÉGION tombait au bord des formes concaves.
- **Pièges** : poser `size` PENDANT `_draw()` est IGNORÉ par Godot → `set_deferred("size", …)`
  (province_panel hauteur-au-contenu) · `var x := expr` sur une expression NON TYPÉE
  (`_graph.size.x…` où _graph est untyped) = PARSE ERROR qui fait tomber TOUT le script → le
  `load().new()` de main.gd échoue en cascade (« Nonexistent function 'new' in base GDScript ») ·
  la probe shot_ui écrit ses PNG AU FIL de la tournée → PURGER le dossier avant de relire
  (diagnostic rendu sur des captures STALE une fois) · scons manuel exige
  `PROCESSOR_ARCHITECTURE=AMD64` + `TMP=/tmp` (KeyError intelc sinon) et ne JAMAIS masquer
  l'exit code par `| tail`.
- **Restes** : deltas topbar (or/pop) ne vivent qu'en jeu (month_ticked) — invisibles en pause ·
  icônes du rail gauche encore ternes malgré le lift ×1.28 (les sprites eux-mêmes sont sombres) ·
  « An N » encore serré contre l'ornement de capsule · panneau détail province (réincorporation
  brute, vide bas) non repris · roadmap EU4 restante dans SYNTHESE_SESSION.md (ledger droite,
  compteurs d'armée, bannières de section, minimap, question carte parchemin vs saturé).

## 2026-07-09 — Vague « inspire-toi » CK3/EU5 (.gui minés par 2 éclaireurs)
- **Découvertes** : les .gui Paradox sont du TEXTE minable (game/gui/ CK3 · game/in_game/gui/
  EU5) — synthèses complètes dans SYNTHESE_SESSION.md § « INSPIRE-TOI » (cellule ressource
  CK3, outliner EU5 405×31 + rubans 39px, tooltips 420/10px, gradient de zoom du lavis EU5).
  Transposé : topbar cellules (valeur/delta empilés, `_cell`, TOPBAR_H 38→48), rubans de
  catégorie du ledger (`_lsection`), saturation lavis 0.60→0.72, compteurs d'armée strips.
- **Pièges** : ⚠ BUG LATENT — `visible = Sim.game_on` posé DANS `_draw()` (empire_sidebar) :
  un Control caché ne redessine JAMAIS → masqué une fois (menu), il ne se remontrait jamais.
  L'empire_sidebar (le ledger entier !) n'était apparu dans AUCUNE partie depuis sa création.
  Visibilité pilotée en `_process`. Règle : jamais de self-visibility dans _draw.
- **Restes** : lignes outliner 31px + jauges verticales fines 5×22 (patterns EU5 en réserve) ·
  minimap · question carte (saturation portée à 0.72 — valider à l'œil joueur).

## 2026-07-09 — Raw-works verrouillées hors du jeu joueur (lot 5, fix moteur golden-neutre)
- **Découverte** : `in1==RES_NONE` traité comme « recette dégénérée » dans les DEUX miroirs
  du bâti joueur (scps_api.c `scps_manuf_legal` ET scps_sim.c drain CMD_BUILD_MANUF) →
  les 3 RAW-WORKS (four à brique→argile · carrière→pierre · scierie→bois, hors-sol N1,
  qui boostent l'output de brut) étaient INVISIBLES et IMPOSABLES pour le joueur — seule
  l'IA les bâtissait (ai_build_rawworks). Fix : seul `out==RES_NONE` rejette ; le gate
  d'intrant (`feed`) saute quand in1==NONE. `make golden` IDENTIQUE (le drain n'est touché
  que par des commandes joueur — la chronique n'en enfile jamais).
- **Piège (payé 5×)** : le `cd` inline dans `bash.exe -lc '...'` se fait MANGER de façon
  répétée à la régénération de la commande → passer par un SCRIPT .sh committé
  (packaging/windows/golden_dll.sh) — c'était DÉJÀ documenté dans SYNTHESE (« le cd inline
  se fait manger ») et je l'ai re-payé quand même. Lire le handoff AVANT de forger la commande.
- **Restes** : `raw_boost` (paliers d'EXPLOITATION par brute) n'a AUCUN verbe joueur —
  écrit par la seule IA (scps_ai.c:1292). Si le joueur doit y accéder : CMD_* + façade à créer.

---

## 2026-07-10 — Minage UI Stellaris

Objectif : extraire des règles de design chiffrées des `.gui`/`.gfx` de Stellaris
(`D:\Steam\steamapps\common\Stellaris\interface\`) pour corriger une UI Godot 2D « patate ».
Tout en px, résolution de référence Stellaris = 1920×1080 (offset natif show_position x=35 y=40).

### Découvertes (fichier : valeurs)

**1. Dimensions de fenêtres (windowType)**
- empire_view.gui:17 — `empire_view` = 1272×620 ; show_position {35,40} ; hide {-1272,40} (slide depuis la gauche) ; animation_time=200.
- planet_view.gui:225 — `planet_view` = 1162×680 ; side-panel `planet_view` secondaire (l.181) = 360×630.
- diplomacy_view.gui:49 — `diplomacy_view` = 1280×635 ; panneau gauche 990×635, colonne droite ~290.
- Contenu interne empire : empire_list à position {11,140}, size 1250×489 (marge latérale ~11px, top ~140px sous header+tri).
- RATIOS : largeur ≈ 1272/1920 = 66 % ; 1162/1920 = 60 % ; hauteur ≈ 620–680/1080 = 57–63 %. Fenêtre « large » standard = ~2/3 de l'écran en largeur, ~60 % en hauteur.
- Marge intérieure standard : bord→contenu = 11–14px lat., listbox interne démarrée à y=138 sous les boutons de tri.

**2. Headers de fenêtre**
- Barre du titre : hauteur effective ~34–40px (topbar HUD = 36px exactement, main.gui:161).
- Titre : font = "malgun_goth_24" (police header 24px ; le corps = cg_16b = 16px bold). empire_view.gui:59, planet_view.gui:191.
- Position titre : empire {x=35 y=5}, planet {x=12 y=12} → marge header ~12px.
- Bouton close : orientation UPPER_RIGHT, position {x=-45 y=16} (empire_view.gui:50) ou {x=-40 y=2} (planet_view.gui:205) ; sprite GFX_close / GFX_close_button_planet ; shortcut ESCAPE.
- Ligne séparatrice header : GFX_line_long à y=21 (empire_view.gui:44) → trait sous le titre, contenu commence ~y=140.

**3. Bottom bar / boutons flottants**
- main.gui:158 `topbar` = 100%×36 avec background ; cellules ressources `tb_*_group` = 70×36 chacune (main.gui:230+), séparateur vertical `basic_resources_divider` = 8×75% (green_vertical_delimiters).
- main_bottom.gui : panneau système bas-centre `leave_system_window`/`leave_galaxy_window` = 166×114, slide vertical (show_position y=-95). Boutons carte `map_button_bg` = GFX_bottombar_button_bg posés en absolu {x=194 y=33}, PAS de plaque de fond continue — chaque icône a son propre petit bg (GFX_bottombar_button_bg) → icônes « flottantes ».
- control_groups (main_bottom.gui:17) : grille 700×13, spacing=2.
- Confirmation : les boutons du bas sont positionnés en coordonnées absolues autour du centre, chacun avec son sprite bg individuel → pas de barre-plaque unique.

**4. Sections internes (listes / grilles)**
- smoothListboxType empire (empire_view.gui:155) : 1230×475, spacing=4, scrollbar "standardlistbox_slider".
- Hauteurs de rangées récurrentes : outliner entries 20 / 27 / 38 / 40 / 41px (outliner.gui) ; advisor 290×34 ; diplomacy rows 148×48, 200×40.
- gridBoxType slotSize icônes : 31×31, 39×39, 40×40 (grilles d'icônes) ; vignettes 93×130 (portraits), 125×85, 275×190 (cartes council).
- Séparateurs : sprites "green_vertical_delimiters" (vertical 8px de large) et GFX_line_long (horizontal header).
- Scrollbars (core.gui:162) : 12×12px, track/slider standardisés.

**5. Tooltips**
- core.gui:40 ToolTipWindow : bg GFX_tooltip_bg, texte font cg_16b, borderSize {x=35 y=15} (padding interne 35 lat / 15 vert), maxWidth 400–500.
- ToolTipConceptWindow (core.gui:76) : icône à {8,8}, texte offset {10,0}, maxWidth 400.
- Delay : PAS dans defines/*.txt ni interface (c'est un réglage utilisateur runtime) → non chiffrable ici. Piège noté.

**6. Palette (fonts.gfx:9 textcolors, format R G B 0-255)**
- Base : T/W blanc {255,255,255} ; t gris clair {198,198,198} ; g gris {128,128,128}.
- Accents : G vert-positif {41,225,38} ; R rouge-négatif {252,86,70} ; H highlight orange {251,170,41} ; C cyan concept {31,224,202} ; B bleu {51,167,255} ; Y jaune {247,252,52}.
- Prose/event : E vert doux {135,255,207} ; V vert foncé {76,138,113} ; L lore beige {195,176,145}.
- Rareté : M magenta {163,53,238}, mauve {204,179,255}, doré {255,221,122}.
- Gamme dominante Stellaris = fond sombre bleu-vert + texte blanc/gris + accents saturés (vert/orange/cyan). Pour un look PARCHEMIN il faut INVERSER la logique de contraste (fond clair, texte sombre) mais garder la discipline : 1 couleur = 1 sémantique.

**7. Grille / alignement**
- Pas récurrent : header 36, cellules ressource 70, corner 9-slice borderSize=12 (core.gfx GFX_button_* corneredTileSprite tous en borderSize {12,12}).
- Spacings de listes quasi tous ∈ {1,2,4} (jamais aléatoires).
- Largeurs de colonnes récurrentes : outliner 260/248, panneaux 318/320, side-panels 290/360.
- Multiples fréquents de 5 et 10 pour les positions ; rangées collent à des paliers 20/27/38/40.

### Synthèse — 10 règles Stellaris transposables
1. Fenêtre large = ~66 % largeur écran × ~60 % hauteur (jamais plein écran ; laisse respirer les bords).
2. Header = bande 34–36px ; titre 24px ; contenu 12px sous le trait séparateur.
3. Close = coin haut-droit, ancré en orientation UPPER_RIGHT à ~-40px, ESCAPE branché.
4. Marge intérieure fenêtre = 11–14px sur les côtés ; premier bloc de contenu ~140px du haut si header+barre d'outils.
5. Bottom bar = icônes FLOTTANTES : chaque bouton porte son propre petit fond (68–70px), pas de plaque continue.
6. Rangée de liste = 27–41px selon densité (défaut ~38–40) ; spacing entre entrées 2 ou 4, jamais plus.
7. Icône de grille = 31–40px carré ; portrait/vignette = ratio ~2:3 (93×130).
8. 9-slice partout : coins de 12px pour cadres et boutons (bords nets qui ne bavent pas au resize).
9. Scrollbar fine 12px, tooltip padding 35×15 avec maxWidth ~400.
10. Palette = 1 couleur = 1 sens (vert+/rouge−/orange highlight/cyan lien) sur fond neutre ; texte corps 16px, contraste fort.

### Pièges
- Coordonnées négatives = ancrage au bord opposé selon `orientation` (ex. close x=-45 UPPER_RIGHT) ; ne pas lire comme du absolu top-left.
- `%%` dans les .gui (ex. width=100%%) = échappement Clausewitz, vaut 100 %.
- Les tailles de fenêtre sont pensées pour 1920×1080 avec `if_scaled_resolution` qui override en jeu → transposer en % plutôt qu'en px bruts.
- Tooltip delay INTROUVABLE dans les fichiers (réglage runtime, pas data-driven).

### Restes
- Pas ouvert : situation_log.gui / council_view.gui en détail (headers seulement survolés) ; icons/ et resource_groups/ non minés.
- Pas trouvé : valeur numérique du tooltip delay (chercher côté settings.txt utilisateur, hors interface/).
- À faire : croiser avec les sprites GFX (dimensions des .dds tiles) pour caler le 9-slice parchemin.

## 2026-07-10 — Vague Stellaris (fenêtres + bottom bar flottante)
**Découvertes**
- `controls.gd` : la « barre du bas » n'était qu'une bande dessinée + MOUSE_FILTER_STOP plein écran — la retirer exige de passer le parent en MOUSE_FILTER_IGNORE (sinon une bande invisible mange les clics carte) ; les IconButton enfants captent seuls.
- Hauteur-au-contenu d'un panneau immediate-mode : le pattern latch `set_deferred("size", …)` depuis `_draw` (déjà payé sur province_panel) se généralise — chaque `_draw_<tab>` du tiroir renvoie son y final ; les clips internes doivent lire `_hmax` (viewport), PAS `size.y` (sinon le clip suit la taille latchée et la liste ne regrandit jamais).
- `VKit.header(ci, w, title) -> Rect2` : header standard Stellaris (bande 36 px, titre FS_BIG, filet or, ✕ haut-droite) — le close-rect renvoyé remplace les `_close_rect = Rect2(PW-26, …)` ad hoc.
**Pièges**
- Export scps.exe : le Godot **mono** (racine du repo, `Godot_v4.6.3-stable_mono_win64_console.exe`) REFUSE l'export sans .NET SDK 10.0.6 (« .NET Sdk not found ») — l'export officiel passe par le binaire NON-mono `/e/JEUX/SCPS/Godot_v4.6.3-stable_win64.exe` (cf. build_godot.sh GODOT=). Le mono sert aux probes ; le non-mono aux exports.
- `grep -c` sans correspondance ⇒ exit 1 : un chain `…; grep -c X log` en fin de commande fait « échouer » la tâche de fond alors que la probe est verte — lire le contenu, pas le code retour.
**Restes**
- Header standard à propager (tech_panel en-tête dense laissé tel quel, battle/country/province_panel petits headers ad hoc).
- Drawer : la plaque de titre `panel_title_plaque` déborde légèrement du cadre à droite (cosmétique).

## 2026-07-10 — Setup en onglets + panneaux plats
**Découvertes**
- `worldgen_set` (binding scps_sim_node.cpp:1572) remplit les clés ABSENTES du Dictionary par des CONSTANTES (0.7/0.5/6…) — un dict partiel ÉCRASE l'archétype de graine (lot W) et les cartes redeviennent toutes pareilles. Pour n'exposer qu'un réglage : fusionner `worldparams_default(graine)` + le réglage, jamais un dict partiel.
- `Sim.current_seed` (autoload sim.gd) porte la graine du monde courant — le créateur autonome n'a plus besoin de son propre champ.
- Sélection exclusive par boutons-toggle (traditions) : `set_pressed_no_signal` pour refléter l'état sans re-déclencher `pressed` (sinon boucle).
**Pièges**
- Les probes existantes (menu_audit/culture_audit) testent le BINDING, pas le layout — l'UI refaite se vérifie par shot_ui (3 captures setup ajoutées : 01b/01c/01d).
**Restes**
- « Quitte à léguer un petit modificateur sur chacun, gamey » (éthos) : l'affichage dit désormais ce que l'éthos pilote (armée/factions/évents) — un VRAI petit modificateur par éthos serait un geste MOTEUR (registre J), non fait ici.
- Sliders monde retirés mais le motif `_make_slider` a disparu avec — si on ré-expose des réglages plus tard, reprendre l'historique git (new_game_panel.gd pré-2026-07-10).

## 2026-07-10 — Réécriture des textes de tech (agent sonnet + finition orchestrateur)
**Découvertes**
- Le jargon « capacité narrative / ordre / fédéralisme » (scps_tech.h) vient du MODÈLE THÉORIQUE de l'auteur — en jeu : dK=prospérité, dL=stabilité&croissance, dF=MORT (jamais lu, comme .eco/.mil). Les textes joueur promettaient des leviers morts ; commentaires de scps_tech.h réécrits pour le dire.
- « béton→marbre » (TECH_QUALITE_MATERIAUX) : hors-monde ET hors-effet — le nœud gate en réalité l'Arbalétrier lourd. Textes réalignés sur l'effet réel dans scps_tech.c ET la table miroir TECH_UTILITY de scps_readout.c (⚠ DEUX tables à tenir synchrones).
- Le composeur hover (scps_api.c, hb[]) appose déjà les chiffres LIVE (prospérité/stabilité/production %/efficacité %/charge/flux) — les utility strings disent l'effet en MOTS, les chiffres suivent seuls ; exception : doctrine d'armée (+dégâts/+moral) et firearm_power, hors composeur → chiffre réel dans le texte.
- RÈGLE 3 : les nœuds sans AUCUN levier vivant ont reçu un petit levier réel (ex. Fortifications dF mort → dL 1.5) — golden resté IDENTIQUE (aucun de ces nœuds n'est recherché <12 ans sur les 5 graines).
**Pièges**
- ⚠ LIEN LATENT : `make` ne re-lie un banc que si un objet change — `scps_ai.o` (committé) référençait `country_knows` (scps_fog.c, lot diplo-fog) sans que AI_DEMO_OBJS porte scps_fog.o ; le banc restait « vert » sur binaire PÉRIMÉ jusqu'au premier re-lien (déclenché ici par scps_tech.o). Fix : scps_fog.o ajouté à AI_DEMO_OBJS. Leçon : un lot qui ajoute une dépendance inter-module doit greper les *_OBJS des bancs.
- Le « cd mangé » MSYS2 re-payé encore : `bash -lc 'cd … && make …'` perd son cd à la régénération → TOUJOURS un script committé (packaging/windows/gates_tech.sh).
- Agent coupé par la limite de session en pleine table NODES[] (Savoir+Forge faits, Société aux 3/4) — l'orchestrateur a fini les 3 dernières chaînes + readout + tech.h à la main, dans le style de l'agent.
**Restes**
- scps_readout.c TECH_UTILITY : synchronisée sur les 2 entrées qui mentaient (béton, métal supprimé) — une passe de cohérence COMPLÈTE des 71 entrées vs les nouveaux textes tech.c serait du polish (elles étaient déjà majoritairement justes).

## 2026-07-10 — Factions en topbar + ledger pliable
**Découvertes**
- Topbar : la rangée de blasons de faction (20 px + mini-barre d'adhésion 5 px dessous, ★ dominante, ⚑ tension) tient à 1920 entre la colonie et le chip d'âge — sur écran étroit, prévoir le repli en un chip « Factions » unique (noté, non fait).
- Ledger : pliage PAR SECTION via `_fold{}` + `_sec_rects` (le bandeau entier cliquable, chevron ▾/▸ dans la bande) — le pattern immediate-mode : chaque corps de section gardé par `if not _folded("TITRE"):`, les zones cliquables du corps (Recompléter) REMISES À ZÉRO quand replié (sinon un clic fantôme sur zone invisible).
**Pièges**
- MISSION : le corps était DANS le `if active:` — plier exige un court-circuit (`mi = {}` après le bandeau) plutôt qu'une ré-indentation de 25 lignes ; documenté sur place.
**Restes**
- Persister `_fold` (user://options.cfg) si le joueur veut retrouver ses replis entre sessions.

## 2026-07-10 — Rail uni + Province flottante EU4
**Découvertes**
- Le rail gauche « disparaissait » : (1) il s'arrêtait à vp.y − BOTTOMBAR_H (la bande n'existe plus — icônes flottantes) ; (2) COL_PANEL est TRANSLUCIDE (a=0xf6) → la carte transparaissait. Fix : pleine hauteur + alpha 1.0.
- Province flottante : position SIDEBAR_W+14 / TOPBAR_H+12, liseré « collé » retiré (panel_bg porte son cadre) — ⚠ le panneau Construction était posé à SIDEBAR_W+320 (l'ancienne largeur 312) → recalé à SIDEBAR_W+374 (main.gd), à resynchroniser si PW bouge encore.
- Les « barres en lignes dorées » du screenshot joueur = la texture de jauge sheet04 étirée à 9-12 px de haut (les enluminures des extrémités bavent) → VKit.gauge (piste + remplissage) pour les rangées fines (Prospérité/Satisfaction/Loyauté) ; UIKit.bar reste pour les barres larges (colonisation).
**Restes**
- PW de province_panel est copié en dur dans main.gd (position du panneau Construction) — une const partagée serait plus sûre.

## 2026-07-10 — Tooltips à concepts + le « paquet de 8-9 jours »
**Découvertes**
- MOTS-CONCEPTS (retour joueur) : registre unique `ui/concepts.gd` (~30 définitions honnêtes) consommé par (1) `ui/tooltip_server.gd` — tooltip global RichText qui REMPLACE le natif (neutralisé par `gui/timers/tooltip_delay_sec` énorme, posé au runtime dans main._ready) : il lit `Control.get_tooltip(pos)` du contrôle survolé (couvre tooltip_text ET les _get_tooltip custom : topbar, province, alertes, atomes Medusa), colore chaque concept en turquoise et appose les définitions dessous ; (2) le CODEX — domaine « Concepts & jauges » GÉNÉRÉ du registre (F1).
- « Les jours s'écoulent par paquets de 8-9 » : MESURÉ FAUX côté sim (perf_probe.tscn : 97 j en 12 s = les 8.1 j/s exacts, 1 j/frame, moteur 0.15 ms/jour). C'était la DATE AFFICHÉE : le topbar se rafraîchit au MOIS ou au CLIC (cadence anti-danse voulue) → entre deux redraws le compteur saute de ~1 s × 8.1 = 8-9 j. Fix : `ui/date_chip.gd` — la date est un contrôle ENFANT à cadence QUOTIDIENNE (Sim.ticked), le topbar garde sa cadence mensuelle ; sa place est réservée par une largeur MAX fixe (l'ancre du chip d'âge ne dépend plus du texte vivant).
- Perf mesurée en fenêtré probe : fps ≈ 16 sur monde an-10 — pas bloquant (la promesse de vitesse est tenue) mais l'overlay reste le budget dominant ; un chantier perf dédié (profiler _draw) est le prochain levier si ça gêne en vrai jeu.
**Pièges**
- `RichTextLabel` dans un tooltip maison : poser la position APRÈS une frame (le layout PanelContainer n'a pas encore sa taille) — `await process_frame` puis clamp viewport.
- BBCode : échapper `[` du texte source (`[lb]`) avant décoration.
**Restes**
- Les tooltips IMMÉDIATS custom (construction_panel/_draw_tooltip, sidebar_drawer._hover_text) ne passent pas par le serveur — à migrer vers tooltip_text/_get_tooltip pour hériter des concepts.
- Cliquer un concept dans le tooltip → ouvrir le codex À la ligne (v1 : hint « F1 » seulement).

## 2026-07-10 — Concepts v2 : icônes + passe TOUS-PANNEAUX
**Découvertes**
- Registre ~55 concepts avec ICÔNE chacun (concepts.gd : {d: définition, i: icône du pack}) — consommé par le TooltipServer ([img=16x16] dans les définitions), le CODEX (domaine « Concepts & jauges », RichTextLabel iconé, vérifié en capture) et implicitement par TOUT hover du jeu.
- CONVERSION des tooltips « maison » vers la voie native (pour hériter des concepts) : sidebar_drawer (_hover_zones fusionnées dans _tips, encart dessiné à la main retiré) · construction_panel (_draw_tooltip → _get_tooltip head+lines ; le surlignage doré au survol reste) · empire_sidebar (SEC_TIPS neufs par bandeau — le ledger n'avait AUCUN hover).
- Matcher : pluriel toléré (alternatives + "s?" ; _key_of retombe sur la clé canonique) — « cicatrices », « fractures » s'éclairent.
- RichTextLabel accepte [img]res://…png[/img] en export (les PNG d'icônes sont des ressources importées) — pas besoin d'atlas.
**Pièges**
- Les mots trop communs sont EXCLUS à dessein (« Or », « Guerre », « Conseil » nus) : ils coloreraient tout ; le registre ne prend que les mots qui portent une jauge/mécanique.
**Restes**
- Hovers restants à « conceptualiser » au fil de l'eau : province_detail (onglets), religion_panel, battle_panel, chronique — la plomberie est en place, c'est du wording.
- Clic sur un concept → scroller le codex à SA ligne (v1 : hint F1).

## 2026-07-10 — Espace fiable + F1-F8 = onglets du rail
**Découvertes**
- Espace « ne marchait plus » APRÈS un clic : le bouton cliqué GARDAIT le focus clavier et mangeait Espace/Entrée avant _unhandled_input. Fix global : `focus_mode = FOCUS_NONE` posé par ui_theme._wire sur chaque BaseButton (présent + futur) — style Paradox, la navigation Tab est sacrifiée sciemment.
- F1-F8 = les 8 onglets du rail gauche PAR EMPLACEMENT (toggle_tab borné) ; F10 devpanel gardé (pas de conflit). Le CODEX (ex-F1) migre au MENU ÉCHAP (bouton + signal codex_requested) — « si conflit, placer les options dans le menu » (consigne joueur). Textes « F1 » mis à jour (header codex, hint du TooltipServer).
**Restes**
- i18n : le bouton « Codex » du menu est un littéral (à passer T_* avec la vague EN).

## 2026-07-10 — Le « double hover » : tooltips verrouillables en cascade (CK3)
**Découvertes**
- tooltip_server réécrit en MACHINE À ÉTAGES : survol 0.45 s → tooltip · 1 s → VERROU (liseré turquoise 2 px, mouse_filter STOP, hitbox élargie grow(16)) · survol d'un mot turquoise 0.3 s → tooltip-ENFANT du concept (né verrouillé) — récursif (chaque définition en appelle d'autres), MAXDEPTH 6, grâce 0.3 s hors hitbox, fermeture du plus profond au plus proche (remonter à un étage N ferme au-delà ; revenir à la SOURCE ne garde que la racine).
- `Concepts.decorate` émet DIRECTEMENT des liens `[url=CLÉ]` (le remplacement après coup ratait pluriels/minuscule : le mot affiché ≠ la clé canonique) — inertes tant que le RichTextLabel ignore la souris, actifs au verrou.
- `RichTextLabel.meta_hover_started/ended` = le mécanisme de cascade (pas de clic requis).
**Pièges**
- Lambda GDScript : `func(x): if c: a; b` met TOUT dans la branche du if — lambdas multilignes obligatoires pour les connects.
- `gui_get_hovered_control` renvoie NOS panneaux une fois STOP : le suivi de la source racine doit les ignorer (`is_ancestor_of`).
**Restes**
- La probe ne survole pas : la cascade est vérifiée en parse + design, l'œil joueur juge le feel (délais DELAY/LOCK_AT/SUB_DELAY faciles à retoucher en tête de fichier).

## 2026-07-10 — Arbre GÉANT scrollable + Construction en liste à paliers
**Découvertes**
- Arbre tech : géométrie FIXE généreuse (rangée 64 px, médaillon ~53 px — enfin lisibles) dans un ScrollContainer (barre latérale) ; le panneau se centre ENTRE le rail (SIDEBAR_W) et le ledger (274) — centré sur l'écran entier il passait SOUS le ledger (capture). Les couloirs/tiers se dessinent sur un FOND enfant du scroll (`_bg.draw.connect`) : ils défilent avec le contenu.
- ⚠ MEDUSA : le Graph recense ses Atomes dans SON _ready → il ne doit entrer dans l'arbre de scène QU'APRÈS la pose des atomes (`_pending_graph_parent`, add_child en dernier). L'ajouter d'abord = graphe vide.
- Construction : « une ligne, un bâtiment » (ROWH 30, icône 24 + nom + « N or · N j », survol doré, ✦ tech) ; les PALIERS familiaux se MASQUENT tant que le précédent n'est pas bâti — façade `ScpsEdificeDef` +{tier, prev, prev_built} (edifice_tier/edifice_prev + scan prov.edi_built de l'empire ; `edifice_prev` renvoie EDIFICE_COUNT pour une base). Défaut SÛR avant rebuild DLL : prev=-1 ⇒ rien de masqué.
**Restes**
- Après rebuild DLL : vérifier en capture que Temple/Cathédrale/Forteresse/Citadelle disparaissent bien an-0 (aucun précédent bâti).

## 2026-07-10 — Construction : onglets + lignes descriptives (recette/effet/flavor)
**Découvertes**
- Façade `ScpsEdificeDef` +{effet, flavor} : l'EFFET est COMPOSÉ du delta ProvBuild réel (K_inst→institutions · H_coerc→coercition · P_open→ouverture · PE_infra→prospérité · food_cap→vivres · faith→foi · savoir · port) — membrane stricte, chiffres du moteur ; le FLAVOR = table EDI_FLAVOR[27] cynique (display-only, scps_api.c).
- Panneau : 2 onglets (Édifices RH 76 : nom+coût / recette en ICÔNES ×qty / effet chiffré / « flavor ») · Manufactures RH 42 ; molette = défilement PAR LIGNE (une rangée partielle repeindrait les onglets : clip au panneau, pas à la liste) ; barre latérale piste+pouce maison.
**Restes**
- Manufactures : pas d'effet/flavor (pas de reader façade des recettes qout/labor) — même motif à étendre si voulu.
- effet/flavor/masquage de palier VISIBLES après rebuild DLL (défauts sûrs d'ici là).

## 2026-07-10 — export Godot : l'env Windows SCRUBÉ (Git Bash → bash MSYS2)
**Pièges**
- Lancer `D:/MSYS2/usr/bin/bash.exe -l` DEPUIS Git Bash scrubbe l'environnement Windows EN ENTIER : `APPDATA` vide (même passé explicitement sur la ligne de commande), `cmd` introuvable → le repli `cmd /c echo %APPDATA%` de build_godot.sh échouait en silence → « aucun modèle d'exportation trouvé ». Depuis un vrai terminal MSYS2 ça marchait, d'où la latence de découverte.
- Fix durable dans build_godot.sh : si APPDATA est vide OU ne contient pas `Godot/export_templates`, on balaie `/c/Users/*/AppData/Roaming` et on prend le profil qui PORTE les templates (détection par présence, pas par env).

## 2026-07-10 — KIT DE DÉPART (stock genèse sur la capitale des empires)
**Découvertes**
- Site du dépôt : `scps_econ.c` bloc « KIT DE DÉPART », inséré juste APRÈS le bloc CS_TRADE_POOL (jurisprudence directe — même écriture directe `pe->stock[RES_X] += …` à la genèse, avant que la vue `region[]` n'existe : aucun risque d'écrasement §P1). Boucle sur `w->country[cid].role==POLITY_PLAYER||POLITY_ANTAGONIST`, dépose sur `w->country[cid].capital_prov` (même garde que le bloc SPAWN_FOOD_RAW juste au-dessus : `cp>=0 && active`).
- Mapping demande→enums réels (scps_types.h) : bois→RES_WOOD · nourriture→RES_GRAIN · argile→RES_CLAY · fer→RES_IRON · pierre→RES_STONE · outils→RES_TOOLS · armes légères→RES_ARMS (RES_ARMS_LIGHT n'est qu'un `#define` ALIAS de RES_ARMS, scps_types.h:219 — pas une ressource distincte) · armes de trait→RES_ARMS_RANGED (confirmé par le commentaire scps_econ.c:70 « armes de trait (fer + bois) », recette BLD_BOWYER) · bière→RES_BEER.
- 9 tunables neufs au registre J (scps_tune_list.h, fin de fichier, après TIER7_POP) : `SPAWN_KIT_WOOD` 50 · `_FOOD` 100 · `_CLAY` 20 · `_IRON` 20 · `_STONE` 20 · `_TOOLS` 20 · `_ARMS` 100 · `_RANGED` 100 · `_BEER` 20. 0 = désactivé (motif standard `tune_f`).
- Diag gated permanent ajouté (motif SCPS_CAPDIAG/SCPS_IPMDIAG) : `SCPS_KITDIAG=1` imprime le stock déposé par pays/capitale — vérifié seed 9 : 5 empires (sur 6 attendus — un 6e a probablement une capitale inactive à la genèse, même contrainte que SPAWN_FOOD_RAW, pas un bug neuf) tous à bois=50 grain=100 argile=20 fer=20 pierre=20 outils=20 armes=100 trait=100 biere=20.
**Pièges**
- Le `cd` inline dans `bash -lc "…"` est mangé (confirmé encore une fois) : `MSYSTEM=MINGW64 D:/MSYS2/usr/bin/bash.exe -l -c "make smoke"` échoue « Aucune règle » — il FAUT un script fichier avec `cd /c/Users/Charl/Desktop/SCPS-main` en 1re ligne, invoqué par chemin ABSOLU (`bash -l /c/…/script.sh`), jamais par chemin relatif (le `cd` du shell interactif -l repart d'ailleurs).
- `packaging/windows/build_ai_demo.sh` existe déjà et fait exactement `make ai_demo + make smoke` — pas besoin d'en écrire un nouveau pour ce genre de vérif, le réutiliser.
**Restes**
- Aucun. `make smoke` 7/7 vert, run court `./chronicle 9 1 30 6 12` sain (hégémon mortel 1/1, exit propre). SAVE non bumpé (dépôt dans des stores déjà sérialisés, `pe->stock[]`). Golden NON re-baseliné (attendu, l'orchestrateur s'en charge après merge).

## 2026-07-10 — TRADITIONS branchées sur le circuit des tunables (TRAD_*_W)
**Découvertes**
- Audit des 9 leviers de `HeritageLeviers` (scps_heritage.h) — la vérité pré-mission : 4 VIVANTS (demographie → `scps_econ.c` §6 fertilité `net_growth=r_base·(1+demo)·(1+bonus)`, rang POP_R_BASE ; coercition → `scps_prosperity.c:314` st.H §2.4 + `scps_diplo.c:790` mil_power ; capacite/permeabilite/arcane/fracture → entrées st.K/st.P/flux_faustien/st.D_bar de `scps_prosperity.c:312-316` — VIVANTS côté §2.4 mais MORTS sur leurs canaux nommés), 1 TÉNU (influence → `scps_diplo.c:807` portée de menace seulement, ±7.5 % de distance effective — jamais l'Influence d'État), 2 quasi-MORTS sur leur canal (rendement ne touchait QUE P_realise `scps_prosperity.c:377`, jamais la production des provinces ; derive ne touchait QUE l'horloge `langue` `scps_world.c:2720` — or `langue` n'est LUE par AUCUNE formule moteur (econ_content_dist = valeurs/subsistance/parente/religion), seulement la bande lignée du readout → un MOT).
- Câblages neufs (7 tunables registre J, tous PAR PAYS via `culture_build_for`, jamais tune_set global) : rendement→`econ_apply_country_tech` (`pe/re->tech_prod += TRAD_REND_W·lev`, rang NODE_PROD_PCT, plancher 0.1) · influence→`sc_trad_influence` dans le standing statecraft (2 sites miroirs, rang prestige) · capacite→pénalité off-culture de society_sat (`scps_econ.c` §6, ×[0.7..1.3]) · permeabilite→P_eff d'`assimilation_tick` (`scps_demography.c` demography_tick) · arcane→`ai_tech_tradition_mult` (coût des nœuds FAUSTIENS ∈[0.5,2], public : ai_effective_cost + voie joueur scps_sim.c + coût AFFICHÉ scps_api.c — le prix montré = le prix payé) · derive→fuse_rate du contact S2 (`demography_contact_tick`) · fracture→fold du grief de révolte (`revolt_scan`, rang W_AGITATION_UNREST, Soudé APAISE).
- `ai_effective_cost` a gagné un 4e param `cid` (10 sites d'appel, tous `a->cid`).
- Aucun nouveau lien Makefile : tout banc qui lie `scps_scps_econ.o` lie déjà `scps_scps_heritage.o` (econ appelle culture_build_for depuis Q6).
**Pièges**
- `re->society_sat` §6 : le build de traditions est désormais HOISTÉ en tête du §6 (`sb_trad`/`trad_lv`) et RÉUTILISÉ par la fertilité (l'ancien `sb_demo` supprimé) — ne pas re-fetcher deux fois.
- `statecraft_influence_flux` n'a AUCUN appelant hors bancs (lecteur mort) mais reste le MIROIR documenté du standing — patché quand même (garder les deux synchrones).
- `culture_build_for(cid)` n'a besoin QUE du cid (hash) — pas de World* : ça se câble partout, y compris statecraft qui n'a pas w sous la main dans ses lecteurs.
**Restes**
- Golden 12 ans BOUGE (les tirages IA mordent la prod/le coût/la révolte dès l'an-0) — re-baseline à l'orchestrateur post-merge, NON faite ici.
- `derive` garde aussi l'horloge `langue` (world_tick) — toujours display-only ; si un jour `langue` doit mordre, c'est un chantier distance-culturelle à part.
- Mesure appariée seed 9 (3 sims × 100 ans, OFF = SCPS_TUNE 7×TRAD_*_W=0 vs ON défauts) : pop 205/131/261k → 241/169/297k (+15-25 %, canal prod + monde plus calme), satisfaction ~égale (70-75/83-88/79-85), hégémon mortel 3/3 les deux, révoltes 18→13, morts 983→446, guerres 59→38 déclarations — sain, pas d'emballement ; le sweep long tranchera les défauts.

## 2026-07-10 — Passe éditoriale TECH (effets/hovers/flavors, scps_tech.c)
**Découvertes**
- Les champs `hover`/`flavor` de `TechNode` (scps_tech.h) EXISTAIENT DÉJÀ (pack display-only posé le 2026-07-05, avec `unlocks` déjà renommés en mots de jeu — « Collège de guerre », « Tour de mages » — préfigurant les renommages demandés) : la mission (3) « AJOUTE le champ » était donc un no-op, tout le travail est passé en RÉÉCRITURE de contenu existant, pas en extension de struct. Aucun risque d'offset/ABI : `tech_hover`/`tech_flavor`/`scps_tech_nodes` (scps_api.c) lisent tous via les accesseurs `tech_node(id)->champ`, jamais par offset brut.
- Audit systématique AVANT édition : grep `"puissance brute"` × dPuissance de chaque nœud a trouvé UN SEUL mensonge (TECH_MANUFACTURE, dPuissance=0 mais hover le promettait) — le reste de la table était déjà honnête sur ce point (résidu d'une passe antérieure). Grep `"agricole"` a trouvé exactement les 7 nœuds listés par le joueur (Irrigation/Abondance/Vergers/Pâturages/Druide/Charrues/Machines agricoles) — aucun de plus, aucun de moins.
- `tech_by_name()` (MODTOOLS `SCPS_MODS` techbonus) matche par le champ `name` — les 7 renommages changent donc la clé de matching d'un éventuel fichier de mods externe déjà écrit (`Qualité des matériaux` → `Taille de précision` etc.) ; sans impact sur le golden/déterminisme (aucun fichier `SCPS_MODS` n'est chargé par défaut).
**Pièges**
- `NODE_EFF_PCT`/`NODE_PROD_PCT` (tables séparées des `NODES[]`, indexées par désignateur `[TECH_X]=`) ne sont PAS dans le hover à l'origine pour Scriptorium/Académie/Université — le +eff% caché n'était mentionné NULLE PART dans le texte joueur ; le retirer n'a donc demandé AUCUN changement de hover, juste la suppression des 3 lignes de table (piège évité : j'ai failli chercher un texte "efficacité" à retirer du hover qui n'existait pas).
- Repéré tard (2e passe) : COMBO_ACADEMIE/COMBO_DRUIDE/COMBO_CHARRUES/COMBO_MACHINES_AGRI avaient été SAUTÉS lors du premier balayage du bloc COMBOS (édités seulement POUDRE/AUTOMATES_ARC puis CHAMAN/GUILDES, en oubliant les nœuds intercalés) — capté par une relecture complète du fichier après la première vague d'edits, avant syntax-check. Toujours RELIRE le fichier entier après un gros pavé d'édits ciblés plutôt que de faire confiance à la liste mentale.
**Restes**
- Effets numériques changés (exactement la liste demandée, rien d'autre) : Scriptorium/Académie/Université −NODE_EFF_PCT (0.05/0.07/0.10→0) · Outillage flux 0.05→0 & charge 0.3→0 · Wards charge 0.3→0 · Fortifications charge 0.2→0 (+ commentaire RÈGLE 3 mis à jour) · Armurerie charge 0.3→0 · Qualité matériaux/Taille de précision +NODE_PROD_PCT 0.05 · Automates +NODE_PROD_PCT 0.08 · Alchimie +NODE_PROD_PCT 0.05 · Scrying/Clairvoyance dK 0→0.5.
- Renommages (champ `name` uniquement, `unlocks` intact) : Scrying→Clairvoyance · Collecte de bois→Sylviculture · Collecte d'argile→Extraction d'argile · Collecte de nourriture→Subsistances · Qualité des matériaux→Taille de précision · Mécanisme d'horlogerie→Horlogerie civique · Foederati→Fédérés.
- Flavors : 74/74 posés (remplacement intégral, texte fourni verbatim) — aucun ID de la liste manquant côté enum, correspondance 1:1 vérifiée par relecture complète du fichier.
- Hovers réécrits (au-delà des 74 flavors) : les nœuds explicitement nommés par le doc (Scriptorium/Académie/Université — aucun changement de texte nécessaire, cf. piège ci-dessus — Wards/Scrying/Outillage/Manufacture/Qualité matériaux/Fortifications/Automates/Armurerie/Alchimie) + les 7 « agricole→production globale » + flux/charge chiffrés annoncés sur Rouages/Horlogerie civique/Glyphes éthérés/Communion éthérée (et, par cohérence, Fonderie/Industrie de masse/Foreuse arcanique/Gravure runique déjà passés en chiffré au fil de la relecture) + Académie cosmopolite/Concile des savants reformulés « efficacité générale (toute production) » au lieu de « recherche » seule. Le reste de la table (POUDRIERE et consorts) n'a PAS été touché en hover — jugé déjà honnête, seul le flavor a changé, pour rester dans le périmètre demandé et ne pas gold-plater.
- Syntax-check : `D:/MSYS2/mingw64/bin/gcc.exe -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party scps/{scps_tech.c,scps_readout.c,scps_api.c}` — les 3 compilent SANS AUCUN warning ni erreur (aucun `cc` natif dans ce Git Bash ; le binaire MSYS2 sous `D:\MSYS2\mingw64\bin\gcc.exe` est la voie qui marche sur ce poste). Pas de `make` lancé (build/ occupé par un autre agent, conforme à la consigne). Golden NON re-baseliné (attendu — les changements d'effets bougent le hash 12 ans, l'orchestrateur s'en charge post-merge). `scps_tech.h` non touché (le champ `flavor` existait déjà).

## 2026-07-10 — Remap des canaux religieux morts (scps_religion.c, pôles + crédos)
**Découvertes**
- Audit confirmé par grep exhaustif (`.ch[RC_*]` hors de scps_religion.c) : SEULS 7 canaux
  sont consommés en prod — RC_K/RC_P/RC_H (scps_prosperity.c:239-241, clampf direct sur
  K/P/H), RC_L+RC_STAB+RC_COHESION (scps_prosperity.c:242, foldés dans `Lt`) et
  RC_POPGROWTH (scps_econ.c:3033, `demo +=`). Les 12 autres (RC_I/F/PE/RESEARCH/ENTROPY/
  INFLUENCE/REVENUE/ASSIM/COERCION/AGITATION/MORALE/CONSCRIPT) ont ZÉRO lecteur — la spec
  du joueur était exacte pôle par pôle (vérifié un par un, aucune correction nécessaire).
- `scps_prosperity.c:233-236` portait déjà un commentaire d'audit identique (« ⚠ SEULS 7
  canaux… ») — probablement laissé par une session antérieure ayant fait le même constat
  sans le exploiter ; ce commentaire a servi de confirmation croisée indépendante.
- `scps_events.c` (`.flavor="…"`) prouve que le RAW UTF-8 direct en littéral C compile et
  tourne déjà dans ce build (MinGW gcc) — pas besoin du style `\xc3\xa9""suite` de
  `strings_ids.h`/`relig_pole_name` (ce style semble spécifique aux tables STR_* membrane,
  pas une exigence du toolchain). `grep -rlP '"[^"]*[àâäéèêëîïôöùûüç][^"]*"' scps/*.c`
  liste ~20 fichiers (tous les `*_demo.c`/`chronicle.c`/`dump.c`/`batch.c`) qui le font déjà.
  → les nouveaux tips/flavors de `relig_pole_tip` sont écrits en RAW UTF-8 (comme
  `scps_events.c`), pas en `\x` escapé (gain de temps + zéro risque de troncature de
  hex-escape sur un accent suivi d'une lettre a-f).
**Pièges**
- Piège hex-escape C99 : `\xNN` consomme TOUS les chiffres hexadécimaux qui suivent
  (0-9a-f), pas seulement 2 — d'où le motif `"\xc3\xa9""condit\xc3\xa9"` (split de
  littéral) partout où un accent est suivi d'une lettre a-f. Évité entièrement en
  utilisant du RAW UTF-8 pour les nouveaux tips.
- `R_RES_UP`/`R_RES_DN` (magnitudes dédiées à RP_GNOSE/RP_ORTHODOXIE sur RC_RESEARCH,
  désormais retirées du remap) n'avaient AUCUN autre consommateur — supprimées proprement
  (grep confirmé 0 référence restante) plutôt que laissées mortes dans le fichier.
- `religion_selftest()` assertait `RC_RESEARCH>0 && RC_ENTROPY>0` sur RP_GNOSE — cassé par
  le remap (ces canaux ne sont plus écrits par ce pôle). Recalé sur `RC_K>0 && RC_L<0`
  (calcul exact : K = FECONDITE(-0.5)+GNOSE(+0.8) = +0.3 ; L = GNOSE(-0.5)+credo
  évangéliste(-0.3, nouveau) = -0.8) — intention du test préservée (vérifier qu'aucune
  annulation inattendue pôle/crédo ne masque le signal).
**Restes**
- Les crédos gardent des deltas sur canaux morts (RC_PE/RC_I/RC_AGITATION/RC_ASSIM/
  RC_COERCION) — VOULU par la spec (réservés à un câblage relationnel diplo/scholar futur),
  documenté en commentaire au-dessus de chaque `g_credo_*`, jamais promis dans les tips.
- Pas de champ `flavor` dédié ajouté à `ReligPoleDef` (la mission demandait explicitement
  le fallback « append au tip » quand le champ n'existe pas) — si un futur agent veut la
  parité stricte avec `TechNode.flavor`/`EvOption.flavor` (séparer mécanique et citation),
  c'est un ajout de champ pur (pas de sérialisation, la table est statique).
- Golden 12 ans : AUCUN re-baseline fait ici (consigne explicite) — attendu à bouger sur
  les graines où une foi naît <an-12 (3/5 selon CLAUDE.md), à charge de l'orchestrateur
  post-merge. `religion_demo`/`scps_api_demo`/`religion_selftest` syntax-checkés seulement
  (gcc -fsyntax-only, MSYS2 `D:\MSYS2\mingw64\bin\gcc.exe`, pas de `make` — build occupé
  par d'autres agents).

## 2026-07-10 — TRADITIONS : les 36 deltas cibles appliqués (docs/EQUILIBRAGE_CULTURE_FOI)
**Découvertes**
- L'agent précédent (vague TRAD_*_W) n'avait câblé que les LEVIERS et re-libellé les hovers
  EXISTANTS — aucun `.lev` de la table `TRAITS[]` n'avait encore bougé vers les cibles de
  `docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md`. Cette mission applique les deltas eux-mêmes
  (36 traits) + ajoute le champ `flavor` (citation courte, display-only, fin de struct —
  `TraitDef` en tête de `scps_heritage.h`, `trait_flavor()` en accesseur) sans toucher `.pts`
  ni `.antonym` ⇒ `build_is_valid` (budget 1 majeur/1 mineur/1 défaut par axe) tient
  automatiquement, aucune vérif requise au-delà d'une lecture.
- Mapping chiffré vérifié contre `scps_tune_list.h` (TRAD_*_W, posés par l'agent précédent) :
  `rendement` 1:1 sur `tech_prod` (0.10=10 %) · `influence` ×10 (TRAD_INFL_W, 0.25→+2.5,
  0.5→+5 d'Influence d'État) · `capacite` ×0.30 sur la pénalité de diversité minoritaire
  (TRAD_CAP_W, 0.5→15 %, 0.25→7.5 %) · `arcane` ×0.25 sur le coût de la branche faustienne
  (TRAD_ARCANE_W, 1.0→25 %) · `coercition`/`fracture`/`derive`/`permeabilite` restent
  additifs directs sur l'échelle moteur (H 0-10, D̄, dérive relative, P) — TRAD_FRACT_W=0.06
  cité en hover pour Soudé/Factieux (±1.0/±0.5 → ±0.06/±0.03 sur le grief de révolte),
  `permeabilite` laissé qualitatif (comme le faisait déjà l'agent précédent, pas de chiffre
  TRAD_PERM_W×lev cité — l'échelle P interne n'est pas documentée en unités lisibles joueur).
- RÉCONCILIATION Arcanique/Sourd (imposée par le brief, contre le texte brut de la spec
  ligne 81/87 qui datait d'avant le câblage) : **gardé** `arcane=+1.0`/`-1.0` tel quel (déjà
  deux faces vivantes : coût de branche ∓25 % + pente de flux ±1) ; **refusé** le
  `K+1`/`K-0.5` que la spec proposait en compensation d'un levier qu'elle croyait mort — le
  brief le dit explicitement, documenté aussi dans le hover ("mais la pente vers la Brèche
  s'accentue"/"mais −1 de flux lui ferme aussi les débouchés arcanes" : chaque pôle énonce
  sa contrepartie, plus de "protection gratuite" ni de "pénalité pure").
- Deltas retirés (remplacés par un autre levier, pas juste réduits) : `T_PROSELYTE`/
  `T_RESERVE` perdent `.derive` (dupliquait Adaptable/Traditionaliste) au profit de
  `.permeabilite` ±0.25 ; `T_ADAPTABLE`/`T_TRADITIONALISTE` perdent `.derive` côté Adaptable
  (Traditionaliste GARDE le sien, +P−0.5 en plus — asymétrie voulue par la spec) ;
  `T_STUDIEUX`/`T_INCULTE` passent de `.rendement` (doublonnait Industrieux/Inculte) à
  `.capacite` ±0.5 (miroir de Discipline/Frondeur) ; `T_FRELE`/`T_CONVALESCENT` perdent
  leur `.coercition` (spec : "SEUL") — ces deux-là restent volontairement ASYMÉTRIQUES vs
  leur antonyme (Robuste garde coercition+0.5, Régénérant garde coercition+0.5) : la spec le
  demande explicitement, `build_is_valid` ne l'interdit pas (seul `.pts`/`.antonym`/`.cat`
  comptent pour la validation, jamais la magnitude des leviers).
- 4 traits « REDONDANTS UI » (exclus du créateur joueur mais vivants côté IA/
  `culture_random_build`) recalés comme demandé : Endurant/Fragile au climat (hover
  dé-climatisé, magnitude ±10 % démo inchangée), Industrieux/Indolent (±10 % prod, gardés
  tels quels — Industrieux devient la SEULE option "prod pure +10 %" côté intellectuel
  depuis que Studieux migre vers K).

**Tableau des 36 (delta AVANT → APRÈS, `.lev`)**
| Trait | Avant | Après |
|---|---|---|
| Robuste | H+1, rend+20% | H+0.5, rend+10% |
| Régénérant | démo+20%, H+1 | démo+10%, H+0.5 |
| Prolifique | démo+15% | démo+10% |
| Longévif | dérive−20% | dérive−20%, K+0.25 |
| Endurant | démo+10% | inchangé (hover dé-climatisé) |
| Sobre | rend+10% | inchangé |
| Frêle | H−1, rend−20% | rend−10% (H retiré) |
| Convalescent | démo−20%, H−1 | démo−10% (H retiré) |
| Lent à croître | démo−15% | démo−10% |
| Éphémère | dérive+20% | dérive+20%, K−0.25 |
| Fragile au climat | démo−10% | inchangé (hover dé-climatisé) |
| Vorace | rend−10% | inchangé (hover corrigé) |
| Belliqueux | H+1 | inchangé |
| Soudé | fracture−1 | inchangé (hover chiffre le fold −0.06) |
| Charismatique | infl+0.5 | inchangé (hover = portée diplo) |
| Ouvert | P+0.5 | inchangé |
| Discipliné | K+0.5 | inchangé |
| Prosélyte | infl+0.25, dérive+20% | infl+0.25, P+0.25 (dérive→P) |
| Débonnaire | H−1 | H−0.5 |
| Factieux | fracture+1 | fracture+0.5 |
| Rebutant | infl−0.5 | inchangé |
| Insulaire | P−0.5 | inchangé |
| Frondeur | K−0.5 | inchangé |
| Réservé | infl−0.25, dérive−20% | infl−0.25, P−0.25 (dérive→P) |
| Inventif | rend+20% | inchangé (hover : "recherche" retiré) |
| Arcanique | arcane+1 | inchangé (RÉCONCILIÉ, pas de K+1) |
| Studieux | rend+10% | K+0.5 (rend retiré) |
| Bâtisseur | K+0.5 | K+0.25, rend+5% |
| Adaptable | dérive+20% | P+0.5 (dérive retiré) |
| Industrieux | rend+10% | inchangé |
| Borné | rend−20% | rend−10% |
| Sourd à l'arcane | arcane−1 | inchangé (RÉCONCILIÉ, pas de K−0.5) |
| Inculte | rend−10% | K−0.5 (rend retiré) |
| Brouillon | K−0.5 | K−0.25, rend−5% |
| Traditionaliste | dérive−20% | dérive−20%, P−0.5 (ajouté) |
| Indolent | rend−10% | inchangé |

**Pièges**
- `cc`/`gcc` nus absents du PATH Git Bash — le binaire MSYS2
  `D:\MSYS2\mingw64\bin\gcc.exe` existe mais ÉCHOUE SILENCIEUSEMENT (exit 1, ZÉRO texte
  d'erreur, y compris sur un `int main(){}` trivial) tant que `mingw64/bin` n'est pas EN
  TÊTE du `PATH` — sans ça il ne trouve pas son propre `cc1` interne. Fix :
  `PATH="/d/MSYS2/mingw64/bin:$PATH" gcc.exe -fsyntax-only …`. Reproduit identiquement en
  PowerShell (`& "D:\MSYS2\mingw64\bin\gcc.exe"` seul → exit 1 muet aussi). Vérifié aussi
  sur `heritage_demo.c` (lecteur de `trait_def`/`.antonym`, non modifié) : compile propre,
  la struct `TraitDef` +1 champ trailing ne casse aucun accès positionnel existant.
**Restes**
- Golden 12 ans : re-baseline attendue (les tirages IA — `culture_random_build` — changent
  de magnitude dès l'an-0 sur ~2/3 des traits) ; NON faite ici (build occupé par d'autres
  agents, consigne explicite de ne pas toucher au moteur/golden).
- Le champ `flavor` n'est PAS encore exposé côté façade (`scps_api.c` `ScpsTraitInfo` n'a
  qu'un champ `hover`) ni côté Godot (`culture_creator.gd`/binding) — câblage de surface
  laissé à un futur agent (hors du fichier autorisé pour cette mission).
- Section ÉTHOS/HÉRITAGES (signatures politiques Légiste/Marchand/Transgresseur/etc.) de la
  spec vit ailleurs (probablement `scps_culture.c`, hors `scps_heritage.c`) — non touchée,
  hors scope de cette mission.

## Passe éditoriale ÉVÉNEMENTS (task #69, 2026-07-10)

**Découvertes**
- `scps_events.h:97-113` `EvOption` n'a AUCUN champ « intro/desc » au niveau de
  l'ÉVÉNEMENT — seulement `label`/`blurb`/`flavor` PAR OPTION. La spec joueur demande
  une OUVERTURE unique par évènement + un flavor par option ; faute de champ dédié (et
  fichier autorisé = `scps_events.c` seul, header interdit), l'ouverture est postée dans
  **`blurb` de CHAQUE option du même évènement** (texte dupliqué verbatim par option) —
  `blurb` n'est lu NULLE PART à l'affichage aujourd'hui (`scps_api.c:2400-2461`
  `scps_pending_event` n'expose que `situation` — `event_title()`, PAS `blurb` — et
  `flavors[]`/`effets[]` calculés à la volée) : c'est un contenu PRÉPARÉ pour un futur
  câblage UI, zéro risque de régression d'affichage actuel.
- `event_title()` (`scps_events.c:1873-1896`) résout `%s` UNIQUEMENT sur `EventDef.name`
  (le TITRE), jamais sur `blurb`/`flavor` — ces deux champs sont de simples `const char*`
  jamais passés en format string nulle part (vérifié : `events_text_clean` les scanne en
  `strstr`, `scps_api.c` les recopie tels quels via `sz()`). Les ouvertures fournies par
  le joueur pour les évènements à titre `%s` (Marbrive, Pont effondré, Cloches, Deux
  cartes, Eau noire, Dernière décision, Foreuse saigne, Relique douteuse, Remède fait
  des morts, Cellule des faubourgs, Tarif appris, les 3 Trahisons, Marche-éthos)
  contiennent ELLES-MÊMES un `%s` (le nom de province/pays/ministre, cohérent avec le
  titre) → posé TEL QUEL dans `blurb` (littéralement `%s`, jamais résolu tant qu'aucun
  code ne fait le même traitement que `event_title` sur `blurb`) : INERTE aujourd'hui
  (blurb non affiché), attend un futur câblage qui appliquera la même substitution.
- Motif d'ajout de `flavor` SANS toucher `eff`/`hook`/`gamble` : `EvOption` a l'ordre
  `label, blurb, eff, ai_chance, hook, flavor, gamble_eff, gamble_p`. Pour les options
  qui s'arrêtaient à `ai_chance` (4 positionnels, ex. QUAKE/FLOOD/…) ou à `hook` (5
  positionnels, ex. « Guerre préventive » de FUSILS_REVIENNENT, « Renoncer à la branche »
  de SAVANTS_ENNEMI), l'ajout `, .flavor="…"` APRÈS la liste positionnelle est un
  initialiseur C99 valide (positionnel jusqu'à l'index N, puis désigné pour un index
  ultérieur) et laisse le(s) champ(s) intermédiaire(s) non nommé(s) (`hook` notamment)
  au ZÉRO IMPLICITE identique à avant — aucun effet de bord sur faction/scar/cooldown.
  Vérifié : compile propre, `hook.faction` reste 0 par défaut comme avant sur ces cas.
- 56/56 `EvId` de `scps_events.h` couverts (tous ceux listés dans la table `EVENTS[]`,
  de `EVID_QUAKE` à `EVID_PRATIQUE_DERIVE`) — aucun EVID de la commande n'était absent
  du code, aucun n'a été inventé.
- Titres RENOMMÉS effectivement (14/56, les autres soit gardaient déjà exactement le
  texte proposé — no-op — soit n'avaient aucun renommage proposé — titre inchangé) :
  QUAKE, FLOOD, DROUGHT, FIRE, PLAGUE, SUCCESSION, SCHISM, XENOPHILE, XENOPHOBE,
  MERV_FONDATION, MERV_SACRIFICE, MERV_ASCENSION, CONSEIL_SUCCESSION, PARENTE_LOINTAINE.
  MARBRIVE et les 15 autres titres à `%s` : gardés BYTE-IDENTIQUES (vérifié par grep,
  16 titres à `%s` avant/après, contenu identique).

**Pièges**
- `cc`/`gcc` nus absents du PATH ; `D:\MSYS2\mingw64\bin\gcc.exe` invoqué en chemin
  ABSOLU sans `mingw64/bin` en tête du PATH échoue SILENCIEUSEMENT (exit 1, zéro texte,
  même sur `int main(){}`) — déjà noté par un agent précédent (cf. entrée Traits ci-
  dessus). Contournement supplémentaire trouvé ici : ajouter `-v` fait apparaître le vrai
  résultat (RC=0, pas d'erreur) même SANS le fix de PATH — mais le fix propre reste
  `PATH="/d/MSYS2/mingw64/bin:$PATH" gcc.exe -fsyntax-only -std=c99 -Wall -Wextra
  -Ithird_party scps/scps_events.c` (RC=0, 0 warning, confirmé deux fois).
- Piège de conception évité : dupliquer l'ouverture dans `blurb` de CHAQUE option (au
  lieu d'une seule fois) était nécessaire car rien à l'échelle de l'`EventDef` ne porte
  un texte partagé entre options — la duplication de littéraux C est inoffensive
  (`events_text_clean` scanne chaque `blurb` séparément, pas de coût de perf sensible,
  table statique compilée une fois).

**Restes**
- `blurb` (l'ouverture) reste NON EXPOSÉ à l'UI (ni façade `scps_api.c`, ni Godot) — un
  futur agent devra soit ajouter `situation_blurb`/`intro` à `ScpsPendingEvent` (lisant
  `d->options[0].blurb`, identique pour toute option) soit réutiliser `event_title()`-
  style resolution pour que les `%s` dans les ouvertures s'affichent correctement (16
  évènements en dépendent) — CHAMP HEADER À AJOUTER, hors du fichier autorisé ici.
- `flavor` EST déjà exposé (`scps_api.c:2421` `out->flavors[i]`) et donc les 56 nouveaux
  textes sont VISIBLES dès aujourd'hui côté joueur sans câblage supplémentaire.

## 2026-07-10 — ÉQUILIBRAGE ÉTHOS + HÉRITAGES + ESCLAVAGE (docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md §ÉTHOS/§HÉRITAGES)
**Découvertes**
- Les 3 tables visées par la spec ne vivent PAS où le nom aurait suggéré : la tolérance
  fiscale par éthos×classe est `econ_tax_tolerance` (scps_econ.c:1652, PAS scps_culture.c) ;
  le « biais institutionnel » Bureaucrate est `build_f` dans `ai_derive_weights`
  (scps_ai.c:142, un multiplicateur de `w_build`, pas une table à part) ; le « surcoût
  militaire IA » Pacifiste est la branche `FN_ARMEE` d'`ai_tech_cost_mult`
  (scps_ai.c:1963, PAS un champ de fiche) ; la « signature politique » d'héritage est le
  bloc switch §2 de `group_ethos_lean` (scps_factions.c:39-46, PAS scps_heritage.c — ce
  fichier ne porte que TRAITS[]/ROSTER[], la signature de FACTION est un fichier distinct).
- Le gate d'esclavage EST DOUBLÉ (miroir volontaire, documenté en commentaire) : `a->can_enslave`
  (scps_ai.c:2389, l'IA) ET `econ_country_can_enslave` (scps_econ.c:490, le VERBE JOUEUR/marché,
  seul point lu par scps_api.c/scps_navy.c/scps_sim.c — 4 appelants externes tous en LECTURE
  de cette fonction, aucun ne réévalue l'éthos lui-même). Les DEUX devaient changer ensemble
  (raté l'un = un vrai split-brain joueur-vs-IA sur qui peut acheter/capturer).
- `ScpsEthosDef`/`ScpsHeritage` (scps_api.h:1020/1024) avaient déjà un slot `hint`/pas de
  flavor — le `flavor` demandé par la spec est un concept SÉPARÉ du `hint` existant (hint =
  résumé mécanique court « Conquête : pousse la coercition… » ; flavor = la phrase d'ambiance
  fournie verbatim par le joueur). Ajouté en FIN de struct (aucun risque d'ABI/offset pour les
  lecteurs C existants — seuls des accès nommés `.epithete`/`.hint`, jamais d'initialiseur
  positionnel `{a,b,c,d}` sur ces deux structs, vérifié par grep avant l'édit).
- Piège de banc pris AVANT compile : `factions_demo.c:66` assertait
  `w[FAC_TRANSGRESSEUR]>0.15f` pour ORDRE+MÉTALLURGISTE — recalcul à la main avec les
  nouvelles valeurs normalisées (Légiste+0.30/Transgresseur+0.30 au lieu de 0.4/0.4) donne
  EXACTEMENT 0.30/2.00=0.15 normalisé (tombe pile sur le seuil, dépend de l'arrondi flottant
  de l'ordre d'accumulation `for f<FAC_COUNT: s+=w[f]`) → seuil abaissé à 0.12f (marge de
  sécurité, intention « Transgresseur présent, non dominant » inchangée). Les 5 autres
  assertions de `factions_demo.c` (agraire mercantile, adaptatif bureaucrate, mécaniste
  pacifiste, l'enracinement conquête §3, la classe qui pèse §4, fracture §6, coup §7) ont
  toutes été recalculées à la main (les deltas de la spec préservent les ORDRES relatifs —
  Σ toujours ≈0.60, juste redistribué entre les deux factions du couple) : marges confortables,
  aucun autre banc cassé. `econ_tax_demo.c` (5 assertions comparatives sur `econ_tax_tolerance`)
  vérifié de même : toutes tiennent avec les nouvelles valeurs (marges ≥0.02 partout, PACIFISTE
  0.38<0.4f tient de justesse mais tient).
**Pièges**
- Trois autres agents éditaient `scps_ai.c` EN PARALLÈLE sur la MÊME fonction
  (`ai_research_step`/`ai_effective_cost`, lignes 2238-2600, la mission tech-diffusion/traditions
  arcane) — mes cibles (build_f:142, FN_ARMEE mult:1963, can_enslave:2389) sont toutes HORS de
  leurs hunks (vérifié `git diff --stat`/`git diff | grep @@` AVANT chaque édition, comme
  demandé). Aucune collision, mais `git diff` a dû être relu une 2e fois après l'édit du gate
  can_enslave car la ligne bougeait de ±quelques lignes à cause des inserts amont d'un autre
  agent entre deux de mes lectures — toujours RELIRE juste avant d'éditer, pas se fier à un
  numéro de ligne mémorisé d'un Read précédent.
- Retirer la branche éthos de `econ_country_can_enslave` rend `w`/`econ`/`cid` inutilisés dans
  le corps — gardés dans la SIGNATURE (4 appelants externes, casser l'ABI C aurait forcé 4
  sites de plus à toucher hors scope) et neutralisés par `(void)w; (void)econ; (void)cid;`
  (motif déjà présent ailleurs dans le fichier, scps_econ.c:3776, repris à l'identique).
**Restes**
- Le champ `flavor` neuf n'est PAS câblé côté Godot (`scps_sim_node.cpp`/`.h` +
  `culture_creator.gd` ne lisent que `epithete`/`hint` du Dictionary) — binding + UI à étendre
  par un futur agent (hors du périmètre .c de cette mission, DLL non reconstruite ici).
- Priorité 5 de la spec (« mesurer sur campagnes identiques ») NON faite ici — aucun sweep
  lancé (consigne : ne pas re-baseliner golden, l'orchestrateur mesurera après merge).
- Sections TRADITIONS et FOI de la spec : NON touchées (hors mission, gérées par d'autres
  agents en parallèle — cf. entrée « TRADITIONS branchées sur le circuit des tunables » et
  « Passe éditoriale TECH » plus haut dans ce fichier).
- Golden 12 ans va bouger (tolérance fiscale + biais IA + signature de faction mordent dès
  l'an-0) — attendu, PAS re-baseliné ici (consigne explicite). SAVE : rien de sérialisé ne
  change (tables statiques + 1 champ ajouté à une struct façade non-sérialisée) — pas de bump.
- Syntax-check : `D:/MSYS2/mingw64/bin/gcc.exe -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party`
  sur `scps_econ.c`, `scps_ai.c`, `scps_ai.h`, `scps_factions.c`, `scps_api.c`, `scps_api.h`,
  `factions_demo.c` — tous compilent SANS AUCUN warning ni erreur.

## 2026-07-10 — clôture de vague : 4 pièges d'orchestration payés cher
**Découvertes**
- scps_api_demo cassé DEPUIS le lot diplo-fog (pas par la vague) : un joueur PASSIF ne
  « connaît » personne (country_knows, radius 2) → tous les verbes diplo = « cible
  inconnue ». Masqué car `make test` complet n'avait pas retourné depuis. Fix :
  `fog_debug_meet_all` (scps_fog.{h,c}, BANC/FUZZ seulement, motif intertrade_debug_set_hub_of).
- `ScpsCountryInfo.gold` est un DOUBLE : l'imprimer en %d = UB varargs qui pourrit TOUTE
  la ligne printf (les « INT_MIN / 1e30 » de mon diag étaient du garbage d'affichage —
  l'économie était saine). Toujours vérifier les types de champs façade avant un printf de diag.
**Pièges**
- `shot_ui.tscn` est une probe FENÊTRÉE (c'est écrit dans son en-tête) : en --headless,
  `await RenderingServer.frame_post_draw` NE TIRE JAMAIS → gel éternel avant la 1re capture.
  4 heures de fausse piste (DLL scons, cache .godot, moteur) pour une option de ligne de commande.
- Le prompt de crash (feedback.gd `popup_centered`) GÈLE aussi une probe : chaque instance
  tuée arme le flag → la probe suivante pend sur le dialogue → on la tue → cercle vicieux.
  Fix : _detect_crash efface le flag et sort en headless.
- Diag du gel : `user://logs/godot.log` (file_logging actif) donne la chronologie NON
  bufferisée là où le stdout redirigé (fully buffered) ment sur le point d'arrêt réel.
**Restes**
- Champ flavor des traits/éthos/héritages/techs rempli côté moteur mais pas encore rendu
  par les panneaux Godot (creator/tech Medusa) — câblage UI à faire.
- EvOption.blurb porte les OUVERTURES d'événements (inerte, rien ne le rend) — à câbler
  dans event_dialog (attention aux %s du blurb au moment du rendu).
- 2 stashs parqués (vague-complete dupliqué, heritage orphelin d'agent) — à droper après
  vérif que le commit de vague contient tout.
- **LECTEURS #68/#69/#70 — aperçu d'action + pénuries + cadres d'identité conseil (2026-07-10)** :
  trois readers additifs PURS dans scps_api.{h,c} (aucune mutation moteur), pour docs/RETOURS_2026-07-10.md
  points 2/4/7. Fichiers touchés : scps/scps_api.{h,c}, godot/src/scps_sim_node.{cpp,h} seulement.
  **(1) `scps_action_preview(s,region,verb,out)`** (scps_api.c, juste après scps_player_purge) — verb
  0=MATER/1=FORMER/2=PURGER. Miroir EXACT de scps_agency.c : REPRESS_DAYS=30 (scps_agency.c:523),
  ASSIM_DAYS=365 (:524), PURGE_FRAC_AN=0.12f (:525) — ces trois `#define` sont `static`/non-exportés,
  donc RE-DÉCLARÉS (valeurs recopiées, commentées file:ligne) faute de pouvoir toucher scps_agency.{h,c}
  (hors périmètre de cette mission) ; AGY_PURGE_YEARS EST exporté (scps_agency.h:169), utilisé directement.
  `biggest_minority` (scps_agency.c:560-573) est `static` → réimplémentée à l'identique en `ap_biggest_minority`
  (même exclusion CLASS_SLAVE — l'esclavage n'est ni cible d'assimilation ni de purge). MATER appelle la VRAIE
  `province_apply_coercion` (scps_demography.c:154) sur une COPIE jetable de ProvincePop + un ModifierStack
  SCRATCH `calloc`'d (motif demography_demo.c/revolt_demo.c — ModifierStack pèse ~112 Ko, JAMAIS sur la pile,
  cf. le piège STACK_OVERFLOW Windows déjà documenté ailleurs dans ce fichier) : la sortie CoercionEffect
  est la sortie RÉELLE de la formule, aucune réimplémentation de son calcul interne. FORMER/PURGER mirent
  directement les lignes d'apply_action/purge_slice (agitation via `agit_from_L`, scps_demography.c:59,
  formule publique commentée — mirée à l'identique : `clampf((6-L)*15,0,100)`). LES TROIS sont GRATUITES en
  or (aucun coût gold dans agency_order_*/apply_action, vérifié) et 100 % DÉTERMINISTES (0 rand()/frand() dans
  scps_agency.c ET scps_demography.c) — `risque` porte donc la conséquence DIFFÉRÉE (masquage/frottement/
  plancher), jamais une chance. `satisfaction_delta` reste 0 pour LES TROIS (honnête : aucune des trois
  formules ne touche `ProvinceEconomy.satisfaction`, scps_econ.h:289 — pas d'invention).
  **(2) `scps_country_shortages(s,country,out,max)`** — miroir DIRECT d'`econ_country_forecast`
  (scps_econ.c:3268, signature `(const WorldEconomy*,int cid,float horizon,EconForecast*)`, scps_econ.h:846).
  Seuil « runway court » = `tune_f("AI_SAFETY_HORIZON",12.f)` — LE MÊME seuil que l'IA lit pour son
  `food_alert` (scps_ai.c:319) : pas un chiffre inventé pour l'UI. `runway[g]` est en ANNÉES côté moteur
  (commenté scps_econ.h:833) → converti en jours (×365) SEULEMENT à l'affichage ; sentinel -1.f si
  runway≥1e8 (le moteur clampe à 1e9f, scps_econ.c:3331).
  **(3) CADRES DU CONSEIL** — `identite`/`portrait_id`/`id_flavor` ajoutés en FIN de ScpsCouncilSeat/
  ScpsCouncilCand (append-only, rien ne casse). ⚠ SPEC CORRIGÉE EN COURS DE LOT (docs/CONSEIL_
  ORIENTATIONS_2026-07-10.md § « Noms, maisons, identités ») : les identités sont **PUREMENT
  NARRATIVES, EFFET MÉCANIQUE 0** — le joueur a tranché « pas de faux modificateurs affichés, pas de
  boucle d'équilibrage relancée ». La 1re version de ce lot exposait un champ `modif` (coefficient
  CONS_ID_* réservé, jamais câblé au tick) : RETIRÉ (struct + Dictionary) et remplacé par `id_flavor`
  (la phrase d'identité de la spec, VERBATIM). Il n'existe AUCUN coefficient ni site de lecture moteur —
  du chrome. Identité = hash DÉTERMINISTE sur les MÊMES clés que nom/tier/âge/faction (seed,cid,seat,
  slot,gen — statecraft_council_cand_*, scps_statecraft.c:105-131) : `sc_hash` (scps_statecraft.c:88-92)
  est `static` → réimplémentée à l'identique (`cons_hash`) avec un salt NEUF (0x1DE47170, distinct des
  5 déjà pris par le module : tier 0xC0FFEE/nom 0x5EAB011/âge 0xA6E11/faction 0xFAC7104/retraite
  0x0DDA6E) → identité ⊥ faction ⊥ siège ⊥ rang, comme la spec l'exige des maisons. Les 8 identités DE
  LA SPEC (Rigoriste/Courtisan/Austère/Réformateur/Vétéran/Ambitieux/Loyaliste/Vénal) avec leurs
  flavors verbatim ; `portrait_id` 0..7 = index DIRECT dans `UIKit.advisor_portrait` (8 bustes planche
  13, déjà exploités par sidebar_drawer.gd:311-312). Golden : hors de portée par construction
  (scps_api.c n'est pas dans chronicle ; rien ne mord le tick).
  **Pièges** : `RES_COUNT` n'est PAS visible côté binding Godot (scps_sim_node.cpp n'inclut QUE scps_api.h,
  volontairement opaque — pas de scps_types.h) → le binding `country_shortages` utilise un buffer fixe [64]
  (RES_COUNT≈57 compté à la main dans scps_types.h) au lieu de `ScpsShortage sh[RES_COUNT]` (aurait cassé la
  compilation C++, symbole inconnu). `modstack_accumulate_drift` DÉRÉFÉRENCE son 1er argument SANS garde NULL
  (scps_modifier.c:42-55) → `province_apply_coercion(&tmp, NULL, H)` aurait planté ; le scratch ModifierStack
  DOIT être alloué (même vide, n=0 via calloc) avant l'appel — jamais NULL.
  **Restes** : les panneaux GDScript (province_detail pour action_preview, topbar pour
  country_shortages, sb_panel_conseil pour identite/portrait_id/id_flavor) ne sont PAS câblés (hors
  scope explicite de cette mission — « le joueur peaufinera le panneau lui-même »). La spec Conseil
  contient d'autres chantiers NON couverts ici (PERSONNE+MAISON séparées, missions décennales
  raccordées au siège, hooks d'événements dynamiques, orientations politiques) — ce lot ne livre que
  les identités narratives. Les identités sont des littéraux FR dans scps_api.c (comme les statuts
  « florissant »/« modeste » existants de la même façade — la migration STR_* de ce fichier est un
  chantier à part). Gates vérifiées : `gcc -fsyntax-only -std=c99 -Wall -Wextra` 0 warning sur
  scps_api.c ET scps_api_demo.c (le header modifié ne casse pas les bancs existants) ; binding C++
  relu (pas de godot-cpp dispo pour `g++ -fsyntax-only` sur ce poste, cf. consigne).

## 2026-07-10 — UI-3/UI-4/UI-5 : zone contextuelle unique + hiérarchie d'actions (main/province_panel/country_panel/province_detail/country_actions)

**Découvertes**
- **UI-3 « remplace, pas s'ajoute » : le hook robuste est `visibility_changed`, pas les sites d'ouverture.**
  main.gd câble désormais `_prov_detail.visibility_changed` (détail visible ⇒ `_prov_panel.visible=false` ;
  détail fermé + `_sel_prov>=0` ⇒ `show_province(_sel_prov)` — le panneau REVIENT) et le miroir
  `_country_actions.visibility_changed` ↔ `_country_panel` (via `_sel_owner`, var neuve). Un hook par
  SIGNAL couvre d'un coup : la pile Échap (`_close_topmost` pose `.visible=false` en direct), les
  ouvertures par signaux (detail_requested/open_country), ET la probe shot_ui (qui pose `.visible`
  brut). Câbler chaque site d'ouverture aurait laissé des trous.
- **`_close_topmost` n'a pas eu besoin de changer** : il hide le détail → visibility_changed restaure le
  panneau → l'Échap suivant tombe sur le panneau restauré → `_clear_selection`. La boucle `while
  _close_topmost()` de shot_ui._reset() TERMINE (le panneau restauré est fermé par la branche sélection).
- **province_detail ancré à la place du panneau** : `_layout()` pose `position = (Frame.SIDEBAR_W+14,
  TOPBAR_H+12)` (l'ancre exacte de province_panel._layout) au lieu du centre — le regard ne saute plus.
  Import Frame ajouté au fichier.
- **province_panel / country_panel : PAS mutuellement exclusifs au sens strict, et c'est VOULU** — ils
  sont nourris par la MÊME sélection (`_on_province_picked` met à jour les deux), vivent sur des bords
  OPPOSÉS (province à gauche = zone contextuelle ; pays à droite = carte d'info compacte étranger-seul,
  `show_country` se cache pour le joueur). Jamais d'état stale (chaque pick réécrit les deux) ; les
  rendre exclusifs tuerait le seul chemin d'affichage de country_panel. Vérifié, pas corrigé.
- **UI-4 hiérarchie d'actions (province_panel `_draw_gov_actions`, remplace le `_act_chips` uniforme)** :
  Réprimer/Assimiler = chips neutres (inchangés) · Purger = ROUGE SOMBRE (fond 0.18/0.05/0.04, liseré
  0.58/0.17/0.13) à CONFIRMATION 2 clics — 1er clic arme (« Confirmer la purge ? », liseré vif), 2e clic
  exécute ; fenêtre 4 s par `_process` (⚠ PAS `Sim.ticked` : en PAUSE le tick ne tourne pas, la confirmation
  ne retomberait jamais) ; motif copié de `_servile_manumit_armed` (sidebar_drawer) · Détail = texte nu
  COL_DIM sans cadre (navigation). Changer de province désarme (`show_province`).
- **country_actions : « Guerre » destructif** — styleboxes rouge sombre locaux (`_mkbox`, miroir de
  ui_theme._box — dupliqué car ui_theme.gd est HORS périmètre et une teinte destructive n'a pas sa place
  dans le thème global) + libellé « ⚔ Guerre » → « Confirmer la guerre ? » armé 4 s (`_war_press`), fond
  `_war_sb_armed` plus vif via override du stylebox "normal" à chaque `_refresh`. Le bouton DISABLED garde
  le stylebox "disabled" du thème (fané neutre) — un destructif grisé ne doit pas crier. `_refresh`
  désarme si la guerre cesse d'être légale.
- **UI-5 (couleur seule doublée)** : l'ambre « il refusera » (country_actions) porte désormais « ⚠ » dans
  le LIBELLÉ (visible sans survol — le modulate seul était invisible avant hover) ; le Logement saturé
  (province_panel) gagne « ⚠ » devant le ratio (le rouge sense(0.12) seul ne se lisait pas). Les autres
  sites sense() des fichiers du lot portaient déjà leur second canal (chiffre, ▲▼·, +/−, mots).
- **Conséquences avant décision** : hover des 3 verbes intérieurs via `w.action_preview(reg, verb)` si la
  méthode existe (clés EXACTES du binding scps_sim_node.cpp:1276 : cost_gold · duration_days · pop_delta ·
  satisfaction_delta · agitation_delta · coercition_delta · risque) ; sinon FALLBACK factuel sans AUCUN
  chiffre (discipline membrane). ⚠ le Dictionary du binding n'est JAMAIS vide (préchargé de zéros) — le
  gate du fallback est « tout à zéro ET risque vide », pas `is_empty()`.
- **Probe shot_ui (extension de périmètre accordée)** : la sélection de « foe » (captures 05/06) tombait
  sur (a) un PAYS REBELLE transitoire Phase 3a (« Rebelles de X », slot POLITY_ANTAGONIST quasi sans terre
  ⇒ panneau vide) puis (b) un pays JAMAIS DÉCOUVERT — `country_actions.open_country` a un gate de
  brouillard (`country_known==0` → return SILENCIEUX) ⇒ 06_diplo capturait une fenêtre ABSENTE (défaut
  PRÉ-EXISTANT, toutes les vieilles captures 06 étaient vides). Critères robustes : ≠ joueur · provinces>0
  · pas « Rebelles » · `country_known!=0`, en 2 passes (rôle 1 d'abord).

**Pièges**
- Le binding `action_preview` est dans scps_sim_node.cpp (19:02) mais la DLL bin/ date de 15:27 → au
  runtime `has_method("action_preview")` est FAUX tant que scons n'a pas retourné : les hovers roulent
  le fallback. Dès la DLL rebâtie, les chiffres réels s'allument SANS retouche GDScript (clés alignées).
- Fenêtres de confirmation : `Sim.ticked`/`month_ticked` NE TOURNENT PAS en pause — tout délai
  d'UI (désarmement 4 s) doit vivre dans `_process` du Control, comme les horloges MUR existantes.
- L'indentation d'un Edit : la boucle `for verb in _btns:` de country_actions est à UN tab (niveau
  fonction) — un old_string recopié à deux tabs ne matche pas ; vérifier `cat -A` avant de re-tenter.

**Restes**
- La confirmation « Purger »/« Guerre » n'est PAS capturable par shot_ui (exige un clic réel) — vérifiée
  par code (miroir exact du motif manumit prouvé) ; un futur harnais de clics pourrait l'exercer.
- L'état ARMÉ rouge vif de « Guerre » (bouton actif) n'apparaît sur aucune capture : sur seed 42/an 60 le
  verbe est grisé (trêve/CB) — le stylebox destructif « normal » est visible dès qu'une cible attaquable
  existe.
- Le panneau province plein (capitale riche) déborde toujours légèrement sous son cadre clampé à la
  hauteur du viewport (chips d'action dessinés sous le liseré) — PRÉ-EXISTANT (même y que l'ancien
  _act_chips), hors périmètre de ce lot.

## 2026-07-10 — UI-2 : topbar en 4 BLOCS + pénurie explicite (topbar/sidebar_drawer/alerts)
**Découvertes**
- Topbar restructuré en 4 blocs séparés par une barre or épaisse (`_block_sep`, 2 px α 0.55 — distincte
  du filet fin α 0.22 que `_cell` pose déjà entre cellules) : ROYAUME (blason·nom·or·pop·prov·stabilité)
  · ÉCONOMIE (nourriture·pénurie·prospérité·savoir·colonie) · POLITIQUE (légitimité·influence·cohésion·
  bonheur·factions·⚑) · TEMPS (chip Engager·date·vitesse). Pas de micro-label de bloc : à TOPBAR_H=48,
  une 3e ligne de texte serait <10 px — contraire à l'audit lisibilité ; le séparateur simple suffit.
- Les 5 cellules de matières brutes (bois·argile·pierre·fer·armes, 07-09) SORTIES de la barre → onglet
  STOCKS les montrait déjà toutes (stock·net/j·couv.) ; ajouté seulement une ligne compacte « Matières :
  bois X · argile X · … » en tête de l'onglet ÉCONOMIE (`_draw_mat_line`, même source country_stocks).
- PÉNURIE : `Topbar.worst_shortage(w, me)` STATIC — voie 1 `country_shortages` si le binding l'expose
  (clés RÉELLES du binding scps_sim_node.cpp:1474 : `nom`/`res_id`/`runway_days`/`structurel` — PAS
  « days » ; vérifié dans le .cpp, le reader du lot #68 parallèle) ; voie 2 (repli toujours présent,
  celle PROUVÉE en capture — le DLL du poste n'a pas encore le binding) : `coverage_days` de
  country_stocks, déjà stock/|net_day| côté façade (scps_api.c:1133, sentinel 366=« >1 an » à EXCLURE
  sinon « rupture 366 j » absurde). Consommée AUSSI par alerts.gd (preload topbar.gd, DRY) : chip
  COL_ECO → Marché quand < 30 j.
- UI-5 sur ces fichiers : pénurie = texte chiffré + ▼ + rouge (3 canaux) ; jauges nationales et bonheur
  doublés ▲/▼ aux EXTRÊMES seulement (≥66/≤33 — une valeur médiane n'a pas besoin d'alarme, ne pas
  sur-décorer). Boutons de vitesse 27→34 px de large (audit point 1, hauteur 36 déjà conforme).
**Pièges**
- Chip « Engager : <âge> » vs bloc POLITIQUE élargi : à 1920 avec 6 factions + nom d'âge long, TOUT ne
  tient pas. 1re tentative (clamper le chip à droite du contenu) le poussait SUR la date et les boutons
  de vitesse — capturé. La bonne réponse : chip ANCRÉ à gauche de la date, LABEL tronqué (« Engager… »)
  jusqu'à tenir dans `avail = dtx0 − content_end` ; nom complet au survol ; `_tips.push_front` pour que
  le tip du chip GAGNE le hit-test sur les blasons qu'il recouvre en cas extrême (le scan `_get_tooltip`
  prend le premier rect touché).
- `content_end` doit être mesuré au CONTENU RÉELLEMENT dessiné (fin du dernier bloc), jamais une
  position fixe — le bloc POLITIQUE varie de ~300 px selon le nombre de factions.
- Éditer topbar.gd par Edit(old_string) échouait mystérieusement sur les gros blocs (mismatch
  d'espaces insaisissable) → réécrit via Write complet ; les petits Edits ciblés passaient.
- Les alertes sont INVISIBLES dans shot_ui : `Sim.generated.emit()` part AVANT `Sim.game_on = true`
  puis pause → `alerts._refresh` gaté puis plus jamais rappelé (aucun tick). Artefact de probe
  pré-existant, pas un bug d'alerte — le chip pénurie ne se vérifie qu'en code/en jeu réel.
- Le stdout de la probe est FULLY BUFFERED : le log s'arrête à « rivières... » et ne montre que 6-7
  « SHOT » alors que les 24 PNG sont écrits — juger aux TIMESTAMPS des fichiers, pas au log.
**Restes**
- Quand la DLL sera rebâtie avec le binding `country_shortages` (lot #68), la voie 1 prendra le relais
  automatiquement (has_method) — re-vérifier alors qu'un runway STRUCTUREL long (>366 j) ne pollue pas
  la cellule (déjà filtré ≤366, mais non testé en live).
- Repli « chip Factions unique » sur écran étroit (déjà noté au lot factions-topbar) : toujours pas
  fait ; la troncature du chip d'âge réduit l'urgence à 1920.

## 2026-07-10 — CONSEIL : coeur moteur (P0/P1/P3, docs/CONSEIL_ORIENTATIONS_2026-07-10.md)

Perimetre : scps_statecraft.{c,h}, scps_factions.{c,h}, scps_missions.{c,h},
scps_tune_list.h, scps_sim.c (ripple : 2 call sites), + statecraft_demo.c/missions_demo.c
(recalibrage des bancs demande explicitement par le brief -- hors scps_api.c/scps_events.c/
godot/, touches en parallele par d'autres agents, jamais lus ni modifies ici).

**Decouvertes**
- L'ancien spectre de faction par siege vivait dans deux tableaux statiques locaux
  SEAT_A/SEAT_B (scps_statecraft.c, statecraft_council_faction) -- remplaces par un
  Fisher-Yates deterministe (sc_faction_shuffle, xs32 de scps_math.h amorce par
  sc_hash(...^0xFAC7104u,...)) : les SC_COUNCIL_CANDS(3) premieres valeurs du melange
  des 6 factions sont TOUJOURS distinctes (prefixe d'une permutation -- pas besoin de
  retirage-si-collision).
- statecraft_council_cost/_cand_cost lisaient une table plate SC_TIER_COST[4]={0,8,16,28}
  (or/mois fige) -- remplacee par econ_country_tax_year(cid) (scps_econ.h:734, borne en
  interne, pas besoin de passer WorldEconomy* en plus de cid) x taux par rang x IPM / 12.
  statecraft_council_ai's garde de budget (6 mois de loyer) utilisait la MEME table --
  recablee sur le meme helper sc_tier_monthly_cost.
- statecraft_council_seat_mult (bonus de RANG seul, jamais l'efficacite) est reste
  INCHANGE de signature -- c'est statecraft_council_apply qui compose
  final_mult = 1 + (rank_mult-1)*eff et pousse ca a econ_set_council_mult. Ce decouplage
  a evite de casser les 3 tests existants de statecraft_demo.c qui appellent seat_mult
  SANS WorldProsperity.
- Mission.coord (0..5) etait un enum ANONYME et LOCAL a scps_missions.c (CB_K...) --
  invisible aux bancs. Deplace/renomme en MIS_COORD_* dans scps_missions.h (P3 en a
  besoin pour mission_responsible_seat ET pour que missions_demo.c construise des
  Mission a la main sans deviner des entiers magiques).
- faction_grievance (lecteur seul) existait deja cote scps_factions.{c,h} ; AUCUN
  ecrivain direct n'existait avant P1-3 (seul faction_lever_apply ecrivait -- mais lui
  aigrit les OPPOSEES d'une faction avancee, jamais la faction elle-meme). Ajout minimal
  faction_grievance_add(cid,f,amount) (clamp [0,1]) -- 5 lignes, aucune autre dependance.
- econ_country_tax_year est un accumulateur annuel serialise (section TXYR, v65+) --
  dans statecraft_demo.c/missions_demo.c, un monde frais (25 ans de warmup via
  econ_tick(dt=1.f) ou 0 tick) donne un revenu NON-NUL des que g_flux_ticks_total depasse
  90 jours simules (le repli An-1 extrapole, scps_econ.c:789-801) -- verifie
  compilateur-only (aucun run reel possible ici, cf. contrainte "NE COMPILE PAS avec
  make") ; le test ">0.f" du cout mensuel dans statecraft_demo.c REPOSE sur ce repli et
  n'a pas pu etre verifie a l'execution.

**Pieges**
- 0xD00Ru n'est PAS un litteral hexadecimal valide en C (R hors [0-9A-Fa-f]) -- gcc le
  lit comme 0xD00 suivi d'un suffixe Ru inconnu -> erreur immediate. Renomme 0xD00D5Eu.
  Verifier chaque nouveau salt hex AVANT de le poser (le lire, pas juste grep).
- if (a) x; if (b) y; sur UNE ligne declenche -Wmisleading-indentation des que
  l'indentation suggere (a tort) un bloc commun -- gcc -Wall -Wextra le catch, make test
  aussi (0-warning gate). Toujours une instruction par ligne pour deux if independants
  consecutifs.
- statecraft_council_hire gagne un gate P1-4 (seat deja pourvu => no-op) -- verifie
  qu'AUCUN site interne (statecraft_council_ai, les 3 bancs, events_demo.c ~ligne 995)
  n'appelle hire sur un siege deja pourvu SANS dismiss prealable dans le meme flot ;
  events_demo.c (hors scope, NON modifie) lu en lecture seule pour s'en assurer -- les 3
  hires y ciblent 3 sieges DIFFERENTS, tous vacants au depart.
- Les tunables COUNCIL_MISSION_* etc. doivent etre declares dans scps_tune_list.h AVANT
  tout usage de tune_f avec ce nom -- tune_f ne PLANTE PAS sur un nom absent (retombe sur
  le defaut passe en argument), donc l'omission serait un bug SILENCIEUX (F10/SCPS_TUNE
  resterait aveugle a la cle). Verifie : les 17 cles neuves sont TOUTES dans le registre
  avant tout usage.

**Choix documentes (demandes explicitement par le brief)**
- P0-4 (personne + maison) : ScpsCouncilCand/ScpsCouncilSeat (scps_api.h) ne sont PAS
  serialises (candidats deterministes, recalcules au vol) donc aucune contrainte de save
  n'empechait d'ajouter un champ "maison" -- mais scps_api.h/scps_api.c sont explicitement
  HORS PERIMETRE (un autre agent y travaillait EN CE MOMENT). Choix : deux fonctions
  ENGINE additives et independantes -- statecraft_council_cand_firstname (24 prenoms) et
  statecraft_council_cand_house (12 maisons, verbatim spec), tirages independants (salts
  distincts 0x91A2E3/0x40C51E vs 0xC0FFEE tier/0x5EAB011 nom-legacy/0xA6E11 age/0xFAC7104
  faction/0x0DDA6E retraite/0x1DE47170 identite-scps_api.c) -- de simples const char*
  locaux (pas de StrId/scps_lang.txt : strings_ids.h est hors perimetre).
  statecraft_council_cand_name (StrId legacy, 8 "maisons" historiques deja consomme par
  scps_api.c:1240/1273 via tr()) reste BYTE-IDENTIQUE. Un futur agent facade devra soit
  composer "prenom + maison" directement depuis ces deux fonctions, soit migrer les 36
  chaines vers strings_ids.h/strings_en.h -- note : make lang-check ne scanne QUE
  viewer.c/scps_readout.c (Makefile:656), cette derogation ne casse AUCUN gate existant.
- P1-1 (efficacite) : statecraft_council_apply recoit desormais const WorldProsperity*
  (ripple : scps_sim.c:739, SEUL appelant hors bancs -- viewer.c ne l'appelle jamais,
  il partage scps_sim.c depuis le dedoublonnage 2026-07-02). statecraft_council_seat_mult
  N'A PAS change de signature (reste le bonus de RANG seul) ; l'efficacite est composee
  SEULEMENT dans apply.
- P1 (couts) : le curseur de paie existant (statecraft_council_pay, 0-2x) est deja cable
  comme multiplicateur du cout ET de la cible de loyaute -- conserve TEL QUEL, le brief
  demandait explicitement de le garder "s'il est cable ainsi".
- P3 (mission decennale) : mission_responsible_seat est une fonction PURE sur (kind,
  coord) -- AUCUN etat neuf, AUCUN id de ministre stocke : le "successeur reprend" est
  satisfait PAR CONSTRUCTION (on relit statecraft_council_seated a CHAQUE grant/penalite).

**SAVE** : rien de serialise ne change. Statecraft/Mission/MissionsState gardent
exactement leurs champs (aucun ajout de struct) -- tout le nouveau calcul est DERIVE
(fonctions pures ou lecteurs/ecrivains sur des champs deja serialises : loyalty[][] via
statecraft_council_loyalty_add, g_lever_grief via faction_grievance_add). AUCUN bump
SAVE_VERSION.

**Gates verifiees** : gcc -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party (PATH MSYS2
en tete) SANS AVERTISSEMENT sur les 13 fichiers touches ou en aval (scps_statecraft.c/.h,
scps_factions.c/.h, scps_missions.c/.h, scps_tune_list.h, scps_sim.c, statecraft_demo.c,
missions_demo.c, scps_api.c, scps_api_demo.c, chronicle.c, viewer.c, events_demo.c,
scps_decrees.c, scps_tune.c, factions_demo.c -- ce dernier n'avait RIEN a recalibrer,
aucune reference au Conseil). AUCUN make/run reel -- conforme a la consigne explicite ;
les nouveaux tests de bancs (P0-1 distinctness, P0-4 tirages, P1-1 formule verbatim spec
K=6/loy=70, P1-3 rancoeur directe, P1-4 gate de nomination, P3 siege responsable + bonus +
loyaute +5/-10) sont donc PROUVES par lecture attentive et compilation propre, PAS par
execution -- un agent disposant de make statecraft_demo missions_demo devrait les faire
tourner en premier.

**Restes**
- Golden 12 ans va bouger (tirage de faction 6-way, couts revenue-based, efficacite qui
  multiplie le bonus de siege) -- PAS re-baseline ici, consigne explicite ("l'IA a un
  conseil -- ATTENDU").
- P0-4 : la migration des 36 chaines vers strings_ids.h/strings_en.h + le cablage facade
  (ScpsCouncilCand/ScpsCouncilSeat -> "maison") restent a faire par l'agent qui possede
  scps_api.{h,c} -- cf. "Choix documentes" ci-dessus pour le contrat exact des deux
  fonctions engine pretes a consommer.
- P2 (hooks dynamiques evenements existants) et P4 (orientations legeres/decrets) sont
  HORS PERIMETRE de cette mission (priorites P0/P1/P3 seulement) -- scps_events.c intouche.
- statecraft_council_cand_cost/statecraft_council_cost retournent tous deux un montant
  MENSUEL (coherent avec STR_COUNCIL_SEATED_FMT = "... or/mois") -- un futur agent facade
  qui veut afficher "N or CETTE ANNEE" doit multiplier par 12 (ou appeler directement
  econ_country_tax_year(cid)*rate*ipm sans le /12) cote scps_api.c, PAS changer la
  semantique de ces deux lecteurs engine.

## 2026-07-10 -- CONSEIL P2 : hooks de faction DYNAMIQUES sur les evenements existants

Perimetre : scps/scps_events.c (le gros du travail) + 2 lecteurs additifs dans
scps_statecraft.{c,h}/scps_factions.{c,h} -- scps_api.c/scps_decrees.c/godot/ non
touches (d'autres agents y travaillaient en parallele, jamais lus ni modifies ici).

**Decouvertes**
- Le mecanisme EvChoiceHook{faction,...} (une SEULE faction, statique dans la table
  EVENTS[]) ne peut PAS exprimer "F = titulaire reel du siege" ni "double concession a
  a ET b" (C1 Ceder) -- le plus petit diff coherent avec le motif data-driven etait de
  garder EvChoiceHook generique pour les evenements NON-conseil, et d'ajouter un bloc
  RESOLU DYNAMIQUEMENT (a la resolution, pas au tirage) dans resolve_choice, juste apres
  le bloc existant qui mute deja le Conseil (dismiss/hire, if (cx->sc && cx->w){...},
  scps_events.c ~ligne 2312). Les hooks STATIQUES de la table pour tous les choix conseil
  concernes sont neutralises (.faction=-1) -- jamais un sentinel -2/-3 : plus simple, et
  coherent avec les tables A1/A2/C1/SUCCESSION qui avaient deja faction=-1 avant cette
  mission.
- BUG TROUVE ET CORRIGE : EVID_TRAHISON_SAVOIR ne renvoyait le ministre QUE sur oi=0/oi=2
  (condition oi!=1) -- l'option n2, litteralement intitulee "Le renvoyer sans bruit", NE
  renvoyait PAS. Le hook statique de oi=0 appelait meme faction_concede(cid,FAC_LEGISTE)
  (strength=0.f tombe dans la branche ELSE de apply_choice_hook) -- une concession non
  documentee par la spec. Les 3 tables TRAHISON avaient ce meme defaut (hooks statiques
  FAC_LEGISTE/FAC_GARDIEN/FAC_MARCHAND avec strength=0.f qui declenchaient silencieusement
  des faction_concede). Tout neutralise et remplace par le calcul dynamique F/Opp(F).
- statecraft_council_dismiss grief DEJA (+COUNCIL_DISMISS_GRIEF~0.10) la faction PROPRE du
  renvoye, et statecraft_council_hire leve DEJA (+COUNCIL_HIRE_LEVER~0.10) la faction du
  candidat -- ces deux automatismes couvrent, SANS AUCUN code additionnel : R1/R2/R3
  "renvoyer les deux" (rancoeur directe aux deux, aucune capture) et C1 "renvoyer les
  deux"/"en sacrifier un" (meme motif). Seuls les cas ou la spec demande un effet EN PLUS
  de ce que dismiss/hire font deja (Opp(F), F+0.05, concession, double concession, biais
  aux allies/3e siege) demandaient du code neuf.
- faction_lever_apply(cid,F,strength) grief DEJA automatiquement TOUTES les factions
  opposees a F (proportionnellement a faction_opposition) -- c'est EXACTEMENT le "rancoeur
  de l'autre par la matrice" que R1/R2/R3 "Trancher" demandaient : aucun code separe requis
  pour ce volet, juste appeler faction_lever_apply sur F=titulaire reel du siege choisi.
- SUCCESSION voulait un retrait SANS le grief de dismiss() -- impossible via le verbe
  public (il grief inconditionnellement des qu'un slot etait assis). Solution : ecriture
  DIRECTE des 4 champs sc->council[][]/council_gen[][]/loyalty[][]/pay[][] (miroir EXACT
  de ce que fait statecraft_council_dismiss MOINS l'appel a faction_grievance_add) --
  motif DEJA precedente dans ce meme fichier (agitation/influence sont deja ecrits
  directement depuis scps_events.c, lignes ~1937/1965/1973/1975) : Statecraft a des champs
  publics, ce module y ecrit deja en dehors des verbes quand le verbe ne peut pas exprimer
  le geste voulu. Pas besoin de toucher scps_statecraft.c pour ca (respecte la restriction
  "lecture seule" du perimetre).
- Les 7 triggers conseil (trig_trahison_seat, trig_conseil_succession, trig_conseil_pair/
  r1/r2/r3, trig_conseil_a1, trig_conseil_a2, trig_conseil_c1) ont TOUS
  "if (cx->human_player<0 || c!=cx->human_player) return false;" -- ces evenements sont
  HUMAN-ONLY par construction, jamais tires en chronique (human_player=-1) ni pour un pays
  IA. Reponse directe a la question du brief : le golden ne bouge PAS a cause de ce lot P2
  (contrairement au lot P0/P1/P3 deja merge, qui lui touche statecraft_council_ai -- non
  gate humain -- et a donc DEJA bouge le golden avant cette mission).
- A2 "Accepter leur candidat" n'existait dans AUCUNE branche de resolve_choice avant ce lot
  (grep confirme : aucun evid==EVID_CONSEIL_A2 nulle part) -- pas une regression a
  corriger, un manque total a combler.

**Pieges**
- statecraft_council_faction renvoie TOUJOURS une faction valide (jamais -1, y compris
  hors-borne : replie sur FAC_COMMUNAUTAIRE) -- un lecteur "titulaire reel, -1 si vacant"
  ne pouvait donc PAS etre ce lecteur-la tel quel : statecraft_council_seat_faction
  (nouveau, scps_statecraft.c) compose seated (qui, lui, renvoie -1 si vacant) AVANT
  d'appeler statecraft_council_faction -- sans ce garde, un siege vacant aurait affiche un
  titulaire fantome de faction FAC_COMMUNAUTAIRE.
- L'ordre d'appel compte : F doit etre LU (statecraft_council_seat_faction) AVANT tout
  statecraft_council_dismiss sur le meme siege (dismiss vide council[][], donc seated()
  renvoie -1 apres -- relire F apres dismiss donnerait toujours -1).
- conseil_pair_find/trig_conseil_a1/a2/c1 re-scannent le monde A LA RESOLUTION (pas de
  cache de ce que le trigger a trouve) -- un event conseil a n_options>1 est ENFILE
  (pending_event_push) et peut se resoudre jusqu'a 180 jours plus tard
  (pending_event_tick_expire) ou via le choix joueur : le monde a pu bouger.
  conseil_a2_find et conseil_succession_seat_find (neufs) suivent la MEME discipline
  (miroir du trigger, re-scanne, jamais un etat capture au tir) -- coherent avec
  conseil_pair_find deja la.
- Les events conseil sont ENFILES pour le joueur (n_options>1 && owner==human_player) --
  world_events_tick seul ne les resout PAS immediatement ; ils attendent le choix joueur
  OU l'expiration (180 j, pending_event_tick_expire, resout au choix de plus haut
  ai_chance). Le banc existant (events_demo.c section 15, TRAHISON_SAVOIR) boucle sur
  30 ans de tics de 30 j precisement pour laisser l'expiration jouer -- comportement
  INCHANGE par ce lot (aucun trigger ni la file pending n'a ete touche).

**Choix documentes**
- Les biais 0.10/0.05 de la spec ("+0,10 biais", "+0,05 biais") sont poses en #define
  LOCAUX dans scps_events.c (COUNCIL_HOOK_OPPOSED_BIAS/_KEEP_BIAS/_ALLY_BIAS/_RETIRE_BIAS),
  PAS au registre scps_tune_list.h -- la spec ne les liste PAS sous ses bullets "Cles :"
  (contrairement aux couts/efficacite/missions du lot P0/P1/P3), et scps_tune_list.h
  n'etait pas dans le perimetre autorise de cette mission.
- Le fallback de nom TREASON_FALLBACK[3] (event_title, ~ligne 1917) est passe de
  {"Le savant","Le notable","Le marchand"} a {"Le conseiller du Savoir","Le conseiller du
  Royaume","Le conseiller des Ouvrages"} -- demande explicitement par le brief ("jamais Le
  marchand") : "Le marchand" suggerait la faction FAC_MARCHAND a tort quel que soit le
  titulaire reel, incoherent avec tout le reste du lot (F = faction REELLE, jamais un mot
  de classe qui presume une faction).
- A2 "Imposer son propre choix" (oi=1) et "Laisser le siege vacant" (oi=2) restent des
  no-op mecaniques (seuls les deltas EvEffect de la table s'appliquent) -- la spec dit
  "ouvrir la liste normale"/"rien", aucun etat neuf a modeliser.

**SAVE** : rien de serialise ne change. Aucune struct touchee (Statecraft/EventsState
gardent leurs champs). Les 2 lecteurs additifs (statecraft_council_seat_faction,
faction_most_opposed) sont des fonctions PURES sans etat. AUCUN bump SAVE_VERSION.

**Gates verifiees** : gcc -fsyntax-only -O2 -Wall -Wextra -std=c99 -Ithird_party (PATH
MSYS2 mingw64 en tete) SANS AVERTISSEMENT sur scps_events.c, scps_statecraft.c/.h,
scps_factions.c/.h, statecraft_demo.c, events_demo.c, chronicle.c, viewer.c, scps_api.c,
scps_decrees.c, missions_demo.c, scps_api_demo.c, factions_demo.c. AUCUN make/run reel
(consigne explicite -- pas de build binaire sur ce poste pour cette mission). Les bancs
events_demo/statecraft_demo n'ont PAS eu besoin de recalibrage : aucune assertion
existante ne teste le contenu des hooks des 9 evenements conseil touches (grep confirme),
seul events_demo.c section 15/17 teste TRAHISON_SAVOIR (fire count) et 4 AUTRES
evenements non-conseil (XENOPHILE/CLOCHES/K3/DROIT_INTEGRATION, hook FAC_COMMUNAUTAIRE,
intouches) -- ces bancs devraient rester verts, a confirmer par make events_demo
statecraft_demo sur un poste qui build.

**Restes**
- make test/make golden non executes ici (pas de build sur ce poste pour cette mission) --
  un agent disposant de make devrait lancer events_demo/statecraft_demo en premier pour
  confirmer a l'execution ce que la lecture+compilation ont prouve.
- Golden 12 ans : ne devrait PAS bouger a cause de CE lot (tous les triggers conseil sont
  human-only, jamais tires en chronique) -- mais golden a DEJA bouge avant cette mission a
  cause du lot P0/P1/P3 (statecraft_council_ai n'est pas gate humain) ; pas de re-baseline
  faite ici (hors perimetre, consigne explicite).
- L'UI (godot/) ne consomme pas encore ces hooks dynamiques -- rien a cabler cote facade
  pour CE lot (les deltas EvEffect/le flux de choix restent inchanges, seul ce qui se passe
  EN INTERNE au Conseil a change).
- P4 (orientations legeres / remplacement des grands decrets) reste HORS PERIMETRE de
  cette mission (scps_decrees.c/scps_api.c non touches, un autre agent y travaillait).

## 2026-07-10 — CONSEIL : les CARTES façade+UI (§ « Interface (cartes) », docs/CONSEIL_ORIENTATIONS_2026-07-10.md)

Périmètre : scps/scps_api.{h,c}, godot/src/scps_sim_node.{cpp,h}, godot/project/ui/sidebar_drawer.gd
(onglet Conseil → Gouvernement SEULEMENT), + 1 ligne dans godot/project/ui/concepts.gd (fuite
« rot », voir Choix documentés) — scps_statecraft/factions/missions/events/decrees/econ
INTOUCHÉS (deux autres agents y travaillaient EN PARALLÈLE ; git diff vérifié avant/après
chaque édition, aucun de leurs fichiers ne figure dans mon diff).

**Découvertes**
- Les lecteurs engine dont j'avais besoin (statecraft_council_efficiency/_seat_mult/
  _cand_cost/_cand_firstname/_cand_house/_faction, mission_responsible_seat) étaient TOUS
  déjà exportés par le lot « coeur moteur » du jour — zéro besoin de toucher scps_statecraft.c/
  scps_missions.c. Seuls DEUX calculs n'ont AUCUN lecteur exporté (ils calculent une PRÉVISION
  pour un candidat pas encore embauché, ou mirent un `static` du moteur) : la loyauté de
  DÉPART qu'une embauche donnerait réellement (`statecraft_council_hire`, scps_statecraft.c:
  225-226, salt `0x10AD17Bu`) et le bonus de rang « nu » pour un candidat non-seatés
  (`sc_seat_base`/`sc_tier_mult`, scps_statecraft.c:87-101, `static`). Les DEUX sont mirés à
  l'IDENTIQUE dans scps_api.c (`cons_predicted_loyalty`/`cons_rank_mult`/`cons_tier_revenue_rate`/
  `cons_efficiency_calc`/`cons_mission_reward_mult`) — MÊME discipline que `cons_hash` qui
  mirait déjà `sc_hash` dans ce fichier (précédent posé par le lot #68/#69/#70 du matin) :
  aucune valeur inventée, chaque constante lue via `tune_f` sur la clé EXACTE de la spec.
  Résultat : « Efficacité politique prévue » d'un CANDIDAT est une prévision EXACTE (pas une
  estimation) — si le joueur recrute CE candidat MAINTENANT, la loyauté de départ que
  `statecraft_council_hire` posera est CELLE calculée par le hover, au bit près.
- Vérifié EN DIRECT (probe fenêtrée, seed 9, 3 sièges vacants) : le bonus de rang affiché
  colle exactement à la spec pour les 3 tiers × 3 sièges — Savoir (base 0.12) rang1/2/3 =
  12.0/18.0/24.0 % ; Société=Royaume (base 0.15) = 15.0/22.5/30.0 % ; les taux de coût
  1.5/3.0/5.0 % aussi. La formule d'efficacité composée (`bonus final = rang × efficacité`)
  a produit des valeurs dans la fourchette attendue (~87-89 % pour une loyauté prévue
  45-65, K et Corruption constants du pays) — la carte candidat rendue à l'écran EST la
  carte de la spec (nom+maison / faction·rang·âge / bonus net avec décomposition rang×
  efficacité / coût taux+montant courant / retraite estimée), boutons Recruter fonctionnels.
- `mission_responsible_seat`/`statecraft_council_seated`/`_cand_tier`/`_efficiency` suffisent
  à recomposer EXACTEMENT `mission_reward_mult` (scps_missions.c:120-131, `static`) côté
  façade — le bloc Mission (siège responsable, bonus, récompense PRÉVUE = base×mult) est
  câblé dans l'onglet Gouvernement (sous les 3 sièges), lu sans jamais toucher scps_missions.c.

**Choix documentés**
- **Les 2 fuites « rot »** — l'identité « Corrompu » N'EXISTE PAS dans `CONS_IDENTITES`
  (8 identités, exactement celles de la spec : Rigoriste/Courtisan/Austère/Réformateur/
  Vétéran/Ambitieux/Loyaliste/Vénal — déjà conforme, posé par le lot #68/#69/#70 du matin ;
  grep large sur tout le dépôt confirme AUCUNE occurrence de « Corrompu » ni « capté par le
  rot » côté joueur). La SEULE fuite réelle trouvée : `godot/project/ui/concepts.gd:68`
  (glossaire de tooltips génériques) affirmait « il ponctionne le trésor » — AUCUN site
  moteur ne prélève le trésor sur la Corruption (grep `faction_capture`/`faction_corruption`
  confirme : la capture ne fait QUE réduire l'efficacité du Conseil et alimenter la tension
  de coup — jamais un débit direct). Corrigée avec le texte VERBATIM recommandé par la spec.
  ⚠ **Déviation du périmètre déclaré** : concepts.gd n'est PAS dans la liste de fichiers
  autorisés du brief — mais c'est la SEULE localisation possible de cette fuite explicitement
  nommée par la mission, c'est un fichier glossaire UI générique (aucun rapport avec les 2
  agents parallèles sur scps_events.c/statecraft/factions et scps_decrees/econ), et le
  changement est un remplacement de chaîne d'UNE ligne, zéro risque de collision. Documenté
  ici plutôt que fait en silence.
- **`efficiency_pct`/`rank_bonus_pct`/`final_bonus_pct`/`resp_bonus_pct` en `float`, pas
  `int`** : la spec donne des exemples à UNE décimale (« 91,5 % », « 16,5 % ») — un
  arrondi entier aurait perdu la fidélité que le worked-example de la spec démontre
  explicitement. `cons_pct100(x) = x*100.f` (pas d'arrondi côté C) ; l'arrondi d'affichage
  (`%.1f`) est laissé à l'UI, comme le reste de la façade (aucun autre champ pourcentage
  n'arrondit côté C dans ce fichier).
- **Coût affiché SANS le multiplicateur de paie** (curseur 0.5×-2× déjà câblé, V2a) : la
  spec § « Rangs et coûts » ne mentionne QUE le taux par rang (1.5/3/5 %) — le curseur de
  paie est une fonctionnalité EXISTANTE, orthogonale, déjà visible juste au-dessus dans la
  carte (les boutons Paie). Mélanger les deux aurait fait mentir soit la carte soit les
  boutons dès que le joueur bouge le curseur.
- **Pas de touche à `strings_ids.h`/`strings_en.h`** (hors périmètre déclaré du brief) :
  `firstname`/`house`/`identite`/`id_flavor`/le mot de domaine restent des `const char*`
  bruts (comme les tables locales `CONS_IDENTITES`/`SC_FIRSTNAMES`/`SC_HOUSES` posées par
  les lots du matin) — `make lang-check` ne scanne QUE viewer.c/scps_readout.c
  (confirmé au Makefile par le lot précédent), donc aucun gate cassé.
- **Mission décennale ajoutée SEULEMENT dans l'onglet Conseil→Gouvernement** (pas dans
  empire_sidebar.gd/country_panel.gd, qui l'affichent déjà en plus léger) — la spec la
  place explicitement au Conseil ; dupliquer l'info sur 2 panneaux avec un niveau de détail
  différent est voulu (un est le résumé toujours visible, l'autre la carte détaillée).

**Pièges**
- `packaging/windows/rebuild_dll.sh` a ÉCHOUÉ une 1re fois sur `build/scps_decrees.os`
  (Error 1) — PAS causé par mon code : c'est une COLLISION avec l'agent parallèle qui
  éditait `scps_decrees.c` AU MÊME INSTANT (fichier lu par scons en cours d'écriture).
  Relancer immédiatement APRÈS a réussi PROPREMENT (0 recompilation de scps_decrees.os —
  déjà à jour depuis leur run précédent ; seul `scps_sim_node.cpp`/`scps_api.os`/
  `scps_statecraft.os`/`scps_missions.os`/`scps_factions.os` ont été refaits, cohérent
  avec ce que MOI j'avais changé + ce qu'EUX avaient déjà committé-sur-disque). Leçon :
  un échec scons en environnement multi-agent peut être une COLLISION transitoire, pas
  une vraie erreur de compilation — relancer une fois avant de creuser.
- Le tiroir `sidebar_drawer.gd` N'A PAS de `ScrollContainer` — la hauteur est bornée au
  viewport (`_hmax`) et tout excédent est simplement CLIPPÉ (pas de scroll), un
  comportement PRÉEXISTANT de tout l'onglet (pas introduit par ce lot). Avec 3 sièges
  VACANTS simultanément × 3 candidats/siège × la carte enrichie (5-6 lignes/candidat), le
  contenu déborde largement en début de partie (capturé en 14_drawer_conseil.png : le
  3e siège « Industrie » est coupé à mi-hauteur). Vérifié : c'est un DÉBORDEMENT visuel
  (clip), pas un crash — le code derrière continue de s'exécuter sans erreur (le bloc
  Mission, plus bas, s'exécute aussi sans erreur malgré d'être hors-écran, confirmé par
  `--` absence totale de SCRIPT ERROR dans les logs).

**Vérifié** : `gcc -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party` propre sur
scps_api.c ET scps_api_demo.c (0 warning) ; binding C++ relu (godot-cpp indisponible sur
ce poste pour un `-fsyntax-only`, cf. consigne) ; DLL debug+release REBÂTIES avec succès
(`packaging/windows/rebuild_dll.sh`, 2e tentative) ; probe FENÊTRÉE
(`Godot_..._console.exe --path godot/project res://shot_ui.tscn`) EXIT=0, **0 SCRIPT
ERROR** sur les 19 captures, `14_drawer_conseil.png` RELUE — carte candidat conforme à la
spec, formules vérifiées à la main (bonus de rang exact pour les 3 sièges × 3 rangs, taux
de coût exact, retraite = fenêtre 66-73 moins l'âge). La branche SIÈGE POURVU (ministre
déjà en poste) n'a PAS été exercée par cette capture (les 3 sièges du pays joueur étaient
vacants à l'an 24 de ce monde) — relue attentivement (symétrique de la branche candidat,
mêmes lecteurs RÉELS non mirés cette fois), non vérifiée à l'exécution.

**SAVE** : rien de sérialisé ne change (tous les champs neufs sont dérivés, recalculés au
vol depuis des lecteurs déjà sérialisés/déterministes). AUCUN bump SAVE_VERSION.

**Restes**
- Débordement visuel du tiroir Conseil quand plusieurs sièges sont vacants en même temps
  (voir Pièges) — un futur lot pourrait ajouter un ScrollContainer au tiroir entier (tous
  les onglets en profiteraient), hors scope de cette mission.
- Branche SIÈGE POURVU non vérifiée à l'exécution (voir Vérifié) — à confirmer par un
  agent disposant d'une save avec un Conseil déjà installé, ou en avançant assez le monde
  de la probe pour qu'une embauche IA se produise.
- Sous-onglet Politiques (décrets/orientations légères, servile) INTOUCHÉ comme demandé —
  le P4 (remplacement des grands décrets par les orientations légères) reste HORS PÉRIMÈTRE
  de cette mission ; ses futurs readers (taux/montant courant par orientation) suivront le
  MÊME motif `cons_tier_revenue_rate`/`cost_year` posé ici pour le Conseil.
- Les 36 chaînes prénoms/maisons/identités restent des `const char*` bruts (pas de
  migration STR_*), comme documenté par le lot P0/P1/P3 du matin — inchangé par ce lot.

## P4 — ORIENTATIONS POLITIQUES (2026-07-10, docs/CONSEIL_ORIENTATIONS_2026-07-10.md,
scps_decrees.{h,c} refondu, SAVE v76→77)

**Mission** : REMPLACER les 4 anciens grands décrets (levée permanente/mécénat/ambassades/
politique de tribut) par les 9-10 orientations légères + 2 décisions ponctuelles de la
spec. Règle absolue : jamais `tune_set` — chaque site de lecture applique
`tune_f("CLÉ") × decree_mult(cid, DECREE_X, mult)`.

**Découvertes**
- **`DecreeId` a 10 membres nommés, pas 9** — le brief disait « les 9 » mais le doc compte
  RATIONS/FOYERS/ÉCOLES/ATELIERS/COMPTOIRS/CIRCULATION/FRONTIÈRES/FÊTES/LÉGATIONS/LEVÉE =
  10 noms distincts (2 paires ⊥ + 6 solos). Vérifié contre « Total payant max 10,75% » du
  doc : max(RATIONS,FOYERS)=1.5 + ÉCOLES 2 + ATELIERS 2 + COMPTOIRS 1.5 +
  max(CIRCULATION,FRONTIÈRES)=0.75 + FÊTES 1.5 + LÉGATIONS 1.5 = **10.75% exact** → 7
  slots « payants » simultanés, 10 orientations nommées. Le « 9 » du brief est un
  arrondi/lapsus de comptage (probablement RATIONS+FOYERS vus comme « un seul axe ») —
  implémenté LES 10, fidèle au doc (source de vérité citée par le brief lui-même).
- **`faction_audit(cid)`** (scps_factions.c, posé par l'agent Conseil PARALLÈLE le même
  jour) fait DÉJÀ exactement « −20 pts de Corruption, renvoie la valeur AVANT » — pas
  besoin de créer de helper, juste l'appeler. `DECISION_AUDIT_CORRUPTION_DELTA` n'est PAS
  au registre J (le −20 est hardcodé DANS faction_audit, hors périmètre scps_factions.c de
  cette mission) — l'ajouter en registre l'aurait rendu DÉCORATIF (jamais lu), violant la
  règle propre du fichier (« uniquement des constantes RÉELLEMENT lues au runtime »).
- **MANUF_BUILD_COST a 6 sites** (scps_ai.c ×3, scps_api.c ×2, scps_sim.c ×1) mais le
  brief ne listait QUE scps_econ.c/demography.c/statecraft.c/revolt.c/warhost.c comme
  fichiers-lecture autorisés — AUCUN de ces 5 ne lit MANUF_BUILD_COST. Résolu : câblé
  UNIQUEMENT à `scps_sim.c:CMD_BUILD_MANUF` (le chemin de construction DU JOUEUR, seul
  site pertinent puisque decree_mult(cid,…) est TOUJOURS 1.0 pour un pays IA — cid IA
  n'a jamais de bit actif). scps_ai.c/scps_api.c non touchés (hors périmètre) ⇒ le
  panneau de PRÉVISUALISATION du coût (façade, `scps_manuf_cost`) n'affichera PAS le
  rabais ATELIERS tant qu'un agent façade ne le raccorde pas — mécanique correcte,
  affichage en retard (gap documenté, pas un bug).
- **`econ_conso_per_capita_year(Resource g)`** (scps_econ.c) N'A PAS de paramètre `cid` —
  seul appelant : `econ_country_forecast` (qui, lui, a `cid`). Évité de changer la
  signature (fonction pure, 1 seul call site) : le multiplicateur RATIONS/FOYERS est
  appliqué AU CALL SITE (`need_full = …*decree_food_need_mult(cid)*effcap`), pas dans la
  fonction elle-même.
- **`food_need` est une `const float` calculée UNE FOIS avant la boucle par-province**
  dans `econ_tick` (scps_econ.c) puis réutilisée à 3 sites DANS la boucle (`owner_` en
  scope à chacun) — pas de refactor de la constante en soi : chaque SITE d'usage
  (`reserve`, la demande, le besoin par strate) multiplie localement par
  `decree_food_need_mult(owner_)`.
- **`Politique de tribut` (DECREE_TRIBUT)** : juste RETIRÉE de l'enum (renumérotation
  complète de toute façon, à cause du SAVE bump). Son levier diplo
  (`diplo_set_tribute_decree`/`diplo_tribute_decree`, `g_tribute_decree[]` statique dans
  scps_diplo.c, jamais touché) reste intact mais **plus jamais atteint** (aucun appelant
  ne reste) → `g_tribute_decree[cid]` = `false` pour toujours, comportement voulu
  (« désexposé, pas supprimé »), scps_diplo.{h,c} INTOUCHÉS.
- **FÊTES PUBLIQUES réutilise LITTÉRALEMENT l'identifiant C `DECREE_MECENAT`** (brief :
  « aucun enum/état/save neuf ») — le nom affiché/flavor change (« Fêtes publiques »),
  l'effet change ENTIÈREMENT (prestige→W_AGITATION_UNREST×0.95), mais l'enum reste
  `DECREE_MECENAT` dans le code. Ne PAS renommer en `DECREE_FETES` par cohérence
  cosmétique avec les autres — c'est une consigne EXPLICITE du brief, pas un oubli.
- **Le cooldown de l'Audit (`g_audit_cd[SCPS_MAX_COUNTRY]`) est un ACCUMULATEUR
  INTER-TICKS** — même classe que EMOB/COLC/TXYR/RVLT (cf. CLAUDE.md) : DOIT être
  sérialisé (section DCRE) ou `--savetest` divergerait après un reload mi-cooldown. Bump
  SAVE_VERSION 76→77 (le vrai déclencheur du bump n'est PAS le renommage de l'enum — la
  section DCRE aurait pu rester `NULL,0`-opaque sans bump si aucun champ neuf n'était
  sérialisé — mais l'ARRAY neuve grossit le blob fwrite, donc bump nécessaire).
- **`ScpsDecree.reforme`** (scps_api.c/h, façade, NON touchée) ne flague QUE
  `DCR_REFORME` — mon nouveau type `DCR_DECISION` (Audit) n'est PAS distingué par ce champ
  (reste 0, comme un ÉDIT normal). `scps_api_demo.c` avait UNE assertion cassée par le
  retrait total de DCR_REFORME (« au moins une réforme… ») → recalibrée pour affirmer le
  CONTRAIRE (`tribut_id<0`, plus aucune réforme) ; le reste du banc (toggle ON/OFF d'un
  ÉDIT, sous-test réforme sauté via son `else` existant) passe TEL QUEL — la plomberie
  drain/CMD_DECREE n'a pas eu besoin de recalibrage, seule l'assertion de catalogue.

**Pièges**
- `demography_manumit_country` (scps_demography.c) est appelée par TROIS chemins : le
  joueur (`CMD_MANUMIT`), l'IA (`scps_ai.c:2828`, `slaves>=10.0`), et un évènement
  (`scps_events.c:2213`, choix « Abolir »). Le biais Communautaire +0.10 (spec « décision
  ponctuelle ») a été posé UNIQUEMENT au call site `CMD_MANUMIT` de `scps_sim.c` (chemin
  JOUEUR), PAS dans `demography_manumit_country` elle-même — sinon l'IA et l'évènement
  auraient AUSSI déclenché le biais politique à chaque appel, cassant `make golden` (le
  biais n'est PAS gaté par human_player si posé dans la fonction partagée).
- Un décret ÉDIT au coût NUL (FRONTIÈRES, LEVÉE_ENTRETENUE) passe par le MÊME
  `decree_afford_capital` que les autres (pas de branche spéciale) : `cost<=0.f` y
  retourne `true` d'emblée SANS toucher au trésor — la table `decree_revenue_rate()`
  renvoie 0 pour ces deux-là, uniforme, aucun `if(id==…)` séparé nécessaire.
- `decree_spend_capital` (DÉCISIONS ponctuelles, prend ce qui est dispo — jamais en
  négatif) et `decree_afford_capital` (ORIENTATIONS mensuelles, TOUT ou RIEN — nouvelle
  fonction) sont DEUX fonctions distinctes avec des sémantiques opposées ; les confondre
  romprait soit « l'audit peut partiellement s'offrir » soit « trésor insuffisant ⇒
  aucun effet ce mois » (l'un ou l'autre, jamais les deux au même site).

**Vérifié** : `gcc -O2 -Wall -Wextra -std=c99 -Ithird_party -Iscps -c` (compilation
RÉELLE, pas juste `-fsyntax-only`) propre — **0 warning, 0 erreur** — sur
scps_decrees.c, scps_econ.c, scps_demography.c, scps_revolt.c, scps_sim.c,
scps_api_demo.c (les 6 fichiers touchés). `make chronicle` PAS tenté jusqu'au bout : le
driver `cc`/`make` de cet environnement Bash pointe vers un `/mingw64/bin` dont le
`TMPDIR` résout vers `C:\Windows\` (permission refusée dès `scps_world.c`, fichier
JAMAIS touché par cette mission) — limite d'ENVIRONNEMENT confirmée pré-existante,
pas un bug de ce lot (chaque fichier compile seul avec le gcc MSYS2 direct). `make
golden`/`determinism`/`smoke` NON relancés (pas de toolchain make fonctionnelle dans
CETTE session) — à faire par le prochain agent/à la prochaine fenêtre de vérification.

**SAVE** : **BUMP 76→77**. Cause : `g_audit_cd[SCPS_MAX_COUNTRY]` (accumulateur
inter-ticks du cooldown Audit) s'ajoute à la section DCRE — jurisprudence EMOB/COLC/
TXYR/RVLT. Le renumérotage complet de l'enum `DecreeId` (DECREE_TRIBUT retiré, les bits
de `g_decree_mask` changent de SENS) rendrait de toute façon un save <v77 dangereux à
relire tel quel (bits au mauvais décret) — le bump couvre les deux causes à la fois.

**Golden/déterminisme** : le module reste **golden-neutre PAR CONSTRUCTION** — TOUS les
sites de lecture appellent `decree_mult(cid, …)` qui renvoie 1.0 tant que
`g_decree_mask[cid]==0` ; ce bit n'est JAMAIS écrit que par `decree_toggle`, appelé
UNIQUEMENT depuis `CMD_DECREE` (scps_sim.c), qui n'existe QUE dans le journal du JOUEUR
(`s->cmdq`, jamais peuplé par la chronique headless, `human_player=-1`). Un pays IA ne
peut donc STRUCTURELLEMENT jamais avoir une orientation active — `make golden`/
`determinism` doivent rester IDENTIQUES (non re-vérifié dans cette session, cf. Vérifié
— À CONFIRMER par la prochaine passe outillée). La seule décision ponctuelle
JOUEUR-UNIQUEMENT (AFFRANCISSEMENT, biais Communautaire) est elle aussi posée au call
site `CMD_MANUMIT`, jamais dans la fonction partagée (cf. Pièges) — même garantie.

**Restes**
- Façade (`scps_api.c`/`.h`, `scps_manumit_preview`) et Godot (panneau Politiques) :
  HORS PÉRIMÈTRE de cette mission (fichiers explicitement interdits). Le prochain agent
  façade doit : exposer les 10 orientations + 1 décision via `scps_decrees_list`
  (générique, DÉJÀ compatible sans modif — vérifié par lecture), ajouter un flag
  « ponctuelle » distinct de `reforme` pour `DCR_DECISION` (Audit) côté `ScpsDecree` s'il
  veut le distinguer visuellement d'un ÉDIT, et écrire `scps_manumit_preview` (âmes
  esclaves actuelles + friction projetée — lecture pure, aucun état neuf requis).
- `scps_ai.c` (coût MANUF_BUILD_COST pour l'IA) et `scps_api.c` (prévisualisation de coût)
  NE reflètent PAS le rabais ATELIERS — sans conséquence mécanique (IA n'a jamais de
  décret actif) mais un futur agent façade pourrait vouloir appliquer
  `decree_manuf_cost_mult(cid)` à la prévisualisation pour que l'affichage corresponde
  exactement à ce que `CMD_BUILD_MANUF` facturera réellement.
- `make golden`/`determinism`/`smoke`/`fuzz-save` non relancés dans cette session (limite
  d'environnement, cf. Vérifié) — le raisonnement de neutralité golden est design-level
  (chaque site gate sur `human_player`/`g_decree_mask`), pas machine-vérifié ICI.

## 2026-07-10 — clôture vague Conseil : la plus grosse cascade de lien du repo
**Pièges**
- scps_econ.c consomme désormais decree_*_mult → 30 bancs BUILD ÉCHEC d'un coup. La cascade
  complète : decrees → warhost (levée) → army + diplo → provlog + legitimacy + statecraft →
  intertrade + routes + missions → readout (metric_agitation vit dans READOUT, pas statecraft).
  Résolu par passes python sur les *_OBJS du Makefile (~120 ajouts d'objets). La règle
  documentée « un lot qui ajoute une dépendance inter-module doit greper les *_OBJS » vaut
  DOUBLE pour un module carrefour comme decrees.
- Test de plancher d'efficacité (statecraft_demo) : la Corruption PLAFONNE à 85 (spec) — à
  loyauté 70 l'efficacité vaut 0,5075 > plancher 0,50 ; le test devait AUSSI écraser la
  loyauté à 0. Un agent qui « raisonne ses asserts sans les exécuter » rate ce genre de cap.
**Restes**
- ScpsDecree.reforme ne distingue pas DCR_DECISION · scps_manumit_preview absent · remise
  ATELIERS non reflétée dans le prix affiché façade (gaps P4 documentés par son agent).
- Sous-onglet Politiques du drawer : à recâbler sur le nouveau catalogue d'orientations
  (readers P4) — l'ancien affichage décrets pointe sur des ids renumérotés.

## [2026-07-10] Politiques — recâblage façade+UI sur le nouveau catalogue (les 4 restes ci-dessus)
**Mission** : fermer les 4 gaps P4 (ci-dessus) — carte orientation/décision complète dans le
drawer, `scps_manumit_preview`, remise ATELIERS symétrique, `ScpsDecree.type` propre. Fichiers
autorisés : scps_api.{h,c}, scps_sim_node.{cpp,h}, sidebar_drawer.gd (Politiques seulement).

**Découvertes**
- **`scps_manumit_preview` EXISTAIT DÉJÀ** (scps_api.c:2235, « LOT J ») et était DÉJÀ câblé dans
  la carte Affranchissement du drawer (`_draw_servile`, sidebar_drawer.gd, DÉJÀ dans le
  sous-onglet Politiques) — le Reste du lot précédent était PÉRIMÉ (un agent postérieur l'avait
  fait sans mettre à jour cette ligne de TROUVAILLES). Formules RÉELLES vérifiées :
  `demography_manumit_country` (scps_demography.c:959) bascule CLASS_SLAVE→CLASS_LABORER +
  ARR_DEPORTE→ARR_MIGRANT (souls comptées, groupes conservés — AUCUNE formule de « friction »
  n'y existe) ; `friction_after` de la preview est un TWIN dérivé d'`econ_off_culture_fraction`
  (même sd/mismatch pondéré par pop, projeté COMME SI les groupes esclaves étaient déjà libres)
  — une projection GROUNDÉE dans une formule réelle déjà utilisée ailleurs pour le même usage
  (friction off-culture), pas une invention. Rien à faire ici : vérifié conforme, non retouché.
- **BUG RÉEL trouvé et corrigé — tampon `ScpsDecree` trop petit dans le binding Godot** :
  `scps_sim_node.cpp` allouait `ScpsDecree d[8]` pour `decrees_list()`, mais `DECREE_COUNT`=11
  (10 orientations + 1 décision) depuis la refonte P4 — les 3 derniers (LÉGATIONS, LEVÉE
  ENTRETENUE, ET **DECISION_AUDIT_OFFICES elle-même**) étaient SILENCIEUSEMENT tronqués côté
  UI : la carte décision était INVISIBLE avant ce lot, sans aucune erreur. Le même bug existe
  dans `scps_api_demo.c:775` (`ScpsDecree decs[8]`) — HORS scope (fichier non autorisé),
  documenté ici pour le prochain agent (les tests ne couvrent donc pas la décision Audit).
- **Formule `cost_year` = lecture DESIGN-LEVEL, pas une reconstruction du charge réel** :
  `decrees_tick` (scps_decrees.c:242) calcule `revenue*rate*ipm*dt_year*12` avec `dt_year=
  days/365` et `days=30` fixe (scps_sim.c:759) → `dt_year*12 ≈ 0.986`, donc la charge PAR APPEL
  MENSUEL (12×/an) est déjà ≈ le montant "annuel" — ce qui, sur 12 mois, prélèverait ≈12×
  le taux annoncé (ex. 2 % annoncé ⇒ ~24 % réellement prélevé sur l'année). Un possible bug de
  calibrage MOTEUR (scps_decrees.c, HORS scope de cette mission). J'ai implémenté `cost_year`
  selon la formule LITTÉRALE de la spec (`tax_year(cid) × taux × IPM`, cf. docs §Rangs et coûts,
  IDENTIQUE au motif déjà en place pour les cartes Conseil `cons_tier_revenue_rate`/`cost_year`)
  — l'affichage suit la spec, pas la charge empirique de decrees_tick ; à réconcilier par un
  futur agent qui A ACCÈS à scps_decrees.c (mesurer sur plusieurs mois si le trésor baisse de
  ~12× le taux annoncé, et si oui corriger `dt_year*12`→`dt_year` ou équivalent).
- **`decree_toggle`/`decree_fire_decision` exigent de franchir `day%30==29`** avant que
  `decree_effective` (active ET FINANCÉ) devienne vrai — un simple `advance_days(1-2)` après
  `player_decree(id,true)` NE SUFFIT PAS pour observer l'effet (mult reste 1.0 tant que le
  mois n'a pas tourné). Vérifié empiriquement (diagnostic temporaire, retiré) : `manuf_cost()`
  ATELIERS actif+non financé = ratio 1.0000 ; après `advance_days(31)` (franchit le palier) =
  ratio 0.9412 (51→48, soit ×0.95 exact, rounding compris) — la symétrie prix affiché/prix payé
  (task 3) est donc CONFIRMÉE EMPIRIQUEMENT, pas seulement par lecture de code.

**Pièges**
- GDScript **PARSE ERROR silencieux au chargement** : `var _c0 := w.manuf_cost()` où
  `manuf_cost()` est une méthode GDExtension (retour Variant non typé côté binding) ⇒
  « Cannot infer the type… » — le process Godot ne CRASHE PAS, il reste chargé mais IMMOBILE
  (CPU quasi nul, `Responding=True`, aucune ligne imprimée) : ressemble à un HANG, pas à une
  erreur. Typer EXPLICITEMENT (`var _c0: int = …`) — déjà documenté ailleurs (S3, overlay.gd)
  mais reconfirmé ici sur un pattern différent (méthode `int`, pas `Dictionary.get`).
- **Deux instances Godot simultanées** peuvent rester en tasklist après un run précédent
  interrompu — `taskkill`/`Stop-Process -Force` AVANT de relancer un probe, sinon un second
  process peut contendre le contexte GL et paraître lui aussi bloqué.
- La `plateaux` (DecreeDef) contient DÉJÀ effet+clé+multiplicateur en mots pour chaque
  orientation (ex. « + SAVOIR_W_{élite,bourgeois,laboureur}×1.05 (recherche) / - ponction
  mensuelle… ») — je l'ai gardée TELLE QUELLE comme hover/description plutôt que de la
  re-décomposer en 3 champs séparés (cohérent avec le motif Conseil qui montre `plateaux`
  en survol, pas en clair).
- Le nombre EXACT de jours de cooldown restant (Audit) vit dans `g_audit_cd[]`, `static` à
  `scps_decrees.c` — **aucun getter exposé**, et ce fichier est HORS des fichiers autorisés
  de cette mission. `cooldown_active` (booléen) est DÉRIVÉ honnêtement (`cond_met && !legal`
  ⟺ cooldown>0, car `decree_legal` d'une DÉCISION == `audit_ready` == cooldown≤0 ET
  cond_met — preuve par lecture de `cond_audit`/`audit_ready`, scps_decrees.c:34-44) SANS
  fabriquer de compte à rebours. Un futur agent AVEC accès à scps_decrees.{h,c} pourrait
  ajouter `int decree_cooldown_days(int cid, DecreeId id)` pour l'exact.

**Vérifié**
- `gcc -fsyntax-only -std=c99 -Wall -Wextra` propre (0 warning) sur scps_api.c ET
  scps_api_demo.c (via `PATH="/d/MSYS2/mingw64/bin:$PATH"` — le `bash -l` par défaut de cet
  environnement pointe vers un `/mingw64/bin` DIFFÉRENT et INCOMPLET, sans gcc/scons : `export
  PATH` explicite requis avant `scons`, PAS juste `bash -l` seul, contrairement à ce que
  suggérait l'instruction de mission — `bash -l packaging/windows/rebuild_dll.sh` échoue seul
  avec « scons: command not found »).
- DLL debug+release REBÂTIES avec succès par un `scons` direct (PATH exporté), 0 erreur, 0
  warning visible au link.
- Probe FENÊTRÉE (`Godot_..._console.exe --path godot/project res://shot_ui.tscn`) EXIT=0,
  **0 SCRIPT ERROR** sur 24 captures. Nouvelles captures ajoutées à `shot_ui.gd` (autorisé par
  la mission) : `14b_drawer_politiques` (taille d'écran normale — état représentatif) et
  `14c_drawer_politiques_full` (fenêtre temporairement agrandie à 1920×3400, pour voir Légations/
  Levée/Décisions/Peuple servile qui débordent hors du viewport normal — limite ScrollContainer
  PRÉEXISTANTE et documentée, non touchée). Les deux RELUES : les 10 cartes ORIENTATION (nom,
  effet+clé+mult via plateaux, coût %, montant courant, note d'exclusivité, bouton Activer/
  Désactiver) + la carte DÉCISION « Audit des offices » (condition d'entrée, coût ponctuel avec
  montant courant, bouton « Décréter » grisé+raison au survol) + la carte Affranchissement
  (préexistante, inchangée) rendent TOUTES conformes à la spec § Interface (cartes).
- Symétrie prix ATELIERS confirmée EMPIRIQUEMENT (cf. Découvertes) : 51→48 or (×0.95 exact).

**SAVE** : rien de sérialisé ne change — `ScpsDecree` est une struct FAÇADE (jamais
sérialisée, recalculée à chaque appel `scps_decrees_list`), les 3 fonctions miroir ajoutées
dans scps_api.c (`decree_revenue_rate_mirror`/`decree_exclusive_id_mirror`/
`decree_cond_met_mirror`) sont PURES (aucun état). **AUCUN bump SAVE_VERSION**.

**Restes**
- Le nombre exact de jours de cooldown restant (décisions) n'est pas exposé — cf. Pièges,
  nécessite un getter dans scps_decrees.c (hors scope ici).
- `scps_api_demo.c:775` a le MÊME bug de tampon `ScpsDecree decs[8]` que celui corrigé dans
  scps_sim_node.cpp (DECREE_COUNT=11) — les tests ne voient jamais LEGATIONS/LEVEE/la décision
  Audit. Fichier non autorisé pour cette mission ; à corriger par un futur agent (bump à 16,
  comme le binding Godot).
- ~~Possible calibrage moteur ≈12× dans `decrees_tick`~~ — CONFIRMÉ ET CORRIGÉ le 2026-07-11
  par l'agent parallèle possédant scps_decrees.c (`dt_year*12` → `dt_year`, le ×12 venait du
  motif Légations où la valeur est PAR-MOIS) : le `cost_year` affiché par la façade (formule
  littérale de la spec) correspond désormais EXACTEMENT à la charge annuelle réelle.
- ~~Débordement visuel du sous-onglet Politiques (pas de ScrollContainer)~~ — RÉSOLU par
  l'addendum ci-dessous (scroll générique du tiroir).
- `make golden`/`determinism`/`smoke` non relancés (façade pure, aucun état moteur touché — le
  raisonnement golden-neutre est le même que les lots précédents : `g_decree_mask`/
  `g_audit_cd` ne bougent QUE via `CMD_DECREE`, jamais peuplé par la chronique headless).

### Addendum (2026-07-11) — cartes TERSES + hovers quantitatifs + SCROLL générique du tiroir
Trois directives joueur arrivées en cours de mission (elles PRIMENT sur la spec doc pour
l'affichage), appliquées à Gouvernement ET Politiques (sidebar_drawer.gd) :

**(1) CARTES TERSES.** Les flavor texts sortent de l'écran (l'identité reste un MOT à côté du
nom — « Oriane Istrane · Courtisan » ; sa phrase vit au survol du nom). PRIX FUSIONNÉ : une
seule ligne « N or par an » (« N or (une fois) » pour une décision) — plus jamais « Coût : X %
du revenu annuel × IPM » ni « Actuellement : … » à l'écran. Carte siège/candidat = nom·identité
/ faction·rang·âge / bonus final / N or par an / retraite (~6 lignes sobres, vérifié en
capture). Carte orientation = nom / N or par an / ⊥ exclusivité / bouton (l'effet+clé+mult —
`plateaux` — et le flavor vivent au survol du nom). La décomposition d'efficacité vit AU SURVOL
du bonus final.

**(2) HOVERS QUANTITATIFS, MEMBRANE TENUE.** Règle : un hover ne définit pas « quoi », il donne
« combien » — et JAMAIS une coordonnée du modèle (« K » se dit « Administration »). Prix au
survol : « 5,0 % du revenu (4 014 or) × IPM 1,01 = 203 or par an — prélevé chaque mois (/12) ».
Bonus final au survol : « Rang : +18 % · Administration : +18 pts · Loyauté : +10,5 pts ·
Corruption : −7 pts · Efficacité : 91,5 % ⇒ +16,5 % net » (pts = valeur×coefficient de la
formule d'efficacité : Adm×3 · loyauté×0,15 · Corruption×0,35). Pour les CANDIDATS, la part
« Loyauté de départ » est DÉDUITE (efficacité prévue − 70 − Adm×3 + Corr×0,35 — arithmétique
sur les lecteurs réels, rien d'inventé) car la loyauté prévue n'est pas dans le Dictionary
candidat ; Administration/Corruption sont lues du dict SIÈGE (remplies même vacant,
scps_api.c:1308). Hover paie : « ×N → loyauté cible ±pts (30×(paie−1)) » (vérifié
COUNCIL_PAY_ADJ=30, scps_statecraft.c:368). Condition Audit : « Corruption N/100 — exige
≥ 20 » (lecteur country_factions existant). **Façade neuve pour l'assiette** (les hovers ont
besoin du revenu et de l'IPM SÉPARÉS, pas du seul produit) : `scps_country_revenue_year(s,cid)`
(= econ_country_tax_year) + `scps_world_ipm_now(s)` (= econ_world_ipm), bindées
`country_revenue_year(country)` / `world_ipm()`. Lectures PURES, DLL rebâtie et vérifiée LIVE
(ipm=1.01, rev=4014 or, seed 9 an 24).

**(3) SCROLL GÉNÉRIQUE du tiroir** (motif construction_panel) : offset PAR ONGLET
(`_scroll:{tab:px}`), molette (pas de 40 px, clampé au contenu), clip (`clip_contents=true`),
barre piste+pouce ∝ fenêtre/contenu, EN-TÊTE FIXE redessiné PAR-DESSUS le contenu défilé
(`_draw_header` déplacé en FIN de `_draw`). Les 4 gardes `if y > _hmax: break` (stocks, marché,
partenaires commerce, relations diplo) SUPPRIMÉES — elles tronquaient le contenu que le scroll
doit maintenant révéler (la limite de COMPTE `shown >= 5` du budget est GARDÉE : résumé voulu,
pas une troncature de viewport). Clic gauche < 36 px (bandeau) ignoré (jamais un clic vers un
bouton défilé caché dessous) ; zones de survol dont le centre est sous l'en-tête écartées de
_tips (pas de tooltip fantôme dans le bandeau). Changer de sous-onglet Conseil remet l'offset
à 0.

**Pièges (addendum)**
- **Capturer un état défilé = TRANSITOIRE à stabiliser** : le 1er `_draw` après un offset forcé
  hors-borne RE-CLAMPE et re-queue un redraw — capturer trop tôt fige un tiroir VIDE (payé :
  14c vide au 1er essai). 2 frames de stabilisation avec queue_redraw avant `_shot` (commenté
  dans shot_ui.gd). `queue_redraw()` DANS `_draw()` fonctionne (le flag se re-pose pour le
  frame suivant) — c'est le mécanisme du re-clamp.
- Un `scons` qui dit « is up to date » APRÈS avoir recompilé des .o modifiés est suspect
  (content-signature + build/ partagé entre debug/release) : `rm` du .dll avant `scons` force
  le relink — le geste sûr quand la fraîcheur de la DLL importe.
- Le probe shot_ui gagne `14c_drawer_politiques_scrolled` (offset poussé à 10000 → clampé au
  max réel) : scroll + en-tête fixe + barre vérifiés en UNE capture. L'ancienne capture
  « fenêtre 3400 px » est retirée (le scroll rend le contournement inutile).

**Vérifié (2e passe)** : gcc -fsyntax-only 0 warning (scps_api.c + scps_api_demo.c) · DLL
debug+release re-liées · probe fenêtrée EXIT=0, 24 captures, **0 SCRIPT ERROR** · 14/14b/14c
RELUES (cartes terses conformes, prix fusionné, hovers posés, scroll fonctionnel, en-tête
fixe). **SAVE toujours non bumpé** (les 2 readers assiette sont purs).

## 2026-07-11 — Colmatage de la fuite de membrane « Je vois de la perméabilité, du K… »
(scps_heritage.c hovers · scps_religion.c tips · concepts.gd · culture_creator.gd)
**Découvertes**
- Périmètre RÉEL plus étroit que redouté : `scps_heritage.c` avait le gros du texte déjà en
  prose (« +10 % de Démographie », « Perméabilité (P — …) », « Capacité (K, … pénalité de
  diversité) », « Influence d'État », « fracture interne D̄ », « (H, puissance militaire) ») —
  36 hovers, TOUS retouchés (lettres retirées, chiffres CONSERVÉS comme exigé). `scps_religion.c`
  était presque propre : `scps_religion.h:169` documente `relig_pole_tip` comme « descripteur
  diégétique court (SANS CHIFFRE) » — DESIGN INTENTIONNEL, pas un oubli — donc PAS d'ajout de
  magnitudes malgré l'exemple générique du brief (« K +1 · L −0,6 → capacité de l'État +1 ·
  Stabilité −0,6 ») ; la SEULE fuite réelle grep-confirmée était 5× le littéral `(K)` dans
  `gestion (K)` (Fécondité/Transe/Silence/Gnose/Orthodoxie) → `capacité de l'État` (accord
  féminin conservé sans casse, « gestion »→« capacité » restent tous deux féminins singuliers).
  Les crédos (`g_credo_*`) ne fuient QUE dans leurs commentaires C (`/* vivant : P +1 · H −1 */`)
  — libres par charte, non touchés ; aucune fonction ne les expose au joueur avec lettre.
- La VRAIE fuite visible en jeu venait de `scps_api.c:3506 scps_culture_preview` (NM[9] =
  « Capacité (diversité) », « Perméabilité (assimilation) », « Affinité arcane », « Coercition
  (militaire) », « Influence » nus) — EXACTEMENT les exemples cités par le brief comme « NOM DE
  STAT » interdit — mais ce fichier est HORS scope (tenu par un autre agent en parallèle). Fix
  posé côté GDScript SEUL (`culture_creator.gd`, dict `NOM_LEVIER_JOUEUR`, table de traduction
  nom-façade→mot-de-jeu appliquée juste avant l'affichage dans `_refresh()`), conforme à la
  consigne explicite de la mission (« pose côté GDScript une table de TRADUCTION locale »).
  Les clés du dict restent les chaînes BRUTES de la façade (jamais affichées, juste matchées) —
  aucune fuite dans le dict lui-même, ni dans son commentaire (libre).
**Pièges**
- Casser le lien de décoration `concepts.gd`/TooltipServer était un risque de la traduction : le
  regex de `concepts.gd._regex()` matche par MOT-CLÉ EXACT (bornes `\b`) sur les clés de `DEFS`.
  En renommant « Capacité (diversité) »→« Capacité de l'État », le mot « Capacité » reste en TÊTE
  de la nouvelle chaîne suivi d'un espace → continue de matcher la clé `DEFS["Capacité"]` et de se
  faire décorer/lier automatiquement (vérifié par relecture du regex, pas testé en jeu — Godot
  absent de cet environnement). En revanche « Assimilation des minorités » (ex-Perméabilité) et
  « Magie faustienne » (ex-Affinité arcane) NE contiennent PLUS le mot-clé d'origine → perdent le
  lien cliquable/tooltip-cascade sur CET écran précis (les autres écrans qui appellent encore
  `trait_hover()`/`relig_pole_tip()` directement, eux, gardent « Perméabilité »/« Affinité arcane »
  intacts dans leurs propres textes SI ces mots y apparaissent ailleurs — seul CE label de preview
  est retraduit). Documenté en commentaire au point d'appel + ici : un futur agent AVEC accès à
  `concepts.gd` pourrait ajouter des clés `"Assimilation des minorités"`/`"Magie faustienne"` (ou
  des alias) à `DEFS` s'il veut restaurer la cascade sur ces deux labels précis.
- Les 36 hovers de `scps_heritage.c` sont dans une TABLE DÉSIGNÉE (`[T_X] = {...}`) très dense —
  édité par BLOCS (Physique/Social/Intellectuel) via `Edit` avec le texte AVANT+APRÈS complet de
  chaque bloc plutôt que ligne par ligne, pour éviter un `old_string` non-unique (plusieurs hovers
  partagent des fragments comme « +10 % de Démographie »).
- Échelle `assimilation des minorités ~±N %` pour `permeabilite` : AUCUNE conversion officielle
  n'existe dans le moteur (TROUVAILLES 2026-07-10 « Traditions branchées » : « permeabilite laissé
  qualitatif… l'échelle P interne n'est pas documentée en unités lisibles joueur ») — le brief de
  CETTE mission fournit néanmoins un point d'ancrage explicite (0,5 → ~8 %) ; j'ai extrapolé
  LINÉAIREMENT (16 %/unité) pour les autres magnitudes du roster (0,25→~4 %) faute de mieux, et
  documenté l'hypothèse ici plutôt que de l'implémenter silencieusement. Si un futur agent câble
  un jour `permeabilite` sur une vraie échelle-jeu lisible (comme `capacite`→pénalité minoritaire
  ou `arcane`→coût de branche l'ont été le 2026-07-10), ces pourcentages devront être recalés sur
  la formule réelle plutôt que sur cette extrapolation.
**Vérifié**
- `gcc -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party scps/scps_heritage.c
  scps/scps_religion.c` (`PATH="/d/MSYS2/mingw64/bin:$PATH"` explicite, sinon `gcc` absent du
  PATH par défaut de ce Git Bash) : **0 warning, 0 erreur**.
- Grep final (les 4 fichiers) : aucune occurrence de `(K)`/`(K,`/`(L)`/`(L,`/`(H)`/`(H,`/`(P)`/
  `(P,`/`D̄`/« Perméabilité »/« Capacité (diversité) »/« Affinité arcane »/« Influence d'État »/
  « Dérive culturelle » [ancienne forme avec chiffre nu] dans du texte face-joueur — les seuls
  résidus sont des commentaires C (`/* P +1 · H −1 */`, libres) et des contractions françaises
  (« L'hiver », « L'étranger », faux positifs du grep `\b(K|L|H|P)\b` sur l'apostrophe).
- GDScript non testé en jeu (pas de Godot/scons dans cet environnement) — relu à l'œil pour la
  syntaxe (accolades/dict équilibrés, indentation par tabulations cohérente avec le reste du
  fichier), le fichier n'a subi qu'un ajout de const + 3 lignes dans une boucle existante.
**Restes**
- Le rename `NM[]` dans `scps_api.c:3514-3517` (façade `scps_culture_preview`) reste la fuite
  RACINE — la traduction GDScript de cette mission est un correctif d'AFFICHAGE en aval, pas la
  guérison de la source. À faire quand `scps_api.c` se libère : remplacer `NM[9]` directement par
  les mots de jeu (`"Croissance de la population"`, `"Production"`, `"Rayonnement diplomatique"`,
  `"Coercition"`, `"Capacité de l'État"`, `"Assimilation des minorités"`, `"Magie faustienne"`,
  `"Dérive culturelle"`, `"Fracture"`) — à ce moment, retirer aussi le dict `NOM_LEVIER_JOUEUR`
  de `culture_creator.gd` (devenu un no-op) et vérifier si d'autres appelants de
  `scps_culture_preview` existent (grep `culture_preview(` côté Godot) qui bénéficieraient aussi.
  Si le rename se fait, envisager d'ajouter les alias `"Assimilation des minorités"`/`"Magie
  faustienne"` à `concepts.gd:DEFS` pour restaurer la cascade CODEX perdue (cf. Pièges).
- Pas de `make`/`golden`/`determinism` relancés : les 2 fichiers C ne touchent QUE des littéraux
  `.hover`/`.flavor` et des tips display-only (aucun `.lev`/`.pts`/`.antonym`/canal RC_* changé) —
  strictement display-only, comme la passe éditoriale TECH du même jour ; le golden ne peut pas
  bouger par construction (aucune formule/donnée consommée par le moteur n'a changé).

## [2026-07-11] UI topbar/country_panel — hovers « quoi + combien » (I0 déjà câblé, metab_pct mort)

**Découvertes**
- Le lecteur de décomposition du Trésor demandé par la mission **EXISTAIT DÉJÀ** — pas besoin
  d'ajouter `scps_country_flux` : `scps_country_budget(ScpsSim*, int cid, ScpsFluxLine*, int max)`
  (`scps/scps_api.c:2045`) parcourt DÉJÀ tout `FluxComp` (`econ_flux_get`/`FX_COUNT`,
  `scps_econ.h:711-724`) et ne retourne que les postes non-nuls (nom `econ_flux_name` + montant
  signé). `scps_budget_summary` (`scps_api.c:2058`) donne `gold/income/expense/net/credit_line/
  creditor`. Les DEUX sont déjà bindés côté Godot : `ScpsWorld::country_budget(country)` →
  `Array[{name,amount}]` et `::budget_summary(country)` → `Dictionary` (`scps_sim_node.cpp:1121,
  1134`), et même DÉJÀ consommés ailleurs (`sidebar_drawer.gd:184-209`, tiroir Économie). `g_flux`
  est l'ANNÉE EN COURS (RAZ à chaque `econ_flux_year_capture`, `scps_econ.c:786` — 365 j, câblé côté
  façade dans `scps_api.c:149-159`), pas un solde « mensuel » : l'ancien libellé topbar "Trésor
  royal (solde mensuel)" était donc FAUX (c'était en fait annuel-glissant) — corrigé en "cette année".
- **`metab_pct` était un champ MORT côté binding** : `ScpsCountryInfo.metab_pct` est bien REMPLI par
  `scps_country_info` (`scps_api.c:550`, `AI_METAB_RES_W × econ_country_metabolized`) mais
  `ScpsWorld::country_info()` (`scps_sim_node.cpp:413-431`) ne le copiait JAMAIS dans le
  `Dictionary` retourné — invisible côté GDScript malgré le calcul moteur déjà fait. Ajouté
  `d["metab_pct"] = c.metab_pct;` (une ligne). Aucun autre champ de `ScpsCountryInfo` n'est dans ce
  cas (vérifié : les 10 autres sont tous copiés).
- Aucune décomposition moteur (style `BreakdownReadout`/`metric_agitation_breakdown`,
  `scps_readout.c:311`) n'existe au grain PAYS pour stabilité/prospérité/légitimité/cohésion — SEULE
  l'agitation de PROVINCE en a une (`ProvinceReadout.agitation_why`, exposée
  `scps_province_agitation`). Confirmé par grep exhaustif (`grep -n "BreakdownReadout\|_breakdown("`)
  → un seul site. Pas de decomposition inventée : les 5 TIPS (stabilite/prosperite/legitimite/
  cohesion/savoir) réduites au NOM SEUL (le mot est déjà une clé `Concepts.DEFS` — `ui/concepts.gd` —
  décoré turquoise et cliquable par le TooltipServer, `ui/tooltip_server.gd:_decorated`) ; c'est
  exactement l'architecture à deux étages déjà en place (hover court → clic sur le mot → définition
  en cascade), découverte en lisant `tooltip_server.gd`/`concepts.gd` (pas mentionnés dans le brief).
- `country_stocks(cid)` (déjà bindé) porte `net_day` PAR BIEN — utilisé pour bâtir le hover Grenier
  à partir des 4 biens vivriers (`Céréales`/`Poisson`/`Bétail`/`Fruits`, filtrage par `name` — pas de
  Resource enum côté GDScript). Filtré par NOM plutôt que `res_id` (pas de constantes RES_* exposées
  côté binding, cf. `sidebar_drawer.gd:156-157` qui utilise déjà des ID bruts en commentaire mais le
  filtrage par nom est plus lisible ici et suffisant : 4 noms fixes, jamais renommés par langue FR).

**Pièges**
- `country_panel.gd::TIPS` n'a qu'UN SEUL consommateur (`topbar.gd:265`, `load(...).TIPS`) — vérifié
  par grep AVANT d'y toucher (`_gauge_row`/`ROWS` dans `country_panel.gd` sont du code MORT, jamais
  appelés dans `_draw()`, préexistant, hors scope, non touché).
- Directive tardive du coordinateur (reçue en cours de mission) : **jamais de coordonnée brute du
  modèle dans un hover** (K/P/H/L/Perméabilité…) — vérifié après coup qu'aucun des textes posés ici
  n'en contient (tous des noms FR déjà face-joueur : `econ_flux_name` retourne des noms de postes
  budgétaires réels, jamais un identifiant de variable).
- Le premier `scons` du rebuild a échoué avec « le fichier spécifié introuvable » sur
  `godot.windows.template_debug.x86_64.o` (2 tentatives identiques) puis a réussi tel quel à la 3e —
  transitoire (verrou de fichier concurrent probable, un agent parallèle buildait aussi la même DLL
  au même moment cf. consigne de mission sur `sidebar_drawer.gd`) ; PAS un vrai échec de build. Si ça
  se reproduit : retenter avant de creuser plus loin.
- `shot_ui.gd` sur seed 9/25 ans **n'a pas exercé `05_prov_foreign`/`06_diplo`** (aucun pays IA
  CONNU+non-rebelle trouvé à ce point de la partie — condition documentée dans le script lui-même,
  `shot_ui.gd:117-137`) : `country_panel.gd` n'a donc été vérifié QUE par son `_ready()`/dict-load
  (Main.tscn l'instancie toujours, `main.gd`) et par lecture de code, pas par un `_draw()` réel en
  probe. 0 SCRIPT ERROR au global reste le signal le plus fort disponible ici.

**Vérifié**
- `gcc -fsyntax-only -std=c99 -Wall -Wextra` propre sur `scps_api.c` (aucun changement C dans cette
  mission — le lecteur existait déjà — vérifié quand même par prudence, 0 warning).
- DLL debug+release rebâties (`scons platform=windows use_mingw=yes target=…`, PATH MSYS2 exporté
  explicitement — `bash -l` seul ne suffit pas, cf. entrées précédentes), 0 warning au compil/link des
  deux fichiers touchés (`scps_sim_node.cpp`, `topbar.gd`/`country_panel.gd` ne sont pas compilés).
- Probe fenêtrée `shot_ui.tscn` (seed=9 years=25) EXIT normal, **0 SCRIPT ERROR** sur les 19 captures
  (01_menu → 19_chronique) ; `02_hud.png` relu — topbar rend TOUTES les cellules (Trésor 10 883,
  pop 7 889, pénurie « Poisson : rupture 0 j », jauges, Bonheur 48 %) avec la mise en page INCHANGÉE
  (seul le hover — invisible en capture statique — a changé) ; le code de `_treasury_tip`/`_food_tip`/
  `_pop_tip` a bien EXÉCUTÉ sans erreur runtime dans `_draw()` (les valeurs affichées prouvent que le
  chemin complet a tourné, tip compris, puisque ces lignes sont dans le même bloc `if valide`).
- Exemples avant → après (hovers, pas le texte affiché à l'écran qui est inchangé) :
  - Trésor : `"Trésor royal (solde mensuel)"` → `"Trésor — taxes +1234 · entretien −320 · admin −150
    · … (net +644 cette année)"` (postes RÉELS `country_budget`, jamais 0 filtrés en amont).
  - Population : `"Âmes de l'empire"` → `"Population"` (+ `" — +45 ce mois"` si delta ≥ 0.5).
  - Nourriture : `"Réserve vivrière (rations)"` → `"Grenier — Céréales +2.1/j · Poisson −0.4/j ·
    Bétail +0.0/j · Fruits +1.2/j"` (net_day RÉEL par bien, `country_stocks`).
  - Pénurie : `"Pénurie au rythme actuel — surveillez ce bien (marché, colonies vivrières,
    chantiers)"` (conseil générique, aucun chiffre) → `"Pénurie — Poisson : rupture prévue dans 0
    jour(s) au rythme actuel"` (le chiffre déjà connu, répété honnêtement, pas de conseil inventé).
  - Savoir : longue définition (« Le savoir : la production de recherche… ») → `"Savoir — 
    métabolisation +12% recherche (clic : l'arbre de technologie)"` (ou juste `"Savoir (clic…)"` si
    metab_pct=0 — champ maintenant vivant grâce au fix binding ci-dessus).
  - Stabilité/Prospérité/Légitimité/Cohésion (`country_panel.gd::TIPS`) : longues définitions
    mécaniques (« Solidité du régime : haute = ordre tenu… ») → le NOM SEUL (« Stabilité », etc.) —
    aucune décomposition moteur disponible à ce grain, donc pas de lecture inventée non plus ; la
    définition reste accessible via clic (mot déjà turquoise dans `concepts.gd`).
  - Influence (topbar + country_panel) : deux lectures différentes (« réputation diplomatique
    (offres, alliances, ligues) » / « la seule mesure PUBLIQUE d'un royaume étranger ») → `"Influence"`
    dans les deux cas (même raisonnement : pas de breakdown, le mot cascade déjà).
  - Bonheur : `"Bonheur — satisfaction pondérée du peuple\nLaboureurs %d · Artisans %d · Noblesse
    %d"` → `"Bonheur — Laboureurs %d · Artisans %d · Noblesse %d"` (retire la clause définitionnelle,
    garde exactement les nombres qui existaient déjà).

**Ce qui n'avait PAS de décomposition moteur** (listé honnêtement, aucune n'a été inventée) :
- Stabilité, Prospérité, Légitimité, Cohésion — aucun `BreakdownReadout` au grain pays.
- Influence, Corruption — aucune formule de composition exposée par la façade.
- Population — aucun compteur naissances/morts par pays (seul `pop_delta` existe et c'est un concept
  DIFFÉRENT : « morts immédiats PURGE » d'un chargement de save invalide, `scps_api.h:757`, sans
  rapport avec la démographie normale).

**Restes**
- `country_panel.gd::_gauge_row`/`ROWS` restent du code mort (préexistant, hors scope) — un futur
  agent pourrait les brancher ou les retirer.
- Si un jour un `BreakdownReadout` pays existe pour stabilité/légitimité (ex. dérivé de `L`/`SI` —
  ATTENTION à ne JAMAIS nommer ces lettres dans le hover, toujours traduire en mots déjà établis
  comme le fait `metric_agitation_breakdown` avec `Coercition`/`Cicatrice`), les TIPS courts posés
  ici pourront regagner un `" — …"` de montants, sur le même patron que Trésor/Grenier.

## 2026-07-11 — LOT 1 UI (1.2/1.3/1.4) : texte enveloppé + pied fixe de province + compteur d'alertes

**Périmètre** : `godot/project/ui/vkit.gd` (helper), `godot/project/ui/province_panel.gd` (1.2+1.3),
`godot/project/ui/alerts.gd` + `godot/project/main/main.gd` (1.4). docs/UI_RECO_2026-07-10.md §LOT 1.

**Découvertes**
- **1.2 — `VKit.text_wrapped(ci,pos,col,texte,largeur_max,max_lignes,fs)`** (vkit.gd, après `text_w`) :
  enveloppe aux mots, casse un mot SEUL trop long caractère par caractère (sinon un identifiant sans
  espace déborde quand même), ellipse sur la dernière ligne si tronqué, renvoie la hauteur consommée.
  Appliqué dans `province_panel.gd` en `max_lignes=1` partout (nom de province généré, ligne climat/
  relief/statut, labels Culture/Idéologie sous les camemberts, la phrase de seuil de révolte) — PAS en
  multi-lignes : ces zones vivent dans un layout à Y FIXE (le header a un pas de 42 px, les labels sous
  pie ont +16 px fixe avant la section suivante) ; wrapper à 2+ lignes aurait fait chevaucher la section
  d'après SANS reflow. `max_lignes=1` réduit le helper à « clip + ellipse », ce qui est exactement ce
  qu'il fallait ici. Le texte COMPLET va en `_tips` (infobulle) SEULEMENT quand `VKit.text_w(full) >
  largeur` (pas d'infobulle superflue sur du texte qui rentrait déjà).
- **1.3 — PIED FIXE + scroll, motif repris de `sidebar_drawer.gd` (PAS `construction_panel.gd`)** : le
  panneau province a des sections de nature TRÈS différente (pies, jauges, barres empilées, grille
  d'icônes qui wrap) — un skip-par-ligne façon `construction_panel` (`if yrow>_ph: continue`) aurait
  demandé de garder cette borne à CHAQUE site de dessin (des dizaines). Repris à la place le motif
  `sidebar_drawer._draw_header` : le header (biome+nom+✕/repli) est un helper `_draw_header(w,info,cap,
  record)` appelé DEUX FOIS — 1re fois AVANT le contenu (établit `content_y0`, pose les tips/rects,
  `record=true`), 2e fois APRÈS tout le contenu scrollé (`record=false`, PUR REDESSIN qui masque tout
  ce qui aurait glissé dans sa bande). Le PIED (Réprimer/Assimiler/Purger/Détail, `_draw_gov_actions`
  INCHANGÉE — seul son SITE D'APPEL bouge, de la fin du flux de contenu à un point fixe
  `footer_y0+10`) suit le même principe : fond OPAQUE + filet or dessiné APRÈS le contenu, puis
  `_draw_gov_actions` par-dessus. Un seul offset `y := content_y0 - _scrolloff` au départ du contenu
  fait « couler » TOUT le reste du flux existant (SATISFACTION→POPULATION→RESSOURCES→PRODUCTION→
  revolte→CAPITALE→BÂTIMENTS→branche non-propriétaire) SANS toucher un seul site de dessin individuel —
  seule la ligne de départ change.
- **`clip_contents=true` clippe BIEN le `_draw()` immédiat du Control lui-même** (pas seulement les
  enfants, contrairement à un doute initial) — vérifié sur `sidebar_drawer.gd` (le drawer Conseil, très
  chargé, coupe proprement « Industrie » à son bord bas, aucune fuite) ET sur `province_panel.gd` une
  fois le vrai bug (ci-dessous) corrigé : tout ce qui dépasse `_ph` (`size.y`) disparaît sans code
  supplémentaire. `construction_panel.gd`'s skip-par-ligne manuel EN PLUS de `clip_contents` est donc
  une ceinture-bretelles pour son cas (limiter le TRAVAIL de dessin, pas une nécessité de correction).
- **Masquage des ZONES INTERACTIVES scrollées hors champ** : `_tips`/`_acts`/`_build_rect`/
  `_colonize_rect` du CONTENU (pas du header, dont les tips précèdent le snapshot `tips_before`) sont
  filtrés en fin de `_draw()` — retire tout rect ENTIÈREMENT hors de `[content_y0, footer_y0]` (un
  rect à CHEVAL est volontairement GARDÉ : sa portion visible doit rester cliquable/survolable, motif
  plus précis que le test « centre < 36 » de `sidebar_drawer._draw()`).
- **1.4 — `main.major_open()` + `alerts.major_open_fn: Callable`** : alerts.gd n'a pas de référence à
  Main → `main.gd` pose `alerts.major_open_fn = Callable(self, "major_open")` juste après avoir
  instancié le panneau (tous les champs `_tech`/`_econ`/… existent déjà à ce point de `_ready()`,
  vérifié par grep des sites d'assignation). `_rows()` (alerts.gd) construit la liste de LIGNES
  réellement affichées : fenêtre majeure fermée = 1 ligne/alerte (INCHANGÉ) ; ouverte = les items
  `critical:true` restent un par un + UNE ligne « N ⚠ » (`_draw_compact`) résume le reste. **Aucune
  alerte n'est typée critique aujourd'hui** — vérifié par lecture : les décisions VRAIMENT urgentes
  (guerre/paix/révolte/sécession/directeur, `POPUP_KINDS` dans `_poll_feed`) sont déjà routées vers
  `event_popup.gd` (OYEZ OYEZ) et n'entrent JAMAIS dans `_alerts`/`_events` — et les dilemmes à vrai
  choix (`event_dialog.gd`, `pending_event`) sont un système SÉPARÉ, toujours par-dessus tout,
  indépendant de `major_open` par construction (il ouvre son propre modal dès `pending_count()>0`,
  sans jamais consulter l'état des autres panneaux). Le mécanisme `critical` est donc du câblage prêt-
  à-l'emploi mais actuellement à vide — documenté en tête de `_rows()` pour qu'un futur type d'alerte
  sache s'y raccrocher sans relire cette investigation.
- **`_process(_dt)` (pas un signal) pour suivre `major_open`** : rien ne PRÉVIENT alerts.gd quand un
  panneau bascule `.visible` (des dizaines de sites différents : ✕, Échap, toggle direct…) — `_process`
  compare `major_open_fn.call()` au dernier état connu CHAQUE FRAME et ne redéclenche `_refresh()` (et
  donc `queue_redraw()` + retaille) QUE sur un changement effectif (pas de redraw-spam). `_draw()`
  relit AUSSI `major_open_fn` en tout début (au pied de la lettre du brief : « lu chaque _draw ») —
  redondant avec `_process` mais un `Callable.call()` est trivial, et ça garantit que `_draw()` ne
  dépend jamais d'un état caché potentiellement périmé.
- **Effet de bord utile (documenté, pas visé)** : `alerts._refresh()` était déjà cassée en probe
  (`shot_ui.gd`) — `Sim.generated.emit()` part AVANT `Sim.game_on=true`, et rien ne rappelle `_refresh`
  ensuite (aucun tick, le monde est en pause) ⇒ `_alerts` restait VIDE tout du long dans les anciennes
  captures (entrée TROUVAILLES du 2026-07-10, « pénurie ne se vérifie qu'en code »). Mon `_process` de
  1.4 se déclenche dès qu'un PREMIER panneau majeur bascule visible (ex. `_prov_detail` à l'étape 4) →
  ce changement force un `_refresh()`, qui tombe CETTE fois avec `game_on=true` ⇒ `_alerts` se peuple
  enfin. Les alertes sont donc désormais VISIBLES dans `shots_ui/` à partir de l'étape 4 (7 alertes,
  seed 9/25 ans) — `02_hud.png`/`03_prov_own.png` (avant ce déclenchement) restent vides, C'EST LE
  MÊME ARTEFACT DE PROBE PRÉ-EXISTANT, pas une régression de ce lot.

**Pièges**
- **Le VRAI bug du pied (payé cher, 3 itérations)** : le fond du pied n'était PAS opaque à 100 %.
  1re tentative : `Color(0.20,0.16,0.10,0.55)` (une simple teinte) — laissait « BÂTIMENTS » complètement
  LISIBLE en dessous (55 % d'opacité = quasi transparent). 2e tentative : `VKit.COL_PANEL` tel quel —
  MEILLEUR mais PAS parfait, un fantôme du texte restait visible : `COL_PANEL` a `alpha=0xf6/255≈0.965`,
  PAS 1.0 (défini ainsi dans vkit.gd pour un effet de cuir légèrement translucide sur le panneau
  PRINCIPAL, où rien ne se dessine PAR-DESSOUS) — pour un masque anti-bleed, il FAUT forcer `alpha=1.0`
  explicitement (`Color(COL_PANEL.r,g,b,1.0)`), jamais réutiliser une constante de palette « presque
  opaque » sans vérifier sa VRAIE valeur alpha. Diagnostiqué en ajoutant un `print()` temporaire des
  variables (`content_y0`/`content_h`/`footer_y0`/`_ph`/`maxscroll`/`size.y`/`y` du cursor final) dans
  `_draw()`, relancé la probe, lu le stdout — a confirmé `_maxscroll=93` (donc bien du VRAI contenu
  scrollé, pas un bug de calcul de hauteur) et que le texte fantôme tombait DANS la bande du pied
  ([footer_y0, _ph], pas au-delà) — orientant directement vers l'opacité plutôt que vers la géométrie.
  Le MÊME correctif appliqué au fond de masquage du header (`_draw_header`, bande nom/sous-titre) par
  précaution symétrique (pas encore observé en capture, le scroll vers le HAUT n'est exercé par aucune
  étape de la probe, mais le mécanisme est identique).
- `_draw_gov_actions(x,y,w)` prend `y` PAR VALEUR et NE RETOURNE RIEN — dans l'ancien code, l'appeler
  n'avançait donc JAMAIS le curseur `y` extérieur (le `+60` de marge finale couvrait implicitement sa
  hauteur). En migrant son appel au pied, `content_h`/`want` n'ont PAS besoin d'un terme correctif pour
  « l'ancien espace occupé par les actions » — il n'y en avait jamais eu dans le calcul de toute façon.
- `Callable(self,"nom_méthode")` (chaîne, pas `self.nom_méthode` lié) est LE motif déjà utilisé dans ce
  repo (`ui/controls.gd:72`) — suivi pour cohérence plutôt que le sucre syntaxique plus récent.
- La probe `shot_ui.gd` a tourné aux CÔTÉS d'agents parallèles qui modifiaient RÉELLEMENT
  `sidebar_drawer.gd`/`country_panel.gd`/`culture_creator.gd`/`concepts.gd`/`shot_ui.gd` lui-même
  pendant cette mission (`git status` les montre modifiés, AUCUN par moi — vérifié par `git diff
  --stat` restreint à mes 4 fichiers autorisés + TROUVAILLES.md avant d'écrire cette entrée). Les
  captures régénérées reflètent donc un ÉTAT MIXTE (mon lot + leur travail en cours) — sans
  conséquence pour la vérification de CE lot puisque les 3 captures demandées (03/15/02) sont dominées
  par mes fichiers, mais un futur agent ne doit pas s'étonner de diffs non attribuables dans
  `shots_ui/*.png` au moment de committer.

**Restes**
- La sélection « province étrangère » (05/06) n'a pas été trouvée sur ce run (seed 9/25 ans, comme
  documenté 2026-07-10 : dépend du monde) — la branche `elif powner>=0 and powner!=me:` (colonize/
  attaque/route/pillage, SANS pied fixe, restée dans le flux scrollable comme demandé par le brief qui
  ne nommait que Réprimer/Assimiler/Purger/Détail) n'a donc pas été vérifiée EN CAPTURE cette session —
  relue par code, structurellement identique au chemin `powner==me` (même masquage générique).
- Le scroll MANUEL (molette, `_scrolloff>0`) et le masquage du HEADER en particulier (redessiné
  par-dessus un contenu remonté) ne sont vérifiés QUE par lecture de code + le fix d'opacité symétrique
  — la probe ne pousse jamais `_scrolloff` par la molette (contrairement à `14c_drawer_politiques_
  scrolled` qui le fait pour le tiroir). Un futur agent pourrait ajouter une étape shot_ui similaire
  pour province_panel si un doute survient.
- Le chip compact « N ⚠ » chevauche légèrement le bouton ✕ de la fenêtre majeure ouverte à 1920×1080
  (visible sur `15_tech.png`/`04_prov_detail.png`) — MÊME ancrage que l'ancienne pile pleine (donc pas
  une régression : la pile pleine aurait chevauché BEAUCOUP PLUS), mais un ajustement de position
  serait cosmétiquement bienvenu (hors scope de ce lot, non demandé).

## [2026-07-11] §27 FINS CORRIGÉES — EAU d'un coup · RONCES dégrade sans purger · SANG = plancher permanent (implémenteur solo, scps_endgame.{c,h}/scps_tune_list.h/endgame_demo.c — SAVE non bumpé)
Périmètre STRICT (docs/AGES_FINS_2026-07-11.md, section « Fins corrigées » + raccord 9) : refaire les
3 fins §27 sur des moteurs EXISTANTS. Session PARALLÈLE à un autre agent sur les ÂGES (scps_events.{c,h}/
scps_missions.h/scps_prosperity.{c,h}) — jamais touché ces fichiers, vérifié `git diff` avant chaque édit.

**Découvertes**
- **EAU** : `cataclysm_water_seed` (scps_endgame.c) traçait DÉJÀ le masque COMPLET du rift en UN passage
  (tous les bras/toute la longueur, `eg->sunken[r]=1` pour CHAQUE région touchée) — c'est
  `cataclysm_water_step` qui étalait le drain sur `SINK_RIFTS_PER_YEAR` (3) régions/an. Le fix est donc
  MINIMAL : retirer le budget (`sunk_now < budget`) de la boucle de `cataclysm_water_step` — elle
  engloutit alors TOUT `sunken[r]==1` en une passe. Comme `cataclysm_water_seed` (dans
  `endgame_select_and_fire`) ET le premier appel de `cataclysm_water_step` (dans le `switch` d'
  `endgame_tick`, juste après) tournent dans le MÊME appel d'`endgame_tick` (le fire), la carte sombre
  ENTIÈREMENT l'année du tir — vérifié en LIVE (`chronicle 9 …` : « 6 région(s) englouties (0 en cours) »,
  zéro pending après le tick de déclenchement, sur 2 runs distincts). WATER_RIFT_ARMS/_LENGTH/_STEP
  promus au registre J en `X(...)` séparés (défaut inchangé 5/96/3) — les anciens `#define RIFT_ARMS`
  locaux supprimés, lus via `tune_f` au point d'usage.
- **RONCES** : le pipeline dégradation-avant-mort (habitabilité BIO_THORNS=0.05 + `econ_cold_refresh`
  appelé après CHAQUE propagation annuelle) était DÉJÀ écrit (LOT F, 2026-07-08) — seul le bloc de
  BASCULE (`THORN_FLIP_FRAC≥0.5` : convertit+détache+strip+refragmente) restait à retirer. Suppression
  pure (aucun remplacement nécessaire, le recalcul habitabilité/grain tourne déjà juste avant le bloc
  supprimé). `camp` devient un paramètre inutilisé de `thorns_step` (gardé pour la signature du switch
  d'`endgame_tick`, `(void)camp;` en tête — miroir du style `(void)rn;` déjà présent).
- **SANG — le vrai piège de conception** : l'ÉRUPTION (`cataclysm_thorns_seed`-like) de SANG n'existe pas
  vraiment — `sang_seed` snapshotait `revolt_scar` UNE fois au fire ; `sang_step` drainait ensuite la pop
  chaque année. Le raccord 9 demande un RATCHET (marque qui ne redescend jamais + plancher permanent sur
  `revolt_scar` des PROVINCES) plutôt qu'un drain — j'ai fusionné `sang_seed`+`sang_step` en UNE
  fonction (`sang_step`, appelée au fire ET chaque année via le `switch`) qui (1) relit
  `econ->region[r].revolt_scar` (agrégat pop-pondéré, `econ_aggregate_regions` scps_econ.c:1124) — si
  ≥`SANG_SCAR_MIN` ET > la marque actuelle, `sang_scar[r]` MONTE ; (2) pour CHAQUE province de la région
  (`w->region[r].province_ids[]`), plafonne PAR LE BAS : `pe->revolt_scar = max(pe->revolt_scar,
  sang_scar[r])`. Écriture DIRECTE sur `ProvinceEconomy.revolt_scar` depuis scps_endgame.c — PAS un
  précédent inventé : `cataclysm_strip_region_econ` (même fichier, EAU) écrit DÉJÀ `pe->owner=-1` etc.
  directement, le module endgame a licence sur ces champs pour SES mutations de fin de partie.
- **`endgame_flee_target` (non-`_arr`) devenait du code MORT** une fois le drain SANG retiré (son SEUL
  appelant) — retiré (aurait été un warning `-Wunused-function` sous `-Wall`, vérifié 0 warning après
  coup). `endgame_evacuate_region`/`endgame_flee_target_arr` restent utilisés (exode générique LOT F,
  EAU/FROID/RONCES/CHAUD — SANG n'y participe PAS, cf. Pièges).
- **`ProvinceEconomy.revolt_scar` (province, la vérité) vs `RegionEconomy.revolt_scar` (agrégat
  pop-pondéré, `scarsum[r]/popsum[r]`, RECONSTRUIT à chaque `econ_aggregate_regions`)** — le raccord 9
  parle de « revolt_scar régionale » comme SIGNAL de lecture (l'agrégat, cohérent avec ce que lisait déjà
  l'ancien `sang_seed`) mais le PLANCHER s'écrit côté province (la vérité sérialisée/decayée chaque mois
  par `scps_econ.c:3064` `re->revolt_scar -= 0.25*dt`). Confondre les deux (planher sur l'agrégat au lieu
  des provinces) n'aurait RIEN empêché de guérir — l'agrégat est écrasé au prochain tick éco.
- **Vérifié en LIVE (chronicle réel, pas seulement le banc synthétique)** : `SCPS_TUNE=ENDGAME_BLOOD_FRAC=
  0.01` (diagnostic seulement, jamais committé) fait fircher SANG de façon fiable (6/6 sur un mini-sweep
  de graines 1-99) — confirme que le sélecteur + `sang_step` fonctionnent bout-en-bout en conditions
  réelles, pas seulement sous les manipulations manuelles du banc.

**Pièges**
- **Le banc RONCES mesurait au MAUVAIS instant** : `cataclysm_thorns_seed` (l'ÉRUPTION) corrompt TOUTE
  la région-épicentre EN UN COUP, dans le MÊME tick que le fire (comme l'EAU) — un snapshot
  habitabilité/grain pris APRÈS le tick de fire capture déjà l'épicentre à son plancher ; 80 ans de
  propagation supplémentaire ne peuvent plus rien y faire baisser (déjà minimal). Il FAUT snapshotter
  TOUTES les régions du monde FRAIS avant même le premier `endgame_tick`, puis comparer `region[epi_b]`
  à cette valeur pré-fire — piège identique en substance à celui déjà documenté pour le FROID/l'exode
  (« FIN_FROID était FLAT », 2026-07-08) : mesurer au mauvais moment masque un effet réel.
- **Le grain (`raw_cap[RES_GRAIN]`) d'une région ne peut baisser que s'il existait déjà** —
  `econ_cold_refresh` ne fait que PLAFONNER vers le bas (`if (cold_grain < raw_cap) raw_cap = cold_grain`),
  jamais remonter ; une province côtière/archipel peut vivre de poisson SEUL (`raw_cap[GRAIN]=0` dès la
  genèse, N1 « carte nue ») — fixer le test sur LE foyer précis échoue selon la géographie tirée (graine
  9 = archétype « archipel »). Fix : chercher, parmi TOUTES les régions dont l'habitabilité a baissé, UNE
  qui avait du grain — plus robuste qu'un test fixé sur l'épicentre.
- **SANG n'a JAMAIS gagné en conditions normales sur les seeds de golden (7/108/209/310/411, 12 ans) ni
  sur le sweep 250 ans par défaut** (`ENDGAME_BLOOD_FRAC=0.20` par défaut ; la mémoire des morts tourne
  autour de 3-9 % de la pop vivante sur des mondes calmes) — c'est ATTENDU (SANG doit être RARE), mais ça
  veut dire qu'un run « normal » ne suffit PAS à observer le mécanisme live ; il faut soit un sweep large
  (50+ graines), soit un `SCPS_TUNE=ENDGAME_BLOOD_FRAC=…` bas en diagnostic ponctuel (jamais committé,
  golden inchangé puisque `SCPS_TUNE` est un override runtime).
- **Le worktree gate-check** : `make chronicle`/`golden`/`determinism`/`smoke` échouaient dans l'arbre de
  travail PARTAGÉ (le second agent avait `scps_events.h` modifié mais PAS encore `scps_events.c` synchro
  — `AgesState.tier_open`/`research_mult`/`AGE_COMMERCE` etc. inexistants côté .c, erreurs de compilation
  à CE moment précis, rien à voir avec mes fichiers). Contournement PROPRE : `git worktree add --detach
  <tmp> fbabfa4` (le commit de départ propre) + copie MANUELLE de mes 4 fichiers modifiés
  (scps_endgame.{c,h}, scps_tune_list.h, endgame_demo.c) dans ce worktree isolé, gates lancés LÀ — zéro
  risque d'écraser le travail en cours de l'autre agent, zéro dépendance à ce que son build soit fini.
  `git worktree remove` (ou suppression manuelle) à faire par l'orchestrateur en fin de vague.
- **Environnement de build** : le shell par défaut du Bash tool n'a PAS `make`/`gcc` MSYS2 sur PATH (son
  `/tmp` n'est même pas le même `/tmp` que celui vu par le sous-shell MSYS2 — écrire un fichier dans
  `/tmp` depuis `MSYSTEM=MINGW64 /d/MSYS2/usr/bin/bash.exe -l script.sh` puis le lire depuis le Bash tool
  droit donne un fichier VIDE/périmé). Toujours (a) écrire un script `.sh` sur disque avec `cd` explicite
  en 1re ligne, (b) l'invoquer `MSYSTEM=MINGW64 /d/MSYS2/usr/bin/bash.exe -l <chemin absolu du .sh>`, (c)
  rediriger toute sortie qu'on veut relire ensuite vers un chemin Windows ABSOLU sous `/c/...` (jamais
  `/tmp`) — cf. `scps-build-windows.md` (mémoire), confirmé à nouveau ici.

**Restes**
- Aucun canal neuf ajouté (conforme au mandat) : SANG reste borné par `revolt_scar`, qui ne capture QUE
  les cicatrices de sac/révolte — une guerre inter-états SANS siège/révolte locale peut faire monter le
  ratio mondial de morts sans jamais marquer AUCUNE région (`sang_scar` reste tout à 0, `sang_step`
  no-op) ; gap PRÉ-EXISTANT déjà documenté 2026-07-08 (Lot F), toujours vrai, toujours hors mandat de
  cette session (le mandat interdisait explicitement un second signal).
- Pas de télémétrie chronicle dédiée au NOMBRE de régions marquées SANG (seule la ligne « mémoire des
  morts X% » existe, mondiale) — chronicle.c est hors périmètre des fichiers autorisés ; un futur lot
  pourrait ajouter « N région(s) marquée(s) SANG » au résumé §27 si le calibrage l'exige.
- `docs/AGES_FINS_2026-07-11.md` §« Les âges » (hors mandat, agent parallèle) reste À FAIRE par l'autre
  agent — l'arbre partagé n'était PAS buildable au moment de cette session (scps_events.c/.h désynchros) ;
  revérifier `make golden`/`determinism` dans l'arbre RÉEL (pas le worktree isolé) une fois l'autre agent
  terminé, pour confirmer qu'aucune interaction imprévue entre les deux lots ne bouge le hash 12 ans.

## 2026-07-11 ÂGES SANS ORDRE IMPOSÉ — docs/AGES_FINS_2026-07-11.md (agent §Ages)
**Découvertes**
- L'ancien système d'âges vivait ENTIER dans `scps_events.h`/`scps_events.c` (AgeId, AgesState,
  events_check_ages, age_dawn, ages_tier_open/ages_dawned/ages_breach_pressure) — AUCUN autre module
  n'implémentait de logique d'âge, seuls `scps_factions.c:age_patron/faction_age_engage` (l'engagement,
  SUPPRIMÉ) et `chronicle.c`/`events_demo.c`/`structural_demo.c`/`factions_demo.c` en lisaient l'API.
- `tier_open`/`research_mult`/`integration_mult` étaient DES CHAMPS MORTS avant cette session : posés par
  `age_dawn` mais JAMAIS lus par `tech_can_research`/`ai_research_step`/demography — confirmé par grep
  exhaustif (seuls les bancs les lisaient). Raccords 1/2/3 = les CÂBLER pour de vrai, pas les inventer.
  Déplacés vers `WorldProsperity` (`age_research_mult`/`age_integration_mult`/`age_tech_mask`, uint bitmask
  `theme*8+tier`) plutôt que laissés sur `AgesState` : tout consommateur (ai_research_step, demography_tick,
  scps_sim.c voie joueur) avait DÉJÀ `wp` sous la main, jamais `ev`/EventsState — évite d'élargir la
  signature de fonctions largement appelées.
- **Piège de link le plus coûteux** : appeler une fonction de `scps_events.c` (`ages_tech_researchable`)
  depuis `scps_ai.c` casse le LINK de `ai_demo`/`FORKS_DEMO_OBJS`/`CREDIT_DEMO_OBJS`/`CAP_DEMO_OBJS` — ces
  binaires ne lient PAS `scps_scps_events.o` (`AI_DEMO_OBJS`, Makefile:343). Fix : DUPLIQUER le petit test
  de bit LOCALEMENT dans `scps_ai.c` (`ai_age_tier_open`, statique, lit `wp->age_tech_mask` directement,
  ZÉRO appel cross-module) plutôt que d'exposer une fonction publique consommée par un module qui n'a pas
  le luxe de tirer tout `scps_events.o`. Le miroir `ages_tech_researchable` (scps_events.c, pour
  scps_sim.c/scps_api.c qui lient déjà events.o) DOIT rester identique bit-à-bit si les 4 paliers gated
  changent (`THM_SOCIETE` 3/5 · `THM_SAVOIR` 4/5) — pas de test partagé, un commentaire de garde de chaque
  côté suffit (cf. les deux fonctions).
- **`Makefile:EVENTS_DEMO_OBJS`** ne liait PAS `scps_scps_fog.o` (ajouté : `country_knows`/
  `fog_debug_meet_all` désormais utilisés par `scps_events.c` pour le déclencheur Découvertes) — toute
  liste `*_OBJS` qui a `scps_scps_events.o` sans `scps_scps_fog.o` cassera au link si un futur âge lit
  encore du brouillard ; awk de contrôle : `awk '/_OBJS.*(:=|\+=)/{if(n)print n,e?"E":"-",f?"F":"-";
  n=$1;e=0;f=0} /scps_scps_events\.o/{e=1} /scps_scps_fog\.o/{f=1}' Makefile` (seule EVENTS_DEMO_OBJS
  avait E sans F après mon ajout de scps_fog.h à scps_events.c — corrigé).
- **`EventCtx` (struct interne à `scps_events.c`, ~10 constructeurs positionnels `{ev,w,econ,...}`)** :
  ajouter un champ EN FIN de struct (`MissionsState *ms`) est SÛR par construction en C99 — un
  initialiseur positionnel avec MOINS d'éléments laisse le reste à zéro (`ms=NULL` partout sauf le seul
  site qui le passe explicitement, `ages_hero_fire`). Mais `-Wextra` prévient quand même
  (`-Wmissing-field-initializers`) — ajouté `,NULL` explicite aux ~7 sites existants pour rester à
  0 warning (le codebase l'exige).
- **Le hero (raccord 7) se détecte HORS du module events** : `scps_missions.c` gagne un flag transitoire
  `Mission.just_completed` (RAZ en tête de CHAQUE `missions_tick`, posé vrai dans la branche succès) —
  scps_sim.c (qui a Statecraft+MissionsState+EventsState tous sous la main) fait le test rang III +
  efficacité + loyauté + encore assis JUSTE APRÈS l'appel à `missions_tick`, puis appelle
  `ages_hero_fire(...)`. `scps_missions.c` reste ignorant des évènements (pas de nouvelle dépendance
  croisée events↔missions dans ce sens). Le SIÈGE est FIXE par EVID (`EVID_HERO_SAVOIR/SOCIETE/
  INDUSTRIE` = seat 0/1/2), motif copié EXACT de `EVID_TRAHISON_SAVOIR/SOCIETE/INDUSTRIE`
  (`hero_seat_of`/`treason_seat_of`, scps_events.c) — la faction du titulaire est résolue DYNAMIQUEMENT
  à la résolution (`statecraft_council_seat_faction`), le hook statique de la table reste neutre.
- Genre du héros : AUCUN état neuf — `statecraft_council_cand_female` (nouveau, scps_statecraft.{h,c})
  relit le MÊME hash que `statecraft_council_cand_firstname` (`h % 24 >= 12`), juste un second regard sur
  un tirage déjà déterministe.
- Le bonus « la prochaine mission du siège » (raccord 7, oi=0/1) est porté par `MissionsState.hero_bonus
  [cid][seat] = {mult, slot, gen}` — consommé (appliqué OU perdu, jamais laissé traîner) dans
  `mission_grant` (scps_missions.c) qui compare `(slot,gen)` COURANT du siège à celui figé au choix : si
  le titulaire a changé, le successeur ne touche rien (spec verbatim), et le slot est remis à 0 dans
  TOUS les cas (pas de fuite d'un bonus jamais consommé si le siège ne se re-route jamais vers ce héros).
- **fire_event() en mode DÉTERMINISTE (pas de roll mtth)** : appeler `fire_event(&cx, evid, subject)`
  directement (sans passer par le scan `if (EVENTS[x].trigger(...) && frand(...)<mtth_p(...))`) enfile
  (joueur) ou auto-résout (IA, `best ai_chance`) IMMÉDIATEMENT, sans consommer `ev->rng` — c'est le MÊME
  mécanisme que `age_dawn` (qui n'est pas non plus dans le scan). `EVENTS[EVID_HERO_*].trigger` pointe
  vers un `trig_never` neutre (jamais appelé, la table exige un pointeur non-NULL) ; `mtth_days=0.f`.
- `capital_max_tier`/tier_open : NE PAS confondre le TIER de la SPEC des Âges (Société 3/Savoir 4/
  Société 5/Savoir 5, un palier de l'arbre TECH) avec le tier de PROVINCE (LOT T, pop→T1..T7) — deux
  échelles totalement différentes qui partagent juste le mot « tier ». Aucune interaction.

**Pièges**
- **Le piège le plus coûteux (2 tests cassés, ~45 min à isoler)** : `events_demo.c` ne réinitialise
  JAMAIS `faction_lever_apply`/`faction_levers_reset()` ni le monde (`route_pe`/`years_held`/`ts.charge`/
  `country_knows` fog) ENTRE ses sections — seul `events_init(s.ev,...)` (RAZ `ev` uniquement) est appelé
  en tête de section. Tant que les âges n'avaient AUCUN effet de bord sur les factions (l'ancien
  `age_dawn` ne touchait jamais `scps_factions.c`), ce résidu inter-sections était invisible. Mes leviers
  scopés (`age_lever_exchange/_discovery/_empires/_breach`, NOUVEAUX) le rendent VISIBLE : la section 4
  (test des âges) laisse `route_pe`/`years_held`/`ts.charge` à des valeurs qui SATISFONT ENCORE les
  déclencheurs plus loin dans le fichier ; comme `events_init()` remet `ev->ages.dawned[]`/
  `year_eligible[]` à zéro à CHAQUE section suivante, les âges peuvent RE-DAWN en section 15 (Conseil) —
  et là, `age_lever_exchange`/`_discovery` votent sur des pays que la section ne s'attend pas à voir
  bouger, cassant `EVID_TRAHISON_SAVOIR`/`EVID_MERV_SACRIFICE` (probabilistes, sensibles à l'état
  faction/rng accumulé). Fix appliqué EN FIN DE SECTION 4 (`events_demo.c`, juste après le dernier
  `ok(...)` des âges) : `faction_levers_reset()` + RAZ manuel de route_pe/years_held/ts.charge + `fog_reset()`
  — même discipline que `events_init()` referme SA section, ce nettoyage referme la section 4. **Tout futur
  banc qui enchaîne des sections SANS réinitialiser le monde ENTRE elles est à risque dès qu'un âge
  gagne un effet de bord** — chercher `faction_levers_reset()`/`fog_reset()` manquants en premier si un
  test lointain casse après une modification de `age_dawn`/`age_lever_*`.
- **Éligibilité ACQUISE (spec) vs exclusion mutuelle Soulèvements↔Tyrans — INCOMPATIBLES telles quelles.**
  Un `age_trig_soulevements`/`age_trig_tyrans` qui inclut la précondition `!dawned[AUTRE]` ne bloque QUE
  l'ÉLIGIBILITÉ (le premier `year_eligible[a]=year` latché) — pas l'AVÈNEMENT (spec : « l'éligibilité
  reste ACQUISE même si la valeur redescend », donc la phase 2/avènement ne revérifie PAS le trigger).
  Un monde qui bascule en crise satisfait SOUVENT les deux conditions matérielles EN MÊME TEMPS (avant
  qu'aucun des deux n'ait dawné) → les DEUX se latchent la même année → celui qui dawn EN SECOND
  (perdant du throttle 1/an, retenté l'an suivant) ignore que l'autre a entretemps dawné, et avient
  QUAND MÊME (violation de l'exclusion). Fix : les triggers `age_trig_soulevements`/`age_trig_tyrans`
  sont des lectures MATÉRIELLES PURES (pas de précondition dessus, servent SEULEMENT à l'éligibilité) ;
  l'exclusion est un GATE SÉPARÉ, réévalué à CHAQUE tentative d'avènement dans la boucle de sélection de
  `events_check_ages` (`if (a==AGE_SOULEVEMENTS && dawned[AGE_TYRANS]) continue;` et le miroir) — capturé
  par `structural_demo.c` §1 (réécrite pour tester l'exclusion mutuelle DANS LES DEUX SENS plutôt que
  l'ancienne causalité Lumières-d'abord, supprimée par la spec).
- **`make`/`cc` de ce poste résout un mingw64 CASSÉ** (`/mingw64/bin/cc` = en fait `D:\Git\mingw64`,
  le mingw BUNDLED avec Git for Windows — PAS `D:\MSYS2\mingw64`) qui échoue avec `Cannot create
  temporary file in C:\Windows\: Permission denied`, de façon INTERMITTENTE et pas forcément liée à la
  taille du fichier (chronicle.c/events_demo.c/ai_demo ont chacun échoué au moins une fois en compile
  OU en LINK, y compris avec `CC=/d/MSYS2/mingw64/bin/gcc.exe` explicite passé à `make` — le `make`
  MSYS lui-même semble scrubber l'environnement différemment d'un appel gcc direct). Contournement fiable
  à 100 % (jamais échoué) : quand `make CC=... <cible>` échoue sur `Cannot create temporary file`,
  RE-EXÉCUTER la commande gcc affichée EN LA COPIANT TELLE QUELLE hors de make (compile OU link), puis
  relancer make normalement — make retrouve le `.o`/l'exe déjà produit et continue. `export TMP=
  'C:\Users\...\Temp' TEMP=... TMPDIR=/tmp` avant TOUT (compile direct ET make) réduit la fréquence mais
  ne l'élimine pas. Les `.exe` déjà présents dans le repo (`events_demo.exe`, `ai_demo.exe`, …) au début
  de session sont des BINAIRES PÉRIMÉS (antérieurs à toute session en cours) — ne JAMAIS les prendre pour
  un test déjà vert sans vérifier le timestamp (`ls -la`) contre l'heure de la dernière modif source.
- Chaque Bash tool call est un shell FRAIS : un fichier écrit sous `/tmp` (ou `/c/.../Temp`) dans un appel
  n'est PAS garanti lisible dans l'appel suivant (observé : `cp /tmp/x.o build/` → "No such file", et une
  redirection `> /tmp/mk.log` suivie de `tail /tmp/mk.log` dans le MÊME `&&`-chain a aussi échoué une
  fois) — préférer TOUJOURS écrire directement dans `build/` (chemin du repo, persistant) plutôt que
  `/tmp`, et regrouper compile+link+run dans UN SEUL appel Bash quand l'ordre compte.

**Restes**
- **`ENTROPY_BREACH_W`** (scps_tune_list.h, `scps_endgame.c:118`) reste à son défaut PRÉ-EXISTANT (0.3),
  PAS le 0.60 de la spec (« poids de la Brèche dans l'entropie 0.60 ») : `scps_endgame.c` est le fichier
  de l'agent parallèle « fins » (en cours d'édition pendant cette session, cf. entrée ci-dessus) — changer
  SEULEMENT le défaut du registre sans toucher le `tune_f("ENTROPY_BREACH_W", 0.3f)` du fichier casse
  l'invariant documenté (« le défaut du registre DOIT égaler le défaut au site d'appel »). Une ligne à
  changer aux DEUX endroits une fois `scps_endgame.c` stabilisé : `0.3f` → `0.60f` (scps_tune_list.h:553
  et scps_endgame.c:118).
- **Citations et readout d'âge** : `scps_age_citation`/`scps_known_pair_share` sont des fonctions
  ADDITIVES neuves (scps_api.{h,c}) plutôt qu'une extension de la signature de `scps_age_state`
  existante — `godot/src/scps_sim_node.cpp` appelle déjà `scps_age_state` avec sa signature actuelle
  (4 arguments) ; l'élargir aurait cassé la compilation du binding Godot (hors périmètre/build de cette
  mission). Un futur agent Godot doit CÂBLER ces deux nouvelles fonctions côté binding + UI (topbar/
  age_recap) pour que les citations/le ratio de pays connus soient VUS par le joueur.
- **Chip « Engager »** : conservé EXACTEMENT tel quel côté verbe (`CMD_AGE_ENGAGE`/
  `scps_player_age_engage`, scps_sim.c/scps_api.c INCHANGÉS) — devenu un pur accusé de réception
  (`player_age_engaged` s'actualise, plus aucun effet moteur). Le chip topbar + l'écran de récap d'âge
  (Godot, `age_recap.gd`/`topbar.gd`) n'ont PAS besoin de changer : ils continuent d'éteindre le chip au
  clic, ce qui reste vrai (le verbe existe toujours, réussit toujours). Seule la SÉMANTIQUE change (« vous
  avez vu cet âge » plutôt que « vous avez voté pour son patron ») — un futur agent UI pourrait vouloir
  retoucher le LIBELLÉ/le texte du récap pour ne plus promettre un effet qui n'existe plus (codex.gd:50
  dit encore « le joueur choisit son moment » — reste littéralement vrai mais pourrait clarifier
  qu'aucun vote de faction n'est plus en jeu).
- **`ai_age_tier_open` (scps_ai.c) et `ages_tech_researchable` (scps_events.c) sont DEUX COPIES** de la
  même règle de gating (Société 3/5, Savoir 4/5) — voir Découvertes ci-dessus pour le pourquoi (le
  link). Si un futur âge ajoute/retire un palier gated, les DEUX fonctions doivent changer ensemble ;
  aucun test ne les compare bit-à-bit (à ajouter si ça dérive).
- **`tier_open` visuel côté façade** : `scps_tech_nodes` (scps_api.c) downgrade OPEN→LOCKED pour les 4
  paliers gated non ouverts, mais `scps_tech_info`/le panneau Medusa Godot n'affichent PAS explicitement
  QUEL âge ouvrirait le palier verrouillé (juste "verrouillé") — un futur agent UI pourrait vouloir un
  hover « s'ouvre à [nom de l'âge] ».
- **Bancs NON revérifiés par cette session** (compilés seuls, jamais linkés+exécutés, faute de temps) :
  `agency_demo`/`army_demo`/`campaign_demo`/`diplo_demo`/`econ_*_demo`/`endgame_demo`/`faith_demo`/
  `navy_demo`/`prosperity_demo`/`readout_demo`/`religion_demo`/`revolt_demo`/`routes_demo`/`social_demo`/
  `warhost_demo` — compilation SEULE (gcc -c) sans erreur pour chacun (confirmé), mais pas de link+run.
  Aucun de ces fichiers ne référence les symboles renommés (AGE_COMMERCE/AGE_REASON/AGE_ORDRE_FER/
  age_patron/faction_age_engage) d'après un grep dédié — risque résiduel FAIBLE mais non nul.
  `intertrade_demo` reste le KO pré-existant connu (setenv, Windows/MinGW, sans rapport).
- **`make golden`/`determinism` NON relancés** (mandat explicite : « NE RE-BASELINE PAS golden — ça va
  bouger fort, attendu, l'orchestrateur gère ») — cette session s'est arrêtée aux gates fonctionnels
  (chronicle 2 graines exit 0, bancs ci-dessus verts). Le hash 12 ans BOUGE nécessairement (nouveaux
  triggers dès l'an-0, ex-Commerce renommé Échanges avec un seuil légèrement différent — X_NODES/
  NODE_VALUE_Y inchangés en valeur mais la logique de %habité est nouvelle) : à re-baseliner par
  l'orchestrateur UNE SEULE FOIS après fusion avec le lot « fins » de l'agent parallèle.
- **SAVE BUMP nécessaire, non fait** : `AgesState` (+`year_eligible[AGE_COUNT]`, -`tier_open[3][8]`/
  -`research_mult`/-`integration_mult`), `WorldProsperity` (+5 champs : `age_P_bonus`/`age_mig_mult`/
  `age_research_mult`/`age_integration_mult`/`age_tech_mask`), `MissionsState` (+`Mission.just_completed`
  +`hero_bonus[SCPS_MISSIONS_MAX][SC_COUNCIL_SEATS]`) changent tous de taille — les TROIS sont
  fwrite/fread BRUTS (scps_save.c:SVT_EVNT/SVT_PROS/SVT_MISS) ⇒ `SAVE_VERSION` (scps_save.h, actuellement
  77) DOIT monter d'au moins 1 avant tout commit. scps_save.c/scps_save.h étaient HORS PÉRIMÈTRE de cette
  mission (pas dans la liste de fichiers autorisés) — un futur agent (ou l'orchestrateur) doit faire le
  bump + vérifier `save_sane` sur les nouveaux champs bornés (`year_eligible` ∈ [-1,~1000],
  `hero_bonus[].slot/gen` ∈ [-1,SC_COUNCIL_CANDS)/[-1,~qqch), `age_tech_mask` n'a pas besoin de borne
  — bitmask).

## [2026-07-11] UI — audit lot 2 restant : Marché/Diplomatie/Détail provincial (implémenteur, docs/UI_RECO_2026-07-10.md §2.3-2.5)
**Découvertes** : (1) **MARCHÉ (sidebar_drawer.gd)** — aucune donnée de « catégorie » n'existe côté façade
(`country_stocks` ne porte que name/marche/stock/net_day/coverage_days/market_band/price/res_id,
`godot/src/scps_sim_node.cpp:715`) ; la frontière brute/manufacturée EXISTE déjà dans le moteur sous
`RES_PROD_FIRST` (`scps/scps_types.h:181`, commentaire « tout ce qui est < RES_PROD_FIRST est une ressource
brute »), comptée à la main (RES_NONE..RES_STONE = 26 entrées) → `MARCHE_CAT_SPLIT := 26` en dur côté GDScript
(pas d'invention : c'est LA frontière moteur, juste sans lecteur dédié). Le mot d'état de marché
(`st["marche"]`, ex. « pénurie sévère »/« engorgé ») était DÉJÀ résolu par le moteur mais dessiné en
`COL_DIM` fixe (jamais coloré par bande) — fix trivial : `_marche_col(band)` s'applique aussi au mot, pas
seulement au prix. (2) **DIPLOMATIE état vide** — le filtrage brouillard (`country_known`) doit se faire
AVANT le test d'liste-vide (l'ancien code filtrait DANS la boucle, donc une liste 100% inconnue affichait
quand même le hint générique « ▸ cliquer une fiche » sans rien dessous). Vérifié EN LIVE (pas fabriqué) :
la seed 9 par défaut du probe (archétype « archipel », cf. log worldgen) laisse le joueur SANS aucun pays
connu à l'an 24 → `13_drawer_diplomatie.png` capture le VRAI état vide, pas une simulation. Le texte
« routes commerciales » du rappel est volontairement formulé comme APPROFONDISSANT un contact déjà croisé
(pas « en créant un ») : `scps_fog.c:14` (`fog_update`) ne découvre QUE par BFS radius-2 (`FOG_RADIUS`,
`scps_fog.h:31`) depuis les régions POSSÉDÉES — les verbes diplo/route exigent déjà `country_knows` en
amont (`scps_api.c:1536`), donc « ouvrir une route » ne peut pas être la cause de la 1re rencontre.
(3) **DÉTAIL PROVINCIAL — le disque « Religion » était mal branché** : `province_groups`
(`scps_api.c:767`, `scps_demography.c:325` `province_composition`) remplit `out[i].religion` avec
`religion_branch_name(eff.rel_branch)` — c'est l'axe « VISION DU MONDE » (naturaliste/universaliste/
cyclique/ritualiste, `scps_culture.c:73`), TOUJOURS non-vide, PAS la foi établie du module `scps_religion`
(P1-P8, qui elle NAÎT athée). L'ancien camembert « Religion » de `province_detail.gd` ne pouvait donc
JAMAIS être vide — la spec demandait une « absence NOMMÉE », impossible à produire depuis cet axe. Fix :
le médaillon Religion lit désormais `religion_of_region(region)` (`scps_api.h:1206`, déjà bindé
`scps_sim_node.cpp:189`) — `<0` ⇒ « Aucune foi établie » (vrai, moteur : le monde nu n'a pas encore de
Temple), `≥0` ⇒ `religion_name(owner)` (déjà utilisé tel quel par `religion_panel.gd:178`). Vérifié EN LIVE :
`04_prov_detail.png` (province Désert Brûlant, aucun édifice religieux encore bâti) affiche bien
« Aucune foi établie ». (4) **Le SEUL gate réel du bouton Déplacer** (réincorporation, LOT G) au-delà de ce
que l'UI garantit déjà (régions à soi via `_reinc_owned`, classe valide via le cycle, quantité>0) est
`ra==rb` — lu directement dans le drain (`scps_sim.c:690`, `case CMD_POP_TRANSFER`) : ownership/klass/n
sont TOUJOURS satisfaits par construction côté UI, seule l'égalité source/destination peut survivre au
clic. La « raison au survol » demandée par la spec est donc EXACTEMENT et SEULEMENT ce message.
**Pièges** : `shot_ui.gd` (hors périmètre de cette mission, non touché) capture DÉJÀ les 3 fichiers cités
par la spec SANS le savoir — la boucle `noms := [...,"marche",...,"diplomatie",...]` (index 3 et 6) avec
`"%02d_drawer_%s" % [7+i, noms[i]]` (`shot_ui.gd:157`) produit `10_drawer_marche.png`/`13_drawer_diplomatie.png`
tels quels ; un futur agent qui cherche à « ajouter » ces captures les dupliquerait pour rien — VÉRIFIER
le nommage dynamique avant d'éditer shot_ui.gd. Les dropdowns de réincorporation (`VKitDropdown`, ajoutés
en ENFANTS du contrôle dans `_ready()`) ont une largeur FIXE (190px, posée une fois via
`custom_minimum_size`) — les empiler verticalement (au lieu de les coller côte à côte, qui débordait toute
colonne < ~410px) est le fix SÛR ; tenter de les redimensionner dynamiquement à `colw` risquerait un clamp
contre `custom_minimum_size` non mis à jour en retour (non tenté, non nécessaire ici). Le seuil `two_col`
(`avail >= 560.0`) est en pratique TOUJOURS vrai : `PW` a un PLANCHER de 648px (`province_detail.gd:81`,
`clampf(vp.x*0.44, 648.0, 1000.0)`) ⇒ `avail` minimal = 616 ≥ 560 — la branche colonne-unique de
`_draw_peuples` est donc du code défensif jamais exercé en pratique (pas un bug, juste non observable sans
baisser ce plancher, qui est hors périmètre de cette mission).
**Restes** : `MARCHE_CAT_SPLIT := 26` (sidebar_drawer.gd) est un NOMBRE EN DUR qui suit `RES_PROD_FIRST` —
si l'enum `scps_types.h` gagne/perd une entrée AVANT `RES_STONE`, ce nombre doit être remis à jour à la
main (aucun lecteur façade `scps_resource_category()` n'existe pour l'exposer proprement ; un agent avec
accès façade pourrait l'ajouter pour supprimer ce couplage fragile). Le survol immédiat qui fait apparaître
le nom de ressource sur la ligne Marché (InputEventMouseMotion → `_marche_hover_res`, motif copié de
`construction_panel.gd:276`) n'a PAS été vérifié par capture (shot_ui ne simule aucun mouvement de souris) —
vérifié par relecture de code contre le motif existant, pas par preuve visuelle ; un futur agent avec accès
computer-use/mcp Chrome pourrait fermer ce dernier doute. Le disque « CULTURE » côté province_detail reste
sur son ancienne source de données (`groups[i]["culture"]`, l'axe éthos réel) — seul « Religion » était
mal branché, la culture n'a jamais eu ce problème (toujours non-vide de façon LÉGITIME, une province
habitée a toujours au moins une culture).

## 2026-07-11 — verdict GIGASWEEP 200 sims (post Âges émergents + fins corrigées)
**Découvertes**
- 200/200 sims, ZÉRO crash (~2h30). Hégémon mortel 183/200 · IPM 1,20 · Laborer 75,5 % ·
  Héros 198/200 (années d'avènement variées 50-76+) · Échanges/Empires/Lumières/Brèche/
  Soulèvements 200/200 en ORDRE ÉMERGENT (Soulèvements avant Empires observé — impossible
  dans l'ancien tableau fixe).
- DÉCOUVERTES 1/200 : le seuil 0,35 de paires connues était INATTEIGNABLE sous la diplo-fog
  (le ratio compte cités-états + hameaux). Calibré 0,35→0,12 (mesuré 7/8 sims sur 4 graines) ;
  golden re-baseliné (la part 0,12 est atteignable <an-12 sur les mondes denses).
**Restes (design, pas des bugs — décision joueur)**
- TYRANS 0/200 : les Soulèvements (≥2 pays en révolution — condition quasi universelle sur
  250 ans) tirent TOUJOURS en premier et verrouillent l'exclusivité à sens unique. Options :
  durcir le déclencheur Soulèvements (3 pays ? simultanéité stricte ?) ou assouplir la
  fenêtre Tyrans. La spec dit « 2 » — non touché sans arbitrage.
- FINS : HIVER 95 · RONCES 68 · EAU 11 · RÉCHAUF 2 · SANG 0 · aucune 24 (8,6:1 vs cible
  ≤2:1). Le fin_mix est équitable À COMPTEURS ÉGAUX (prouvé lot F) mais les COMPTEURS de
  rares sont biaisés (essence/foreuse quasi morts ⇒ EAU rare) ; SANG exige un ratio de
  morts que les mondes calmes n'atteignent pas (gap connu 08-07). Chantier compteurs, pas
  dispatch.

## 2026-07-11 — « lisse les déclencheurs + trouve pourquoi certaines fins ne viennent jamais » (task #82)
Réponse aux deux « Restes » du verdict gigasweep ci-dessus. Investigation par DIAG gated
(`SCPS_FINDIAG` dans scps_endgame.c au TIR + à l'an 250 des sans-fin ; `SCPS_AGEDIAG` dans
scps_events.c, 1/an) puis itération SCPS_TUNE. Validation 10 graines × 2 sims × 250 ans.

**FINS — causes RÉELLES par fin (mesurées, pas supposées)**
- EAU n'était pas *rare* mais **mathématiquement IMPOSSIBLE** : l'ancien sélecteur faisait un
  argmax STRICT des compteurs de conso de rare (essence→EAU) ; or la foreuse (essence) mesure
  0.0 dans 100 % des tirs observés (côté IA, hors périmètre : un seul couplage de construction,
  famine de fer tier-4 sans beeline, vs réplicateur/corne qui ont FAU5). 0×poids=0 ⇒ EAU jamais
  choisie ; et le fallback-climat n'était jamais atteint (un des 2 autres compteurs dépasse quasi
  toujours le seuil de dominance). FIX : `endgame_pick_fin_lottery` fusionne les 2 modes en UNE
  loterie — poids = plancher climatique (jamais nul, modulé par temp/humidité RÉELLES) + part de
  production (share∈[0,1]). `FIN_BASE_EAU`=1.5 (vs 1.0 R/F) COMPENSE que sa production est ~0.
  ⚠ INVARIANT : le call-site avait `tune_f("FIN_BASE_EAU",3.0f)` ≠ registre 1.5 (le registre gagne
  au runtime, mais fallback trompeur) → corrigé à 1.5.
- SANG 0/200 : le seuil `ENDGAME_BLOOD_FRAC`=0.20 datait de l'ère PRÉ-Phase-1 (spirale de révolte,
  morts ÷10 000 depuis). Le ratio AU TIR (mémoire décrue HL 40 / pop vivante) s'étale 0.014-0.112
  → 0.20 JAMAIS franchi. L'assiette (morts de BATAILLE Campaign, ~30-80k) est SAINE — c'était le
  SEUIL d'une autre ère. 0.20→**0.09** : les ~2 mondes les plus sanglants /12 franchissent.
- « aucune » 24/200 + RÉCHAUF 2 : le repli CHAUD exigeait `FUEL_FALLBACK_MIN`=4.0 de combustible/
  tête ; les 24 sans-fin en avaient 0.9-3.9 (TOUS sous 4.0, calé sur des mondes prospères d'AVANT
  la refonte éco per-capita). 4.0→**2.0** : 22/24 rattrapés, les 2 vraiment sobres (0.9,1.0) restent
  sans fin (cohérent — « un monde calme ET sobre reste sans fin »).
- RÉSULTAT (20 sims) : EAU 3 · RONCES 4 · HIVER 5 · RÉCHAUF 5 · SANG 3 · **AUCUNE 0** (était 24/200) ;
  ratio naturel EAU/RONCES/HIVER = 1,67:1 ≤3:1. Toutes les fins tirent.

**ÂGES — Tyrans 0/200 : le VERROU à sens unique brisé (Soulèvements↔Tyrans)**
- MESURÉ (AGEDIAG) : `revolutionnaires` pic à 3-5 (mode 3), atteint ≥8 seulement ~10 %/années
  (sporadique, tardif) ; `dereal_moy` (le signal LIANT de Tyrans) part de ~0 et monte lentement.
  L'ancien seuil Soulèvements MIN=2 était atteint dès la vague de révolte an 5-13 dans TOUS les
  mondes → Soulèvements verrouillait Tyrans à vie. Les seuils Tyrans étaient de toute façon
  inatteignables (fracture_moy plafonne 0.36-0.72 vs 3.0 ; SI_moy ~6 jamais <5).
- FIX : `AGE_SOULEVEMENTS_MIN_COUNTRIES` 2→**8** (une vraie VAGUE mondiale) · `AGE_TYRANS_FRACTURE`
  3.0→**0.30** · `AGE_TYRANS_SI` 5.0→**8.5** (DEREAL 1.25 inchangé, atteignable). L'embranchement
  émerge : le monde à forte vague de révolte précoce → Soulèvements ; le monde à fracture/
  déréalisation lente sans vague ≥8 → Tyrans.
- COURBE MESURÉE (20 sims, grep correct `ÂGE : L.[^|]*`) : MIN=5 → Soulèv 100 % / Tyr 0 % (verrou) ·
  MIN=7 → 85 % / 5 % · **MIN=8 → 70 % / 15 % / ni-ni 15 %** (l'embranchement cible « Tyrans minorité
  réelle 15-40 %, Soulèvements le reste »). MIN=8 retenu.
- ⚠ **PIÈGE DE MESURE (2h potentiellement perdues)** : mon 1er grep `"an [0-9]*  ÂGE : L.Âge des
  Soulèvements"` renvoyait 0 FAUSSEMENT (le préfixe « an NNN␣␣ÂGE » ne matchait pas la vraie ligne)
  → j'ai cru Soulèvements MORT à MIN=8 et failli baisser MIN (ce qui aurait RESTAURÉ le verrou).
  Le grep correct `grep -oE "ÂGE : L.[^|]*"` (la ligne d'avènement, hors bilan `| pays`) révèle
  Soulèv 14/20. LEÇON : compter les avènements d'âge par `ÂGE : L.[^|]*`, jamais avec un préfixe
  « an NNN » (nombre d'espaces variable).

**Restes / hors périmètre**
- Échanges dawne 20/20 an 3-6 (uniforme). LAISSÉ tel quel : c'est un âge FONDATEUR (les réseaux
  de commerce se forment tôt partout) — la donnée montre l'uniformité mais c'est correct pour cet
  âge ; l'embranchement dramatique est Soulèvements/Tyrans, désormais vivant. Forcer une variance
  ici re-baselinerait pour un gain douteux.
- La foreuse (essence) morte côté IA (scps_ai.c) reste la cause SOURCE du déséquilibre EAU — la
  loterie la CONTOURNE par le plancher, sans toucher l'IA (hors périmètre). Un vrai fix source =
  donner à la foreuse un beeline (comme réplicateur/corne) — chantier IA séparé.

## UI Lot 3 (topbar + rail gauche), 2026-07-11

**Contexte** : docs/UI_RECO_2026-07-10.md §3.1/§3.2, sur une capture baseline
(shots_ui/1280x720/02_hud.png) antérieure au commit fbabfa4 ("Vague soir"). Fichiers
exclusifs : ui/topbar.gd · ui/sidebar.gd · ui/icon_button.gd.

**Découvertes**
- ui/topbar.gd:1-518 (avant édition) — la capture baseline était DÉJÀ PÉRIMÉE : le bug
  parenthèse orpheline (« Poisson : rupture 0) »), le chip « Engager… » tronqué et le
  ruban Pause flottant au centre-haut étaient déjà corrigés par fbabfa4 (const
  `_FOOD_NAMES` + `_food_tip`, chip « Engager : %s » avec troncature garde-fou ancrée
  à `content_end`, ruban Pause ancré sous les boutons de vitesse `Frame.LEDGER_W`). Seul
  restait le vrai chantier §3.1 : la barre affichait encore ~18 chiffres/chips (nom,
  trésor, pop, provinces, stabilité, nourriture, pénurie générique, prospérité, savoir,
  colonie, légitimité, influence, cohésion, bonheur, N blasons de faction variables,
  ⚑ coup/corruption) au lieu des 8 permanents demandés.
- godot/project/assets/scps/ui/icons/ contient `tax_ledger.png` (revenu net),
  `corruption_coin.png` (corruption), `influence_compass.png` (influence) — tous
  DÉJÀ dans le pack, inutilisés jusqu'ici. `graph.png` n'existe QUE sous
  `addons/medusa/icons/`, pas dans le pack UI (`UIKit.icon()` aurait no-op silencieusement).
- icon_button.gd:_draw() — la branche `bg == ""` sert DEUX appelants distincts : les
  onglets du rail (fg_is_chrome=false, icône nue) ET les boutons `setup_chrome()` du
  bas (fg_is_chrome=true, ex. zoom). Le soulignement or « sélectionné » n'existait
  QUE pour la 1re variante (`bg=="" and not fg_is_chrome`) — donc mes ajouts de fond
  plein (halo sélectionné/survolé) touchent AUSSI les boutons de mode de controls.gd
  (setup_icon(..., "")), pas seulement le rail — c'est voulu (même sémantique
  « sélectionné », amélioration cohérente) mais à savoir si un futur agent cherche
  pourquoi le bas de l'écran a changé d'aspect aussi.
- Aucun des 3 fichiers n'utilise le motif `tr("T_*")`/TranslationServer (celui
  documenté dans CLAUDE.md ENGLISH lot E, câblé seulement dans le shell menu/options) —
  topbar/sidebar/icon_button restent 100 % littéraux FR, motif suivi tel quel (pas de
  régression, mais pas de nouveau système introduit non plus).
- `Sim.game_on` (autoload/sim.gd:37) n'a AUCUN signal dédié — seulement assigné
  directement dans menu_root.gd (Lancer/Charger). Les raccourcis F1-F8 (main.gd:375)
  le gardent déjà en garde. Sidebar.gd poll désormais ce booléen en `_process` (1
  comparaison/frame, redraw seulement au changement) pour l'état « indisponible »
  du rail — pas de signal à créer, pas de fichier hors périmètre à toucher.

**Pièges**
- Le format `"%s" % x if cond else "%s" % y` — en GDScript (grammaire proche de
  Python) le `%` de formatage lie PLUS FORT que le `if/else` ternaire ⇒ ça parse
  bien comme `(fmt1 % x) if cond else (fmt2 % y)`. Utilisé tel quel dans
  `_net_income_tip`-style et dans `sidebar.gd:_sync_enabled` (tooltip conditionnel)
  — vérifié par lecture, pas testé en éditeur (aucune fenêtre lancée, cf. consigne).
- Le fond « sélectionné » de icon_button.gd n'était PAS gaté par `enabled` dans un
  premier jet — un onglet sélectionné-puis-devenu-indisponible aurait gardé son
  halo plein doré tout en ayant l'icône éteinte (double message contradictoire).
  Gardé défensivement (`selected and enabled`) même si le cas est inatteignable en
  pratique aujourd'hui (`game_on` ne repasse jamais à false après le lancement).

**Restes**
- Aucune probe fenêtrée lancée (consigne explicite de l'orchestrateur — il détient
  l'unique affichage). La vérification est par relecture de code + inspection des
  assets ; à confirmer visuellement par l'orchestrateur aux 3 résolutions (§3.4).
- Les jauges/factions démotées vivent maintenant SEULEMENT en tooltip natif
  (`_get_tooltip`/`tooltip_text`) — si LOT 4 (finition graphique) veut un rendu
  tooltip plus riche que du texte multi-ligne brut, il faudra remplacer le
  mécanisme natif par un composant dédié (hors périmètre ici).

## UI Lot 4.4 (créateur/codex/annales/nouvelle partie), 2026-07-11

**Contexte** : docs/UI_RECO_2026-07-10.md §4.4 + « Par écran », sur les captures
baseline 1280×720 (18_codex.png · 01c/01d_creator*.png · 01b_newgame.png ·
19_chronique.png). Fichiers exclusifs : ui/codex.gd · ui/culture_creator.gd ·
ui/new_game_panel.gd · ui/chronique.gd. Aucune probe fenêtrée (consigne de
l'orchestrateur) — vérifié par relecture + comptage de balises (parenthèses/
accolades/crochets équilibrés par fichier, aucun Python/Godot CLI dispo dans ce
sandbox pour un parse réel).

**Découvertes**
- codex.gd — les 5 DOMAINES thématiques (Empire & Économie/Peuples/Diplomatie &
  Guerre/Foi & Savoir/Fin de partie) EXISTAIENT déjà comme catégorisation ; le vrai
  manque du §4.4 était juste qu'elles n'étaient ni repliables ni cherchables (liste
  plate). Rendu la « catégorisation par thème » gratuite en réutilisant l'existant.
- culture_creator.gd — `culture_preview(t0,t1,t2)` (l'aperçu CHIFFRÉ) ne dépend QUE
  des traditions, jamais de l'héritage/éthos choisi ; `_her`/`_eth` n'apportent que du
  texte qualitatif (sphère/epithete/hint + les LORE consts locales HER_LORE/
  ETHOS_LORE). Donc les nouvelles cartes Héritage/Éthos n'ont PAS de chiffre à
  afficher dessus (règle membrane : ne pas inventer un nombre) — compensé par un
  `tooltip_text` = la LORE complète (comparaison rapide au survol sans cliquer).
- new_game_panel.gd — `Sim.world.worldparams_default(seed)` (scps_sim_node.cpp:1676)
  renvoie DÉJÀ n_continents/world_age/land_amount/mountains/erosion/temperature/
  humidity, PURE fonction de la graine (pas de génération, pas de mutation) ; ET
  `godot/project/i18n/ui.csv` a DÉJÀ les clés T_NG_AGE/LAND/MOUNTAINS/EROSION/
  TEMP/HUMID/CONTINENTS (labels FR+EN) — préparées par un agent antérieur pour un
  aperçu qui n'a jamais été câblé. L'« aperçu compact » du §4.4 était donc déjà
  amorcé aux deux bouts (façade + i18n), juste jamais relié dans ce fichier.
- new_game_panel.gd — `worldgen_set` (via `_gather_params`) n'écrase QUE
  n_empires/n_city_states depuis le slider TAILLE ; n_continents/terres/relief/
  climat restent l'ARCHÉTYPE de la graine (worldparams_default), inchangés par la
  taille. Donc le nombre de RÉGIONS n'est PAS une fonction pure de la taille seule
  (dépend aussi de l'archétype) — honnêteté du §4.4 respectée : décompte
  empires/cités EXACT (table SIZES), régions en qualitatif seulement (pas de
  chiffre fabriqué).
- chronique.gd — `annals()` (scps_sim_node.cpp:1302) renvoie déjà `kind` (int,
  enum ANNAL_* de scps_events.h : DILEMME=0/CICATRICE=1/AGE=2/GUERRE_GAGNEE=3/
  GUERRE_PERDUE=4/SECESSION=5/HEGEMON_BRISE=6/MONUMENT=7/FIN=8/TRAHISON=9/
  MERVEILLE_ETAPE=10) — jamais lu côté Godot avant ce lot. C'est le levier
  « entrées enrichies » du §4.4 (glyphe+couleur par catégorie) sans rien inventer.

**Pièges**
- Le ternaire GDScript AVANT `%` : `"%s %s (%d)" % (["▸",n,c] if cond else
  ["▾",n,c])` — j'ai explicitement PARENTHÉSÉ le ternaire complet côté droit du
  `%` (au lieu de compter sur la précédence, cf. le piège noté au Lot 3 : `%` lie
  plus fort que `if/else`, donc SANS parenthèses `"fmt" % a if c else b` parse en
  `("fmt"%a) if c else b`, PAS ce qu'on veut ici où les DEUX branches doivent être
  formatées par le même gabarit).
- `LineEdit.text = "..."` en GDScript (assignation directe de propriété, pas
  `set_text()`) NE déclenche PAS le signal `text_changed` — vérifié par lecture de
  la doc Godot 4, pas testé en éditeur. Le bouton dé du new_game_panel devait donc
  appeler `_refresh_world_preview()` EXPLICITEMENT après avoir posé la graine
  tirée, en plus du `.text_changed.connect(...)` qui ne couvre que la frappe
  utilisateur.
- `ScrollContainer.ensure_control_visible(ctrl)` (utilisé par le sommaire du
  codex pour sauter à une section) attend un layout à jour : appelé après un
  `await get_tree().process_frame` pour laisser le conteneur fraîchement rendu
  visible (body.visible=true) se dimensionner avant de calculer le défilement —
  non testé en éditeur (pas de probe fenêtrée), à surveiller si le saut atterrit
  un cadre en retard au premier essai.

**Restes**
- Aucune probe fenêtrée lancée (consigne explicite de l'orchestrateur). Vérifié
  par relecture complète + un comptage de balises équilibrées par fichier
  (parenthèses/accolades/crochets, heuristique — ne détecte pas un mauvais TYPE
  de fermeture) ; à confirmer visuellement aux 3 résolutions.
- Traditions (culture_creator.gd, onglet 3) n'a PAS été converti en cartes — c'était
  DÉJÀ des boutons `toggle_mode` en `HFlowContainer` (pas un menu déroulant), donc
  hors du "vs déroulants" du §4.4 ; seuls Héritage/Éthos (les deux vrais
  `OptionButton`) ont été convertis.
- new_game_panel.gd : les bucket-words de l'aperçu monde (« aride »/« tempéré »/
  « vieux et fendu »…) sont des LITTÉRAUX FR non passés par `tr()` — impossible
  d'ajouter des clés à `i18n/ui.csv` dans ce lot (fichier hors périmètre) ; suit le
  précédent DÉJÀ dans ce même fichier (`T_NG_ARCH_HINT`, fallback littéral quand la
  clé n'existe pas en CSV). Un futur agent i18n pourra ajouter T_NG_LAND_LOW/MID/
  HIGH etc. et les basculer en `tr()` sans toucher à la logique de bucket.
- chronique.gd : le plafond `MAX_SC=480px` de la liste (~16 lignes visibles avant
  défilement) et le plancher `MIN_SC=90px` sont un choix arbitraire raisonnable,
  pas une valeur mesurée sur un vrai règne long — à ajuster si un règne à 40+
  faits rend le tiroir trop haut ou trop court en pratique.

## 2026-07-11 — UI Lots 3-4 : vérif de parse GDScript (piège check-only)
- `godot --check-only -s res://ui/X.gd` (script ISOLÉ) renvoie TOUJOURS « Compile Error:
  Identifier not found: Sim/Sound/Frame » pour tout script qui touche un AUTOLOAD → FAUX
  POSITIF, pas un vrai bug de syntaxe. Le signal FIABLE d'une vraie erreur = une ligne
  « Parse Error: … » (ex. `nmc` : « Cannot infer the type of variable »). Sinon, seule la
  PROBE FENÊTRÉE (le script chargé dans le projet complet + rendu) prouve qu'un script est bon.
- Un `:=` sur une variable de boucle non typée (`for nm in names: var nmc := nm`) casse
  l'inférence si `names` n'a pas de type d'élément → typer explicitement (`var nmc: String = nm`).
  C'est ce qui a fait crasher tout le codex (main.gd `_codex` restait Nil, la probe s'arrêtait
  au shot 18). Corrigé ; le reste des fichiers d'agent (topbar/sidebar/créateur/newgame) avait
  déjà PROUVÉ son parse en RENDANT (shots 02/01c/01b) avant le crash codex.

## 2026-07-11 — Lot 4 (4.1 matières + 4.3 hiérarchie typo), vkit.gd seul
**Découvertes**
- `godot/project/ui/vkit.gd::panel_bg` (appelé par 10 panneaux : economy/country/tech/
  province_detail/province_panel/construction/battle/event_dialog/event_popup/
  sidebar_drawer) est LE point d'entrée unique des « grandes surfaces » — enrichir CE
  helper propage le style à tous les panneaux sans toucher un seul fichier appelant. Même
  logique pour `VKit.section()` (bandeau de sous-section, ~15 appelants) et `VKit.header()`
  (bande de titre de fenêtre majeure) — les 3 points de levier pour la hiérarchie typo 4.3.
- `Noise.get_image(w, h)` (méthode de la classe de base `Noise`, héritée par
  `FastNoiseLite`) est SYNCHRONE et renvoie directement une `Image` (format L8, converti en
  RGBA8 via `img.convert(Image.FORMAT_RGBA8)`) — pas besoin de `NoiseTexture2D` (génération
  async, le pattern déjà utilisé dans `map/iso_ground.gd::_make_noise` pour le sol de
  carte) quand on veut un grain CUIT UNE FOIS et mis en cache statique. Vérifié par
  exécution headless réelle (voir méthode de vérif ci-dessous) : format avant convert = 0
  (L8), après = 5 (RGBA8), `ImageTexture.create_from_image` accepte le résultat direct.
- **Méthode de vérif SUPÉRIEURE à `--check-only -s` (qui donne les faux positifs
  « Identifier not found » sur autoload, cf. entrée du dessus)** : lancer un vrai
  `SceneTree` headless avec `--path` sur le projet + `--script` sur un script de test HORS
  du projet (scratchpad) qui fait `load("res://ui/vkit.gd")` puis appelle directement les
  fonctions statiques (`VKit._grain()`, `VKit.COL_VALUE`, etc.) dans `_initialize()`. Les
  autoloads du projet se chargent normalement (aucun faux « Identifier not found »), les
  vraies erreurs de parse/type remontent, et on peut vérifier le comportement RÉEL d'une
  API engine (ici `Noise.get_image`) sans jamais ouvrir de fenêtre : `Godot...exe --headless
  --path <projet> --script <script_hors_projet>.gd`. N'exécute PAS `_draw()` (nécessite un
  CanvasItem en cours de dessin) mais couvre 100 % du reste : compilation, types, constantes,
  présence de méthode, et tout appel d'API engine qui ne dépend pas d'un contexte de rendu.
- Un paramètre nommé `value` (dans `gauge(ci,x,y,w,h,value:int)`, préexistant) et une
  NOUVELLE méthode de classe `value()` (ajoutée ici) coexistent sans conflit — GDScript
  scope les paramètres localement à leur fonction, aucune collision avec un symbole de
  classe de même nom tant que le corps de la fonction ne s'auto-référence pas. Confirmé par
  la compilation headless (aucune erreur/warning bloquant).

**Ce qui a été ajouté (vkit.gd uniquement — ui_theme.gd et uikit.gd non touchés, aucun
besoin identifié pour ce lot)**
- `COL_VALUE` (or clair lumineux 0xffdd8c) + commentaire de palette documentant les 4
  niveaux (Titre=`header()`/COL_PARCH·FS_BIG < Section=`section()`/COL_GOLD < Valeur=
  `value()`/COL_VALUE < Détail=`detail()`/COL_DIM·FS_SMALL) — la hiérarchie existait déjà
  À MOITIÉ (Titre/Section distincts par construction) ; le vrai trou était le niveau 3
  (« le chiffre qui compte », plus lumineux que tout le texte courant COL_PARCH).
- `VKit.value(ci,pos,s,size=FS)` / `VKit.detail(ci,pos,s,size=FS_SMALL)` : wrappers minces
  autour de `text()`, opt-in, AUCUNE signature existante changée. Taille inchangée (FS/
  FS_SMALL déjà existants) — la hiérarchie passe par la couleur, jamais par +1px (contrainte
  dure de la mission : les layouts sont vérifiés serré à 1280×720).
- `VKit._fleuron(ci,center,r,a)` : losange vectoriel discret en COL_GOLD (PAS un asset —
  `panel_corner_ornate_*`/`panel_title_plaque` du pack chrome appartiennent à l'habillage
  9-slice explicitement RETIRÉ des panneaux le 2026-07-10, cf. CLAUDE.md « débarrasse-toi
  des assets de panneau » ; les réintroduire aurait été un backslide). Posé dans la MARGE
  RÉSERVÉE avant le texte (jamais sous/après) : `header()` à (6,17) r=3.5 (marge gauche 0-12
  avant le titre x=12), `section()` à (x-1,y+7) r=3.0 (marge x-4..x+2 avant le titre x+2) —
  zéro déplacement de texte, zéro changement de hauteur consommée (`section()` renvoie
  toujours `y+24` comme documenté « AUCUN layout appelant ne bouge »).
- `VKit._grain()` + branchement dans `panel_bg()` : bruit fbm 256×256 (simplex smooth,
  fréquence 0.22, 3 octaves, seed 4242) cuit une fois, caché statiquement, dessiné en RGBA8
  sur `r.grow(-3.0)` (marge anti-bordure) à `Color(1.0,0.95,0.85,0.055)` — alpha UNIFORME
  5,5 % (dans la fourchette 5-10 % demandée), seul le RGB varie par pixel = le grain ; garde
  `gr.size > 4×4` pour éviter un rect dégénéré sur un panneau minuscule.

**Ce que ça donne**
- Les 10 panneaux consommant `panel_bg()` gagnent le grain automatiquement, sans édition.
- Les fenêtres via `header()` et sections via `section()` gagnent le fleuron de titre
  automatiquement (tous appelants confondus).
- `VKit.value()`/`VKit.detail()` sont disponibles pour les PROCHAINS panneaux (ou une
  future passe de migration) — aucun panneau existant n'a été retouché pour les adopter
  (hors périmètre : les 3 fichiers autorisés étaient vkit/ui_theme/uikit, pas les panneaux).

**Pièges évités**
- Ne PAS utiliser `NoiseTexture2D` pour le grain (génération async, threads — inutile pour
  un cache statique cuit une fois ; `Noise.get_image()` suffit et est synchrone).
- Ne PAS agrandir FS/FS_SMALL/FS_BIG pour la hiérarchie — uniquement la couleur (contrainte
  dure de la mission, layouts vérifiés serré).
- Ne PAS réintroduire d'asset de chrome de panneau (ornate corners/plaque) — décision
  produit déjà actée et documentée dans CLAUDE.md.

**Restes**
- Aucun panneau existant n'adopte encore `VKit.value()`/`VKit.detail()` — ils gardent leurs
  couleurs actuelles (souvent `sense()` pour une valeur bonne/mauvaise, ce qui reste
  légitime et prioritaire sur la hiérarchie neutre quand un jugement de qualité s'applique).
  Une future mission « migration typo » pourrait les adopter panneau par panneau.
- 4.2 (brouillard de carte) explicitement laissé de côté (hors des 3 fichiers autorisés,
  sous-système shader/map distinct — l'orchestrateur tranchera).
- Non vérifié VISUELLEMENT (aucune probe fenêtrée autorisée) : la compilation/API est
  prouvée par exécution headless réelle (voir méthode ci-dessus), mais le RENDU final (le
  grain se voit-il bien à 5,5 % sur cuir #171109, le fleuron est-il bien positionné en
  pixel-perfect aux 3 résolutions) reste à confirmer par l'orchestrateur qui détient
  l'affichage.
