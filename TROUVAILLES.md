# TROUVAILLES — index de handoff (compressé 2026-07-14, 4413→360 lignes)

> **Format** : ce qui a COÛTÉ cher à trouver (pas ce qui a été écrit). Le code et git racontent le reste.
> Pièges dedupliés, découvertes structurelles (fichier:symbole), restes ouverts. L'historique complet vit dans git.

---

## ÉCONOMIE & PROVINCES

**Découvertes clés** :
- Stock national (P1) : pool par empire, prix soldé UNE FOIS/empire, re-projeté régions (scps_econ.c:econ_tick) — jamais direct région[] sauf via `econ_region_stock_add`.
- Matière réelle (v61) : accumulateurs inter-ticks SÉRIALISÉS (EMOB v57 : friche/lowsat ; COLC v61 : répit colonisation ; TXYR v65 : g_flux annuel courant).
- Accumulation : tout dépôt via `econ_region_stock_add(econ,region,good,delta)` (scps_econ.h:502) met à jour PROVINCE + vue région en UN appel ; test entre deux doit insérer econ_aggregate_regions() pour voir divergence.
- Tier de province = POP unique (scps_labor.c:capitale_max_tier, T2 2000…T7 10000, registre J) — TOUT reader (façade, readout, IA) délègue là.
- Foreuse (v76) : panier minéraux + audit essence (scps_econ.c, dépôt chimie/géologie/alchimie).
- IA colonisation : n'ordonne plus EN GUERRE (at_war[] précalculé sim, econ diplo-free) ; grenier au déficit vivrier pas capitale figée.
- **RE-KEY PROVINCE des VERBES ÉCO joueur (2026-07-14, SAVE v85)** : `econ_region_rep_province` ne survivait QUE dans la façade/les drains (le moteur, `ProvinceEconomy`, était déjà province-grain — cf. entrée ci-dessus). Éliminée sur TOUS les chemins joueur économiques : CMD_BUILD/CMD_BUILD_MANUF/CMD_MANUF_LEVEL/CMD_DEMOLISH_EDI/CMD_ALLOC_* (scps_sim.c) portent un PID direct dans a[0]/a[1] ; `econ_build_manufacture`/`econ_manuf_level_delta`/`agency_demolish_edifice` prennent un PID direct (plus de résolution ni de miroir région[]) ; `BuildOrder.prov` (scps_agency.h, -1=héritage/≥0=PID direct posé par CMD_BUILD) porte le grain d'ÉCRITURE d'`apply_action` ; `agency_build_acct` gagne le paramètre `prov`. Façade : `scps_region_alloc`→`scps_province_alloc` (raws/pool/bâtiments/poids TOUS province) ; `scps_build_legal(_ex)`/`scps_manuf_legal`/`scps_player_build*`/`scps_player_alloc_*` prennent un PID. **PIÈGE collatéral trouvé** : `econ_build_manufacture` était aussi appelé par l'IA (scps_ai.c: `ai_build_manufacture`/`ai_pay_and_build`/`ai_build_civmanuf`) et les cités-états (scps_intertrade.c: `intertrade_seed_citystate_arms`) avec une RÉGION (pas listé dans le brief initial — la signature a changé pour TOUT le monde) : ces appelants résolvent désormais EUX-MÊMES `econ_region_rep_province` au point d'appel (byte-identique, même cache — golden intact). Le gate MATIÈRE (marché intertrade) et les gates géo (port/estuaire) restent volontairement au grain RÉGION (dérivée de la province au point d'entrée) — seule l'ÉCRITURE finale change de grain.
- **Enum appendu + boucle générique** : `for(cl=CLASS_BOURGEOIS; cl<CLASS_COUNT; …)` (scps_econ.c:econ_migrate_tick) réélargit silencieusement quand CLASS_SLAVE ajouté en fin → grep `CLASS_.*; .*<CLASS_COUNT` AVANT chaque ajout enum SocialClass. Parent : econ_tick (croissance), demography (fusion, assimilation_tick), revolt (mobilize/demobilize), agency (repression/purge_slice/biggest_minority), events (apply_region_eff pop_mult).
- **Statics non sérialisés** : TOUT accumulateur inter-ticks → SAVE (3 occurrences confirmées : EMOB v57 / COLC v61 / TXYR v65 ; signature = UNE SEULE grandeur dérive, reste byte-identique). Cas latents : ordonnanceur (next_day) non sérialisé = accumulateur AUSSI.
- **Vue région stale** : region[].raid_cd_days / stock[] post-econ_tick (mensuel) — gate/reader post-drain lisent ancien état (relire `prov[econ_region_rep_province(r)]` directement). Cf. diplo_demo §6, navy_course_tick gate.
- **Bancs aveugles** : test qui écrit→lit SANS econ_aggregate_regions() ou sim_day entre = AVEUGLE à write-fantôme (endgame_demo C6 / missions_demo §2 passaient AVANT fix v75).

**Esclavage** :
- 9 fuites strates-fantômes (v68 SLAVEDIAG) : `strata[CLASS_SLAVE]` + `groups[i].klass==CLASS_SLAVE` sont DEUX comptes parallèles PAS synchronisés (root causes scps_econ.c croissance · demography fusion/réfugié · revolt mobilize · agency purge · events pop_mult) — invariant Σ strates serviles == Σ groupes serviles manquait. « Tout `for(c<CLASS_COUNT)` sans connaître PopGroup casse l'invariant. »
- Capture : double-gate `diplo_pillage_region` + `diplo_enslave_capture` (scps_diplo.c:956-957) posait CD DEUX FOIS → SECOND bloqué. Fix : `diplo_pillage_fresh` (read partagée) + victim_cid paramètre (owner change AVANT pillage).
- Dépôt IA (scps_ai.c:1307) : RES_ARMS_LIGHT alias RES_ARMS (scps_types.h:209), site déjà routé `econ_region_stock_add`.

**Restes** :
- Esclavage : repère complet des 9 fuites en git (agent dédié repairera).
- Colonisation : auditer `econ_colonize_*` ponction CLASS_SLAVE (exclure du bassin).
- **RE-KEY PROVINCE incomplet (2026-07-14)** : les verbes SOCIAUX `CMD_REPRESS`/`CMD_ASSIMILATE`/`CMD_PURGE` (drain scps_sim.c → `agency_order_repress`/`_assimilate`/`_purge`, scps_agency.c) restent RÉGION-GRAIN (BuildOrder.region, sans `prov` posé — `apply_action` retombe sur la province représentative comme avant). Hors périmètre de la vague CMD_BUILD*/CMD_ALLOC* (édifices/manufactures/allocation) ; à transférer au même patron (a[0]=pid direct + BuildOrder.prov) au prochain passage si la doctrine doit s'étendre à l'intérieur/répression.

---

## SAVE / DÉTERMINISME

**Découvertes** :
- Sérialisation accumulateurs : g_friche/g_lowsat_streak (EMOB v57) · g_colony_cd (COLC v61) · g_flux (TXYR v65).
- Intertrade hub : g_hub_of/g_hub_dist (caches dérivés lus QUOTIDIENNEMENT, sérialisés À L'IDENTIQUE via intertrade_save/load v58).
- Replay byte-identique : savetest v74 (seeds 9/7/11/42) prouvé 200 ans ; determinism-deep 200 ans × 2 graines STABLE.
- Load robustesse : `scps_load_game` ne reset PAS statics modules → divergence dès v57/v61/v65 (fixe exhaustive : chaque module restore son état sérialisé).

**Pièges** :
- Piège savetest : capturer accumulateur APRÈS load quand flux annuel courant se lit post-reload TRONQUÉ (v65 g_flux). Drain CMD avant any flux-read pour force recalc.
- Statique ordonnanceur (next_day, scps_labor.c) non sérialisé = accumulateur AUSSI — relire `econ_migration_tick` : cadence liée à l'interne.

---

## WORLDGEN & GÉOGRAPHIE

**Découvertes** :
- Érosion hydraulique #1 (v74) : 200k gouttes xorshift → valeurs dendritiques ÉMERGENTES, déterministe (scps_world.c:step_hydraulic_erosion ~110 l).
- Lacs priority-flood #3 (v74) : tas min clé entière (niveau<<32|index), dépression FERMÉE → inonde jusqu'débordement (fill_lakes ~200 l).
- Rivières émergentes (v74) : trace_stem remonte MAIN STEM par flux (descendant pur), affluents plafonnés RIVER_MAX_TRIB/bassin → dendritique réparti (pas un gigantesque tronc).
- Biomes pente/limon #5 (v74) : escarpement RAIDE (slope>0.030) → collines nues (scps_world.c assign_biome_refined) ; plancher alluvial (slope<0.006, flux≥48) → verdure +1 cran ; TOUS dérivés (pas de champ).
- Archétypes graine (v75) : 8 types (pangée, continents, archipel, mer intérieure, froid, aride, jeune-montagneux, vieux-érodé) par avalanche hash (splitmix32).
- Falaises maritimes (v75) : lithologie (convergence plate-1 + litho_noise) · rampe côtière ÉRODÉE ∝ dureté (35%/30% step_erosion) · `cell.lake` = piège (plein relief) → seul `height<SEA_LEVEL` compte (scps_world.c:world_cliff_intensity dérivé).

**Pièges** :
- biome_habitabilité (scps_climate.c) manquait BIO_THORNS (ronces plus vivables que steppe !) → exode endgame bloqué — corrigé v74.
- endgame_region_intensity(FROID) était PLATE (modulation locale documentée, jamais codée) → froid uniform, pas de gradient → exode cible manquait.
- Priority-flood: décalage petit de bouche hydro-correct (cells.lake ne ré-accumulent flux après flood) — greenlight requis, hors scope.

**Restes** :
- Archétypes age BAS (pangée 0.48, jeune-montagneux 0.24) : masses FUSIONNÉES (dérive faible, géo-cohérent) ; si 4 masses visibles vouées : découpler dérive/age (plates_init prend world_age pour drift).
- SEA_LEVEL fixe (pas de slider) — land_amount en tient lieu ; si vraiment mobile : normaliser + recalibrer seuils (`CLIFF_H_MIN/MAX`, géographie).

---

## HAMEAUX LIBRES — capitale fantôme corrigée (2026-07-14, worktree w1-wild)

**Mission joueur** : « les WILD avaient une capitale — modèle cité-état Civ6. INDÉPENDANTS. TOUS. »

**Découvertes** :
- **1 slot-pays = 1 hameau DÉJÀ vrai avant cette mission** (scps_econ.c:1852-1899, `WILD_PLANT`/`wslots[]`) : le vieux modèle « pseudo-empire wild multi-hameaux » décrit dans certains commentaires est un COMMENTAIRE ancien, pas l'état du code — chaque hameau a déjà son cid distinct, sa diplomatie/guerre indépendante (`diplo_status(s->dp,o,best_emp)` déjà par paire de cid, scps_sim.c:105/`wild_cultural_tick`). Rien à changer côté « un slot par entité ».
- **La vraie « capitale fantôme »** : `build_hierarchy` (scps_world.c:2201-2208) pose `capital_prov` pour TOUS les slots-pays (y compris ceux qui deviendront WILD) AVANT la distribution des rôles — capital_prov pointe alors vers la plus grande province de la RÉGION héritée par l'agglomération territoriale pré-rôle, une province SANS AUCUN RAPPORT avec le hameau réellement planté plus tard par `econ_init`/`WILD_PLANT` (BFS près d'un spawn jouable, scps_econ.c:1873-1897). Un commentaire pré-existant (scps_world.c:2889 avant fix) le savait déjà côté nom/couleur (« le slot WILD réservé n'a pas de capitale ») mais capital_prov lui-même restait fantôme.
- **`refine_capitals`** (scps_world.c:3522) tourne DANS `world_generate`, donc AVANT `econ_init`/`worldgen_seed_peoples` — aucun conflit d'ordre avec le fix (pas de risque d'écrasement après coup).
- **capital_prov n'est pas cosmétique pour WILD** : `scps_warhost.c` (lignes 273-403) filtre seulement `role==POLITY_UNCLAIMED`, donc mobilise/défend AU capital_prov pour les WILD aussi (« ils défendent » du contrat) — avec l'ancien capital_prov fantôme, un hameau tentait de défendre une province qu'il NE POSSÉDAIT MÊME PAS. Le fix n'est donc pas qu'un affichage : il corrige potentiellement la défense réelle des hameaux.

**Fix** (scps_world.c, `worldgen_seed_peoples`, bloc juste après le nommage tribal WILD) : au lieu de démonter un slot WILD sans région à `has==false` (test sur `econ->region[].owner`), on cherche directement la province ÉCONOMIQUEMENT possédée par le slot (`econ->prov[p].owner==c && colonized` — doctrine province-grain) et on pose `capital_prov = hamlet` (ou -1 + UNCLAIMED si aucun hameau planté). Diff minimal, aucun champ ajouté/retiré → **pas de bump SAVE_VERSION** (struct Country inchangée).

**Impact save** : les vieilles saves gardent leur capital_prov fantôme (World est désérialisé DIRECTEMENT depuis le fichier — `sv_read_payload`, pas de regen depuis la graine au chargement) ; `save_sane` reste satisfait (capital_prov fantôme est un index de province valide, juste sémantiquement faux) — auto-guérit seulement sur une PARTIE NEUVE. Non bloquant, non traité (hors scope : aucune migration de save demandée).

**Gates** : `make test` 39/40 vert (intertrade_demo = KO Windows pré-existant, inchangé) · golden RE-BASELINÉ (worldgen change confirmé — hash monde a changé) + `make determinism` stable après re-baseline · `make golden` vert · sweep chronicle (seed 9, 3 sims × 250 ans + seed 42) : IPM final moy 1.03 (pic 1.11) ≤ 1.35, satisfaction Laborer/Bourgeois/Élite 50/76/77 %, hameaux WILD semés 6.7/sim, colonisation/guerres/sécessions actives (monde vivant, pas de collapse) · `--savetest 9` byte-identique (A==B) · `--fuzztest` 8/8 (216 octets flippés, save_sane rejette chaque forge).

**Restes** :
- **Affichage godot/project** : si le front dessine un marqueur « capitale » distinct par-dessus un hameau WILD (icône/étoile), il vivra maintenant sur LA bonne province (plus fantôme) — mais un hameau libre n'a normalement PAS besoin de ce marqueur (sa seule province EST son siège, trivial, modèle Civ6). Un agent UI devrait vérifier si `scps_country_capital_province`/l'icône capitale s'affiche encore sur les hameaux et, si oui, la retirer (ou la garder — décision joueur, hors périmètre INTERDIT godot/project de cette mission).
- Aucune migration de save pour les parties déjà commencées avant ce fix (capital_prov fantôme y persiste jusqu'à nouvelle partie) — non demandé, documenté ici si jamais ça remonte en bug report.

---

## RELIGION & CULTURE

**Découvertes** :
- Fondation religion : requiert TEMPLE T2 bâti (masque TEMPLE|CATHÉDRALE, scps_religion.c:trig_faith_founded) — change IA zèle (avant Sanctuaire fait=0, nocap_bloque maintenant).
- Plafond : ⌈N/2⌉ racines × ≤2 schismes PAR racine (religion_cap_schism_max, scps_religion.c).
- Culture par groupe (v77) : nom stable groupe, filiations (substrat A sous élite B), métabolisation PAR ARR_mode (natif/migrant/soumis/déporté, scps_demography.c:PopGroup.arrival).

**Pièges** :
- Métabolisation math : `econ_country_heritage_digested` divise POP TOTALE (voulu tech-access tier-3, impossible COMPTE) — Merveille lit diaspora INDIVIDUALISÉE (dig_X/tot_X≥0.60 + 500 âmes, pondérée ARR). Dénominateur question : pop_total vs diaspora de CET héritage.
- Fondation religieuse étouffe Sanctuaire (nocap ~60-70 > dispo bois ~14 au pool §5) — aucun fix appliqué (décision équilibrage : assouplir quoi ?).

**Restes** :
- Events B2/B3/B5/B6 (consommateurs credo_drift) · C2/C3/C4 (credo RUPTURE trig) : NON créés (signal en place, mots porteurs).
- Lettré IA : schisme RUPTURE auto-résolvant (repick aléatoire valide).

---

## TECH & ARBRE

**Découvertes** :
- Étoffe 12 branches tier 1-2 (v74) : parallèles tier-3 signature existante → tiers graduée par accès (contact/métabolisation, scps_ai.c:heritage_access_pack) + coût √N provinces (tech_cost_n_k 0.90) + bonus métabolisation +X% recherche (AI_METAB_RES_W 1.0) + remise diffusion (AI_TECH_DIFFUSE_MAX 0.40).
- Apex triples tier-5 (v74) : Arquebuse runique/Concile/Légion ; 3 héritage PLEIN (tier 3 natif OU métabolisé) ; Arquebuse hook firearm_power +0.50.
- Push arbre (v74) : AI_RESEARCH_INCOME_W 4.5 (28→50 % arbre) ; ENTROPY_TECH_W 0.20 (coût faustien ×0.20 annule boost, repli an-240 FIN_CHAUD monde sans fin).

**Pièges** :
- Soif de palier (épargne tier-2, motif S1 similaire SOIF_DE_SAVOIR Scriptorium→Université) : aide peu si arc ne démarre pas → audit S3/S4/FAU5.

**Restes** :
- Arbre 50 %<60 % cible (clamp stock négatif membrane V5 levé) ; arbre moteur 28.3 % (re-baseline c/sweep).

**Découvertes (refonte UI Civ 6, 2026-07-14, tech_panel.gd/tech_popup.gd — GDScript seul)** :
- La façade EXPOSE déjà exactement ce qu'il fallait pour « titre / plusieurs effets au survol / flavor au popup » — aucun trou côté binding : `tech_nodes()[i].hover` est un CHIFFRÉ multi-effets ("mot mécanique — dK +x · dL +x · production +x % · …", scps_api.c:2668-2698, généré depuis les seuls leviers VIVANTS), `.effet` un résumé court, `.flavor` (scps_api.c:2698 `tech_flavor((TechId)i)`) est PUREMENT flavor et n'était encore JAMAIS séparé de `.hover` côté UI — l'ancien tech_panel.gd les concaténait tous les deux dans le même tooltip.
- Les « trois arbres = Forge·Société·Savoir » du prompt sont déjà les 3 COULOIRS de l'ancien Medusa radial : `l = int(nd["quarter"]) / 3` (3 quartiers par couloir, ordre THM_* de scps_tech.h), `LANE_NAMES := ["Savoir","Forge","Société"]` indexé par `l` — aucun nouveau champ côté façade nécessaire, juste réutiliser le même calcul.
- `Atom` (addons/medusa) fait un hit-test CIRCULAIRE par défaut (`_has_point` : distance au centre ≤ `get_effective_radius()`) sauf si `icon` est posé (alors plein-rect via un `click_mask` généré depuis l'alpha de la texture) — inadapté à une carte RECTANGULAIRE compacte façon Civ 6 sans bricoler une texture blanche pleine juste pour le hit-test. Abandonné Medusa Graph/Atom entièrement au profit d'une classe imbriquée `TechCard extends Control` (hit-test = tout le rect par défaut) — plus simple, plus robuste, `tooltip_text` natif marche identique (Control l'hérite déjà, c'était déjà le mécanisme utilisé par Atom).
- `tech_panel.gd` est un enfant PERSISTANT ajouté une fois dans `main/main.gd:157-159` (`_tech = load(...).new(); _tech.name="TechPanel"`), juste caché (`visible=false`) — confirmé AVANT de coder le popup : pas besoin du repli « popup au prochain visible=true » en dernier recours dégradé, c'est le comportement NOMINAL retenu (le popup de découverte ne doit interrompre aucun input critique ⇒ s'il survient panneau fermé, il est mis en attente `_pending_discoveries[]` et s'ouvre à la prochaine ouverture du panneau — testé via le motif réel `_queue_discovery()`, pas un raccourci de probe).
- Un Control ajouté en enfant `top_level=true` n'échappe PAS à la visibilité en CASCADE de son ancêtre (seule la TRANSFORMATION ignore l'ancêtre) — confirme que le popup ne peut PAS s'afficher panneau fermé sans reparenter hors de tech_panel (hors périmètre fichiers autorisés) ; d'où le choix ci-dessus.
- Layout Civ 6 « aucun scroll vertical » obtenu en dimensionnant `card_h` pour que `rows_per_cell` cartes + gutters remplissent EXACTEMENT `lane_avail_h` (jamais plus) — le ScrollContainer garde `vertical_scroll_mode=DISABLED` en toute sécurité (un DISABLED sur un contenu qui déborde aurait juste CLIPPÉ sans le dire) ; la largeur, elle, grandit librement par tier (sous-colonnes de `rows_per_cell` cartes empilées) → `horizontal_scroll_mode=AUTO` absorbe tout excès. Vérifié en probe (74 nœuds, seed 42, an 118 → largeur contenu 2276 px, aucune carte hors-lane, aucun débordement vertical).
- Les noms de couloir (Forge/Société/Savoir) sont dessinés HORS du ScrollContainer, directement dans `tech_panel._draw()`, à `x` fixe et `y = _scroll.position.y + _lane_y0[l] + …` (positions VERTICALES posées par `_build()`, jamais scrollées puisque le scroll est latéral seul) — donne des en-têtes de rangée FIGÉS façon Civ 6 sans widget dédié : le scroll horizontal ne fait bouger que le contenu, jamais l'étiquette de couloir.
- `move_child(_popup, get_child_count() - 1)` est OBLIGATOIRE avant chaque `show_tech()` — `_popup` est ajouté en tout premier dans `_ready()` (avant que `_build()` ajoute `_scroll`/les cartes), donc sans ce ré-ordonnancement le popup se dessine SOUS les cartes du dernier tier (repéré au screenshot : titre+effets du popup visibles en transparence derrière « Fonderie »/« Rouages de précision », pas une histoire d'alpha).

**Pièges (idem, probe)** :
- Une `InputEventMouseMotion` de synthèse via `Input.parse_input_event()` + `await create_timer(0.65s)` ne déclenche PAS le tooltip natif dans une capture `get_viewport().get_texture()` en probe fenêtré (`tech_shot.gd`) — le tooltip Godot semble rendu par un mécanisme hors de la texture de viewport capturée (fenêtre/overlay séparé). Le contenu du hover (`tooltip_text`) est correct par construction/lecture de code (même mécanisme Control natif qu'avant sur Atom) mais n'a PAS pu être vérifié PAR CAPTURE — seulement par inspection du texte assemblé. Non bloquant (mission l'autorise explicitement en "si possible").
- Appeler `card.activated`-style directement (`tp._on_card_activated(idx)`) dans un probe LANCE réellement `player_research` si la carte est `allowed` — utile pour tester header+footer en une passe, mais change l'état du monde pour les captures suivantes (le nœud ciblé passe "en cours" avant le test popup) ; assumé dans `tech_shot.gd` (ordre des captures : arbre → survol → dossier/footer → popup).

**Restes (idem)** :
- `nd.get("unlocks","")` a été observée VALANT LE MÊME NOM que le nœud lui-même sur au moins un cas (Scriptorium → "Débouche sur : Scriptorium", seed 42 an 118, capturé en popup ET dossier) — champ lu tel quel (identique à l'ancien tooltip, comportement PRÉEXISTANT), pas creusé côté scps_tech.c (hors fichiers autorisés pour cette mission) ; possible curiosité de nommage sur les nœuds étoffe/combo, à vérifier si un futur ticket touche `tech_tree_readout`.
- Pas de barre de progression EN DIRECT sur la carte elle-même (Civ 6 remplit visuellement la carte en cours de recherche) — écarté volontairement : les cartes ne sont reconstruites qu'à `_build()` (ouverture/génération), un remplissage live aurait exigé de retrouver la carte ciblée à CHAQUE tick depuis les coordonnées scrollées (offset `_scroll.scroll_horizontal`) pour un gain cosmétique mineur face au bandeau d'en-tête qui affiche déjà nom+% en direct. Laissé pour une itération future si demandé.
- Aucune ligne fine réservée aux prérequis qui traversent BEAUCOUP de couloirs/tiers (le mission accepte le remplacement minimal « nommé au survol » quand ça croise trop) — les arêtes sont de simples segments semi-transparents, jamais évitées/routées ; lisible sur l'échantillon testé (74 nœuds) mais pourrait densifier sur un arbre encore plus large.
- Fichiers de probe créés (`tech_shot.gd`/`.tscn`) laissés dans `godot/project/` à la racine, même convention que `empire_shot.gd`/`province_shot.gd` — non nettoyés (utiles pour re-vérifier visuellement après un futur changement de tech_panel.gd).

---

## GUERRE & DIPLOMATIE

**Découvertes** :
- Pillage uniformisé (v75) : double-gate diplo_pillage + diplo_enslave → `diplo_pillage_fresh` (read partagée, scps_diplo.c:diplo_pillage_region) + victim_cid paramètre (owner change AVANT).
- Razzia pirate (v75) : gate IA éthos Dominateur/Honneur + tech enslavage ; verbe joueur exclusion ALLIÉ/PACTE ; CD/balafre province-owned (navy_mark_raided, scps_navy.c).
- Révolte COMPLÈTE (v55 phase 3a) : soulèvement = pays rebelle (slot ANTAGONIST) + armée campagne + déclare guerre + siège région ; résolution par score de guerre ; soutien étranger 2e front ; nœud vétérans +2 piquiers (REBEL_VET_ADD 2, scps_revolt.c).
- Vassalité (v32) : intégration lente (20 ans VASSAL_INTEGRATE_YEARS) + contribution typée (agraire/martial/commerce) + annexion-PROCESSUS (discount intégration, scps_diplo.c:suzerainty_tick).

**Pièges** :
- diplo_demo §6 posait vue region[] sans provinces support.
- navy_course_tick signature +TechState* — NULL gate désarme razzia proprement (if(ts && …)).
- g_navy_raid_slaves / g_occ_pillage_total : globaux télémétrie jamais RAZ inter-sims → delta-snapshot chronicle, jamais sérialiser/lire moteur.

**Restes** :
- Razzia IA « tout pirate razzie » (si asservissement pirate historique sans doctr. État voulu) → retirer gate can_enslave scps_navy.c.

---

## GODOT & UI

**Découvertes** :
- Parchemin unique (v75) : shader 100 % procédural (iso_antique.gdshader) — lavis sépia, côtes encre, rivières plume, relief, rose des vents, pas d'asset carte.
- Frontières calligraphiques (v75) : façade scps_border_segments_col (owner+normal) → binding dict → overlay ruban dégradé (inline éthos, outline héritage) 5 passes Taubin ; capital = liseré fin (CAP_INK).
- Urbanisme retiré (v75) : −738 lignes overlay.gd (composer town intégral + 5 renderers vectoriels) — urbaniste Godot solo désormais.
- Assets eau (lot V) : canopée extras (±3.5 cellules offset) + placement primaire testent LAYER_WATER (scps_water_at) ; urbaniste rangs/champs/tours/moulin pullés _pull_dry (relire biome). Routes/ponts/dressing DÉJÀ corrects (resample LAYER_WATER chaque point).
- Tier urbaniste (lot U) : 8 vignettes (t1-t7) recentrées (58514f connected-components) + t7 UNIQUE global (boucle pays/tick _update_top_cap) ; cache par sid+tier (rebâtit si tier monte OU t7 change mains).

**Pièges** :
- Marais (biome 15, symbole cartographique grille sinus) ≠ eau (is_water) — la grille est volontaire, AUCUN fix.
- `var := dict.get(...)` (Variant inféré) REFUSE GDScript `-Wall` — typer explicitement.
- MSAA 2D n'existe pas GL Compatibility (gater sur get_rendering_device()) — antialiasing par-trait suffit.
- `.godot/` reimport requis (warnings BÉNINS ignorés, « [DONE] reimport » suffit).
- shot_parch : ignore `cap=1` quand cx=/cy= passés (arg explicite écrase défaut).

**Restes** :
- Lavis variant_map (V3 statut, reader façade `scps_map_owner` → binding political_image).
- Enceinte vignette cités-états : arcs pas gardés (retirer point-à-point déformerait cercle), wrad large déborde presqu'île rare (non observé seeds 9/11/42).
- « or/an » restant dans `province_panel.gd` (fiche province, hors périmètre — un autre agent la possède) : lignes 317/882, à linéariser /mois quand cette fiche sera traitée.
- `codex.gd:34` (« 1 ordre/an ») et les mentions « une colonie par an » (`province_panel.gd`) sont des CADENCES de jeu (cooldown), pas des flux monétaires — volontairement NON converties en /mois (÷12 d'un cooldown discret n'a pas de sens).

**Découvertes (lot linéarisation + sphère, 2026-07-14)** :
- Le mot « sphère » venait bien de la FAÇADE : `ScpsHeritage.sphere` (scps_api.h:1404, rempli par `sphere_name(heritage_sphere(h))` dans scps_api.c:4284) — les valeurs sont « Anciens/Hommes/Étrangers » (scps_heritage.c:24-26), le vocabulaire « espèce » interne. Le champ existe pour la mécanique de distance culturelle (`sphere_distance`, kinship diplo) — **ne pas renommer/retirer côté C**, juste ne plus l'AFFICHER. `culture_creator.gd` l'utilisait tel quel en sous-titre de carte (l.400) et en préfixe du texte info (l.562) ; remplacé par le champ `exemple` (ethnonyme déjà généré côté façade, `scps_heritage_list`) qui existait déjà dans le Dictionary mais n'était pas consommé ailleurs que dans un commentaire.
- `sidebar_drawer.gd` avait DEUX classes de « or/an » : (1) des valeurs RÉELLEMENT annuelles (intertrade `g_gold[cid]`/`g_pair[][]`, RAZ+recalculées 1×/an par `intertrade_tick`, appelé depuis `scps_sim.c:1208` sous `if (s->day % 365 == 364)`) — conversion ÷12 légitime (export commerce, partenaires) ; (2) des COMMENTAIRES périmés (« N or par an ») décrivant un code qui affichait déjà « or / mois » (conseil : `cost_year/12.0`, l.737/914/938/997) — juste la doc en retard sur le code, mise à jour sans toucher la logique. Seul le décret (`_draw_decree_card`, l.1126-1135) affichait vraiment « or par an » en dur (`cyear` non divisé) — corrigé (÷12, hover sans le calcul explicite « /12 »).
- `topbar.gd`/`budget_panel_v2.gd` : TOUT l'affichage réel était déjà en /mois (`budget_summary().monthly_net`, `country_budget` × 30/day_of_year) — seuls deux commentaires disaient « revenu net annuel », corrigés en « mensuel » pour ne pas induire en erreur le prochain agent qui grep.
- Pas de « sphère »/« espèce » ailleurs dans godot/project (codex.gd et le reste du projet en étaient déjà propres) ; pas de STR_* C à toucher (aucune string ids/en concernée) ⇒ pas de rebuild DLL ni de `make lang-check` nécessaire pour cette vague.

**Piège (lot linéarisation)** :
- `empire_window.gd` : `refresh()` de la page Population est appelé à CHAQUE changement d'onglet (pas seulement au tick mensuel `Sim.month_ticked`) — un delta de croissance naïf (pop_totale − dernière_valeur) serait bruité si le joueur bascule vite entre onglets. Normalisé sur le JOUR ABSOLU réel (`year()×365+day_of_year()`, pas un compteur d'appels) pour rester honnête même si les refresh ne tombent pas pile /30j.

**Découvertes (fiche province v2 + menu construction, 2026-07-14, LOT lisibilité)** :
- `province_info` (ScpsProvInfo) portait DÉJÀ `logements_libres/_cap`/`services_libres/_cap` côté C/façade (scps_api.h:122) — jamais consommés par `province_panel_v2.gd` ni par `province_detail.gd` (grep confirmé). Aucun reader manquant : juste câblés dans la fiche (chantier logements/services).
- `Province.habitability` (scps_types.h:242, géo pure figée à la genèse) N'ÉTAIT PAS exposé — ajouté `ScpsProvInfo.habitabilite_pct` (0-100, `scps_api.c:scps_province_info`) ; **PIÈGE** : ajouter le champ au struct C + au remplissage `scps_api.c` NE SUFFIT PAS — `scps_sim_node.cpp::province_info()` a sa PROPRE recopie Dictionary champ par champ (pas de boucle générique) : oublié une fois, `info.get("habitabilite_pct",-1)` silencieusement -1 côté GDScript (aucune erreur, juste absent) ; repéré seulement en RELISANT le screenshot de hover (ligne « Habitabilité » manquante) — toujours VÉRIFIER visuellement un champ neuf, pas seulement compiler.
- Recette de manufacture (intrants→produit, quantités) n'existait NULLE PART côté façade (seul `building_recipe(bld,&in1,&in2,&out)` — les ENUMS, pas les quantités — scps_econ.c:793, déjà exporté pour l'IA). Ajouté `building_recipe_qty` (scps_econ.c/.h, miroir pur) + `scps_manuf_recipe`/`ScpsManufRecipe` (scps_api.h/.c, noms résolus via `resource_name`) + binding `manuf_recipe(bld)`. Sert DEUX endroits : le hover des chips manuf bâtis (province_panel_v2, `«produit +Y/mois»` matché par `manuf_recipe(bld).out` contre `province_income`) ET les cartes du menu construction (`«Laine ×1.5 → Étoffe ×2.8»`).
- `province_income`/`province_income_prov` (scps_readout.c:612) : le champ `source` d'une ligne MANUFACTURÉE est le nom du BIEN PRODUIT (resource_name), jamais le nom du bâtiment — c'est la clé de jointure avec `manuf_recipe(bld).out` (pas `manuf_name(bld)`).
- `scps_edifice_succ` renvoie le SENTINEL `EDIFICE_COUNT` (pas -1) quand un édifice n'a pas de palier suivant — un test `succ >= 0` (motif préexistant dans `_edi_chip`, province_panel_v2.gd, non touché) est donc TOUJOURS vrai et se repose sur le gate suivant (`build_legal`) pour ne rien afficher. Pour le nouveau tag « Prochain palier » du menu construction, évité ce piège : `_bytype.get(succ, {})` (dict qui ne contient QUE les types valides 0..EDIFICE_COUNT-1) rend le sentinel naturellement absent, sans connaître sa valeur numérique.
- `_make_custom_tooltip(for_text) -> Object` (Control, natif Godot 4) marche très bien pour un hover IMAGE+texte riche sans widget dédié : classe interne GDScript (`class BiomeTip: extends PanelContainer`) construite à la demande dans le hover, retournée depuis un `HBoxContainer` custom (`TerrainRow`) avec `tooltip_text=" "` (non-vide, sinon Godot ne déclenche jamais le hover) + `mouse_filter=STOP`. Aucun fichier neuf nécessaire (classes imbriquées dans `province_panel_v2.gd`) ; se PROUVE en probe headless en appelant `_make_custom_tooltip("")` directement et en l'ajoutant à l'arbre (le vrai hover dépend d'un minuteur réel, impossible à attendre proprement dans un probe).
- Menu construction : le filtre « tech-locked = pas listé comme posable » existait DÉJÀ (`if not on2: continue`) mais masquait aussi la CONNAISSANCE du palier suivant. Restructuré pour dériver le tag « Prochain palier » de la carte VISIBLE (via `edifice_succ`) plutôt que de lister le palier verrouillé lui-même — le texte de refus long (« indisponible ici (palier, file ou bâtiment existant) ») déborde la carte s'il n'est pas passé dans `_fit()` (repéré au screenshot, pas en lisant le code).

**Restes (fiche province v2 + menu construction)** :
- Aucun reader d'ENTRETIEN/upkeep récurrent pour les ÉDIFICES côté moteur (`EdificeDef` n'a que `days`+`cost`+`delta`, scps_agency.h:57 — un bâtiment n'a PAS de coût /mois, contrairement aux unités qui ont `entretien_or10`). La ligne « Entretien » du mockup de mission est donc OMISE intentionnellement (rien à lire, rien à inventer) ; le prix affiché reste le coût de CHANTIER (or + jours), déjà réel.
- `_bld_slot`/`_res_chip` (province_panel_v2.gd) restent du code mort PRÉEXISTANT (jamais appelés avant NI après cette vague) — laissés en l'état, hors périmètre de la mission (pas de nettoyage non demandé).
- `province_panel.gd` (legacy, sans le `_v2`) garde ses ProgressBar de classe et ses chips fantômes « à bâtir » — HORS PÉRIMÈTRE de cette mission (fiche `_v2` seulement) ; à harmoniser si `province_panel.gd` est un jour retiré au profit de `_v2`.

**Découvertes (JOURNAL persistant des notifications, 2026-07-14)** :
- Le rail droit PERMANENT est `empire_sidebar.gd` (W=288, `Frame.LEDGER_W`) — PAS `sidebar_drawer.gd` (le TIROIR à 8 onglets, ouvert par clic, mutuellement exclusif avec la fiche province). `empire_sidebar.gd` avait DÉJÀ une section « JOURNAL » mais c'était une résolution de texte DUPLIQUÉE (sa propre `_poll()` relisait `AlertsK.FEED_KINDS` et reconstruisait le tip lui-même, plafonné à 18 entrées, SANS couleur/icône) — les vraies notifications colorées vivaient ailleurs (`alerts.gd::_stack()` = `_events`+`_alerts`, rendu par la section « NOTIFICATIONS » du même panneau).
- Fix : `alerts.gd` devient l'UNIQUE point de résolution des DEUX sorties. `_push_journal(entry)` (ring `JOURNAL_MAX`=200, push_front) est appelé à DEUX endroits : (1) dans `_poll_feed()`, pour CHAQUE kind du fil moteur, AVANT le tri chip/popup (`kind in POPUP_KINDS: continue`) — sinon les popups majeurs (guerre déclarée, révolte, sécession, évènement directeur) n'auraient jamais atteint le journal, alors que ce sont justement les notifications les plus « importantes » ; (2) via `_journal_track_conditions(alerts)`, appelé juste après `_alerts = _collect()` dans `_refresh()` — les CONDITIONS (conseil vacant, pas de recherche, pénurie…) n'ont pas de `seq` (elles sont recalculées à CHAQUE tick, pas des évènements discrets) : une clé stable `_cond_key` (act+région+tip AVEC LES CHIFFRES RETIRÉS) fait l'edge-detection — sans ça, « 3 siège(s) vacant(s) » puis « 2 » puis « 1 » auraient créé 3 entrées au lieu d'une seule (le journal enregistre l'APPARITION, pas chaque variation du compteur).
- `empire_sidebar.gd` lit désormais `_alerts_source.call("journal_rows")` (comme `ledger_rows()` déjà utilisé pour NOTIFICATIONS) — sa propre `_poll()`/`_log`/`_seen_seq`/`_log_rects` (résolution dupliquée) SUPPRIMÉS. `_region_name()`/`_city_names` GARDÉS (pas une résolution de contenu, juste une substitution COSMÉTIQUE : le `tip` moteur contient « région <N> » numérique — le seul format que `feed_poll` connaît — remplacé par le nom lisible AU RENDU, dans `_journal_full_text`, sans toucher le tip canonique stocké dans `_journal`).
- `activate_ledger`/nouveau `activate_journal` PARTAGENT `_route_action(al)` — les deux vocabulaires d'`act` (évènements : `tech_metab`/`goto` ; conditions : `council`/`army`/`market`/`tech`/`construct`/`religion`/`age`/`goto`) sont DISJOINTS par construction (un évènement n'a jamais `act="council"`, une condition n'a jamais `act="tech_metab"`), donc un seul `match` fusionné couvre les deux branches sans collision — ancienne duplication (deux blocs `match` quasi identiques dans `activate_ledger`) éliminée au passage.

**Piège (journal)** :
- Le gabarit `FEED_KINDS[k].fmt` inclut TOUJOURS « (an {y}) » en fin de motif (ex. `"PAIX signée avec {a} (an {y})"`) — l'ancien `_poll()` d'`empire_sidebar.gd` le retirait via `.replace(" (an {y})", "")` **sur le TEMPLATE brut** (avant substitution de `{y}`) puisqu'il préfixait déjà « an %d · » lui-même. En réutilisant le `tip` déjà résolu d'`alerts.gd` (où `{y}` est remplacé par la valeur réelle), il faut retirer `" (an %d)" % annee_reelle` (valeur CONNUE, pas le placeholder) — sinon chaque ligne affiche l'année EN DOUBLE (« an 82 · PAIX signée avec Clans Karrokor (an 82) — … »). Fait dans `_journal_full_text` (empire_sidebar.gd), pas dans `alerts.gd` (le tip canonique, utilisé aussi par les popups/chips SANS préfixe d'année, doit garder son « (an N) »).
- `sidebar_audit.gd` (probe existant) instancie `empire_sidebar.gd` SEUL, sans `alerts.gd`/`set_alert_source` → `_alerts_source == null` → la section JOURNAL doit se réduire à « rien à signaler » sans planter (`_alerts_source != null and _alerts_source.has_method(...)` gardé partout) ; reste un bon test de non-régression (revérifié : 0 SCRIPT ERROR après ce lot).
- La hauteur du panneau (`_maxh = vp.y − TOPBAR_H − BOTTOMBAR_H`) s'est révélée TOUJOURS suffisante en probe headless (67 entrées + tout le résumé d'empire tiennent SANS scroll, `_maxscroll` mesuré à 0.0 même en fenêtre 560×1000) — la fenêtre de test a fini par grandir plus que demandé (`get_window().size` semble s'appliquer avant le premier `_layout()`), donc NE PAS supposer qu'un panneau à 200 entrées ne défilera JAMAIS en jeu réel (fenêtre plus petite) ; le mécanisme de scroll existant (`_scrolloff`/molette) reste le filet, non testé interactivement ici (headless).

**Restes (journal)** :
- Aucun cap d'AFFICHAGE volontaire sur les lignes dessinées (contrairement à VILLES, plafonné à 10 + « … et N autres ») — les 200 entrées du ring sont TOUTES dessinées, seul le scroll du panneau absorbe le surplus. Repose sur le pattern de scroll déjà éprouvé par le reste du panneau (armées/mission/etc.) ; à revisiter si un futur audit mesure un coût de frame notable à 200 lignes en jeu réel (non mesuré ici, probe headless seulement).
- `journal_audit.gd`/`.tscn` (probe créé pour cette mission) avance le monde par appels DIRECTS `Sim.world.advance_days()` (comme `sidebar_audit.gd`) plutôt que par la boucle normale de `Sim` — les signaux `Sim.ticked`/`month_ticked` ne sont donc PAS émis pendant l'avance (seul `alerts._refresh()` en fin de script, via `_ready()`/`_refresh.call_deferred()`, draine tout le fil accumulé d'un coup) ; suffisant pour peupler et vérifier le journal, mais ne teste pas le comportement « une entrée par tick réel ».

---

## AGENTS & PROCESS

**Pièges dedupliés** :
- **Enum + boucle générique** : `for(c<CLASS_COUNT)` réélargit silencieusement APRÈS CLASS_LABORER — grep AVANT chaque ajout SocialClass.
- **Statics non sérialisés** : accumulateur inter-ticks TOUJOURS en SAVE (3 occurrences EMOB/COLC/TXYR, motif identifié, classe de bug confirmée).
- **Godot worktree** : godot-cpp absent → robocopy autre worktree DÉJÀ construit (176 Mo .a+.dll), scons recompile moteur+binding seulement.
- **Savetest vs bancs** : test sans econ_aggregate_regions() entre write et read = AVEUGLE (bancs ≠ sim_day complet).
- **Git stash/pop** : `git stash push -- <fichiers>` (explicite, jamais nu) = pare-feu multi-session rapide ; Re-Read après tout replace_all (offset bougé).

---

**Synthèse** : 47 entrées (07-06…07-13) → 8 systèmes ; pièges dedupliés (enum/statics/génériques/vue-stale/bancs-aveugles) avec repères fichier:fonction ; découvertes structurelles ; restes ouverts par système (esclavage/fondation-religion/arbre/razzia/ui).

---

## PANNEAU ARMÉE — SECTION COMBAT temps réel + résultat (2026-07-14)

**Découvertes** :
- Tout le nécessaire était DÉJÀ bindé (godot/src/scps_sim_node.cpp) : `corps_ids/corps_info` (compo inf/arch/cav/mages en HOMMES via `units_are_humans`, journal de campagne taken/legs/battles), `battle_info(region)` (les DEUX camps + phase + stage Choc/Accalmie + morale_pct + terrain/counter/balance/rupture ×100 + loss_atk/def en PAQUETS de 100 + toute la lecture de siège + war_score), `region_war_state(region)` (0 paix · 1 assiégée · 2 occupée + belligerent), `corps_move_preview/corps_refill_preview`, verbes `player_corps_*`. AUCUN reader façade manquant pour cette mission n'a bloqué (voir Restes pour les absents non-bloquants).
- Le panneau `ui/army_panel.gd` (barre de commandement) et `ui/battle_panel.gd` (panneau de combat W-GUERRE lot B, ouvert par clic sur le JETON de la carte via main.gd::_on_province_picked) coexistent — la mission a mis la section combat DANS army_panel (suit la sélection du corps) sans toucher battle_panel (fichier d'un autre lot, non listé interdit mais laissé intact : deux chemins d'accès au même read `battle_info`, pas de duplication de logique moteur, seulement de présentation).
- **Verdict de fin de combat** : le fil (feed_poll, kinds 8 BATAILLE GAGNÉE / 9 PERDUE / 11 INDÉCISE, `v` = pertes packées nôtres|ennemies<<16 en paquets de 100 — cf. alerts.gd::_battle_losses_text) est la SEULE source du verdict TRANCHÉ côté joueur. Pour un SIÈGE PUR il n'y a AUCUN évènement de fil : le verdict honnête est la DISPOSITION de la place — `region_war_state` state 2 + belligerent == attaquant (occupée, paix pas encore signée) OU `region_owner(region) == attaquant` (propriété déjà basculée). Mesuré en probe : un siège peut ABOUTIR (corps `taken` incrémenté) puis la paix REND la région au défenseur → l'encart final doit dire la disposition FINALE (« la région reste à X »), pas un « Victoire/Défaite » déduit d'un snapshot périmé.
- `feed_poll(after_seq)` est une lecture PURE à curseur — appeler `feed_poll(0)` depuis un second consommateur (army_panel) ne dérange PAS le curseur `_seen_seq` d'alerts.gd (chacun le sien).

**Pièges** :
- **GDScript `:=` sur un retour de `Sim.world`** : `Sim.world` est UNTYPED à dessein (sim.gd, garde-fou libscps absente) → `var x := w.player_declare_war(t)` est une ERREUR DE PARSE (« Cannot infer the type ») qui fait échouer le CHARGEMENT du script ENTIER — la scène tourne alors à vide, fenêtre ouverte, sans jamais quit() : le probe semble « gelé » alors qu'il n'a jamais démarré. Toujours typer explicitement (`var x: bool = ...`) les retours du binding.
- **Probe + pipes bash** : `godot_console.exe ... | tail/grep/head` BUFFERISE — aucun output visible avant la fin du process ; si on kill le task bash, Godot survit ORPHELIN bloqué sur le pipe cassé (233 Mo, stall). Rediriger vers un FICHIER (`> build/x.log 2>&1`) et le lire, jamais de pipe sur un run long.
- **Lever un corps en probe** : à l'an ~90 la réserve du joueur (warhost) peut être 0 ; la séquence prouvée (army_audit) est `player_set_levy(3)` + `player_recruit(0..5)` + boucle `player_refill()`+`advance_days(180)` → réserve ~6 régiments → `player_raise_corps(reserve/2, capitale)`. Le premier `player_refill` seul ne suffit PAS (reserve reste 0 sans recruit/levy).
- `battle_info.loss_*` est en PAQUETS (float) → ×100 pour des hommes ; `corps_info` est déjà en hommes SI `units_are_humans` (sinon ×100, compat vieille DLL — motif déjà dans army_panel).

**Restes** :
- **ENTRETIEN par corps ABSENT de la façade** : `unit_roster` expose `entretien_or10/entretien_vivre` PAR TYPE d'unité (génériques, labor_upkeep_per100) mais rien ne donne le coût /mois d'UN corps précis (sa compo × entretien, ni la part solde/vivres réellement payée par le trésor pour CE corps). La section ENTRETIEN du panneau est donc OMISE (doctrine : valeur réelle /mois ou rien). Reader souhaitable : `scps_corps_upkeep(id)` → { or_mois, vivres_mois }.
- **Bonus/malus de doctrine au REPOS absents** : `battle_info` expose terrain/contres/balance UNIQUEMENT pendant un affrontement ; hors combat il n'existe aucun reader « stats de combat » d'un corps (doctrine d'armée weapon/moral/firearm_power, modificateurs de paie…). Le panneau montre la compo + le journal de campagne hors combat, et les facteurs tactiques seulement en combat. Reader souhaitable : `scps_corps_doctrine(id)` (mots + ×100).
- **Pertes finales par CORPS** : le fil ne porte que le total nous/ennemi de la bataille (packé) ; aucune ventilation par corps engagé. Affiché tel quel (« Nos pertes / Pertes ennemies »).
- La capture « bataille de choc vivante » (in_battle=1, cohésions + lecture tactique) n'a PAS été obtenue en probe : le monde seed 42 an ~90 a produit un SIÈGE sans sortie défensive (l'ennemi n'a pas de corps de campagne à cet endroit) — la branche bataille est codée au miroir de battle_panel.gd (mêmes clés) et la branche siège + conclusion sont PROUVÉES par PNG ; à revérifier en jeu réel à la première vraie bataille.
- battle_panel.gd (clic sur le jeton carte) fait désormais doublon PARTIEL de présentation avec la section combat d'army_panel — convergence possible quand le propriétaire de battle_panel repassera dessus (non touché ici, hors périmètre).

### DA parchemin unifiée (2026-07-14, agent interrompu — repris/validé par l'orchestrateur)
- **Découverte** : TOUT le chrome sombre sortait de 2 fichiers — godot/project/ui/vkit.gd (COL_* consommées par armée/tech/construction/rail/topbar) + ui_theme.gd. Re-skin aux valeurs EXACTES de parch_theme.gd, sémantique conservée (COL_PARCH=texte devient ENCRE, COL_GOLD=accent devient or fané #7a5c22).
- **Piège** : l'agent est mort en pleine boucle d'itération (captures _zoom_*) — l'orchestrateur a re-passé les 4 probes (armée/tech/construction/journal) : tous parchemin, lisibles, 0 SCRIPT ERROR. Un agent tué ≠ travail perdu : le tree portait un état cohérent.
- **Restes** : ports STRUCTURELS (conteneurs natifs + squelette onglets) armée/construction/tech encore à faire — le re-skin n'est que la couleur.

### PANNEAU ARMÉE — PORT STRUCTUREL au squelette unifié (2026-07-14)
**Découvertes** :
- `army_panel.gd` passe de `extends Control` (wrapper plein-écran mouse_filter IGNORE + `_panel` PanelContainer positionné à la main) à `extends PanelContainer` DIRECT — le même patron que province_panel_v2/empire_window (racine = le panneau lui-même, `theme = ParchTheme.build()`, `position` auto-calculée). Deux onglets (ButtonGroup "Tab") : Composition (barre d'unités PopBar.proportion_bar réutilisée — DRY —, ligne de campagne, actions Lever/Renforcer/Piller/Scinder/Fusionner/Dissoudre) et Combat (vide « Aucun engagement » hors combat, temps réel identique à l'ancienne section inline, résultat figé à la conclusion).
- **Bascule AUTO sur l'onglet Combat** : `_refresh_combat_state` détecte la transition « pas de combat vivant → combat vivant » (`was_live` avant/après) et force `_tab=1` — sans ça un joueur qui regarde Composition raterait le siège qui s'allume (le panneau se rafraîchit sur Sim.ticked, chaque jour, sans notification visuelle sinon).
- **Le drag-band (main.gd:452, DRAG_HEADER_H=40) était FAUSSÉ par l'ancien wrapper** : `army_panel` (Control plein-écran, position (0,0) via FULL_RECT) était le nœud ajouté au groupe "draggable" (main.gd:433-435, liste externe — army_panel n'appelle PAS `add_to_group` lui-même, contrairement à province_panel_v2/empire_window qui le font dans leur propre `_ready()`) → la bande de drag valide correspondait à l'écran y∈[0,40] (le HAUT de l'écran), PAS au bandeau visible du panneau (ancré en bas). En passant `army_panel` en PanelContainer auto-positionné, `p.position.y` devient la VRAIE position du panneau → le drag-band tombe enfin sur le HeaderStrip visible. Amélioration de bord, pas demandée, zéro risque (main.gd inchangé, le mécanisme de groupe fonctionne identiquement qu'on s'y ajoute en interne ou en externe).
- `tests/army_panel_test.gd` instancie `ArmyPanel.new()` SANS `add_child` (jamais `_ready()`, donc jamais `_build_shell()`/theme) et appelle directement 5 fonctions PURES (`_corps_status_text`, `_move_preview_text`, `_stack_summary_text`, `_split_packets`, `_refill_summary_text`) avec les MÊMES signatures qu'avant (dont `_stack_summary_text(corps_data: Array[Dictionary], regions: Array[int], total: int)`, typée) — tout renommage/retypage de ces 5 aurait cassé le test silencieusement (le test ne tourne dans AUCUN `make`, seulement via probe Godot dédiée). Vérifié après le port : `army_panel_test: OK`.
- Extraire une valeur typée `Array[Dictionary]`/`Array[int]` d'une `Dictionary` (mon `_gather_corps` retourne un Dictionary agrégeant l'état) et la repasser à une fonction paramètre `Array[Dictionary]`/`Array[int]` MARCHE (le typage runtime de l'array survit au passage par une Variant Dictionary) — mais je type explicitement à l'extraction (`var corps_data: Array[Dictionary] = data.corps_data`) plutôt que `Array` nu, par prudence : le scénario multi-corps (stack/fusion, `_stack_summary_text` avec 2+ éléments) n'a PAS pu être exercé par le probe (le levier `player_raise_corps(packets, capr)` du script de capture consolide TOUJOURS en UN SEUL corps — `corps_ids=[157]` même avec packets=3).

**Pièges** :
- `ParchTheme`/`VKit` sont déjà 1-pour-1 alignées (commit a7c9945 a re-skinné vkit.gd sur les valeurs EXACTES de parch_theme.gd) : `VKit.sense(tone)` à `tone=0.5` retourne EXACTEMENT `ParchTheme.TAB_UNDERLINE`/`COL_GOLD` — pas besoin d'inventer un mapping de couleurs, `VKit.sense()`/`VKit.SLICE_PAL` restent valides tels quels dans le squelette conteneurs-natifs.
- **Theme override potentiel non prouvé** : main.gd:427-429 réassigne `c.theme = get_window().theme` (UiTheme, qui NE définit AUCUNE des variations Title/RowDim/RowLabel/HeaderStrip/LedTabStrip/Tab/Section/Body) pour CHAQUE enfant direct de `ui` — ce qui inclut army_panel ET province_panel_v2 ET empire_window (tous `ui.add_child(...)`, tous auto-assignent `theme = ParchTheme.build()` dans LEUR `_ready()` AVANT ce loop). Si ce loop s'exécute réellement après leur `_ready()` (ordre linéaire du bloc main.gd), les 3 panneaux perdraient leurs variations de thème EN JEU RÉEL — mais AUCUN des probes existants (army_panel_shot.gd, province_shot.gd, tech_shot.gd) ne lance Main.tscn : ils instancient le panneau seul, donc CE loop n'est JAMAIS exercé par la vérification standard. Non vérifié en jeu réel, non corrigé (main.gd hors périmètre) — signalé pour un futur audit du chrome de thème en contexte Main.tscn complet.

**Restes** :
- Le scénario multi-corps (pile/fusion, `_stack_summary_text` avec 2+ éléments, `merge_ok`/`split_ok` désactivés par co-location) n'a jamais été capturé en écran — seul le chemin 1-corps a été vérifié visuellement. Le code est un port direct de l'ancien (logique inchangée, testée par `army_panel_test.gd`), mais la MISE EN PAGE de la ligne "Stack ·" (couleur HEADER_INK) n'a pas d'écran de référence.
- Theme-override potentiel ci-dessus : à vérifier via une capture qui lance Main.tscn (ou un probe dédié) plutôt que le panneau seul.

### CONSTRUCTION + TECH — port structurel / chrome parchemin (2026-07-14)
**Découvertes** :
- `construction_panel.gd` porté de `extends Control` immediate-mode (hit-zones `_hover_zones`/`_click_zones`, scroll manuel `_scrolloff`) au squelette PanelContainer + ParchTheme + HeaderStrip + LedTabStrip + ScrollContainer natif. Les surfaces PUBLIQUES à préserver (grep avant port) : `open_on(tab)` + `target_pid` (main.gd:305-307 et province_shot.gd:143-148), le signal `build_requested(kind, type)` (émis, personne ne l'écoute aujourd'hui — gardé tel quel), et `_build_info_card(b, legal)` que `tests/build_info_card_test.gd` appelle SUR UNE INSTANCE JAMAIS add_child-ée (donc jamais `_ready()`) : la fonction doit rester pure (aucune dépendance à `_body`/`_scroll`), c'est le cas.
- Le dossier hover par CARTE dans un panneau conteneurs-natifs : l'ancien monolithe exposait `get_info_card(at_position)` au grain panneau (hit-test manuel) ; le TooltipServer appelle `get_info_card` sur LE CONTROL SURVOLÉ (gui_get_hovered_control) — une nested class `InfoCard extends PanelContainer` portant `card_data` + `get_info_card()` suffit, à condition que les ENFANTS de la carte soient `MOUSE_FILTER_IGNORE` (sinon le hovered control est le Label intérieur, sans méthode, et le dossier ne s'ouvre jamais).
- `_fit_scroll()` : `_body.get_combined_minimum_size()` mesuré la frame même de l'ajout des cartes REND LA HAUTEUR D'AVANT (les queue_free de l'ancien contenu + le layout des nouvelles cartes ne sont résolus qu'à la frame suivante) → l'onglet Manufactures (3 cartes) gardait la hauteur de l'onglet Édifices (7 cartes) : panneau aux 2/3 vide. Deux `await process_frame` + jeton de génération `_fit_gen` (une bascule d'onglet rapide invaliderait la mesure en vol) règlent le hug.
- tech_panel : le chrome ParchTheme dans un `_draw()` immediate-mode = 2 StyleBoxFlat statiques cachées (`ParchTheme.sb(...)` par frame allouerait) + `draw_style_box` — le reste du dessin (couloirs, cartes, métab) inchangé. Le titre au serif (`VKit.text_map` outline 0, encre HEADER_INK) est ~60 % plus LARGE que l'Alegreya : la jauge « Recherche : » posée à x=220 passait SOUS le titre → x=380.
- province_shot.gd pose `construct.theme = ui_theme.build()` AVANT add_child : sans conséquence, `_ready()` (déclenché par add_child) réassigne `theme = ParchTheme.build()` par-dessus — l'ordre probe est sûr pour tout panneau du squelette unifié.

**Pièges** :
- Le même theme-override potentiel main.gd:427 (boucle `c.theme = get_window().theme` sur les enfants de `ui`, cf. l'entrée du panneau armée ci-dessus) s'applique désormais AUSSI à construction_panel — non vérifiable sans lancer Main.tscn, main.gd hors périmètre, comportement identique aux panneaux de référence (province_panel_v2/empire_window logés pareil).
- L'ancien panneau routait aussi un kind "recruit" (`player_recruit`) dans `_act` — mort depuis que la levée vit dans army_panel : PAS reporté (aucun appelant, grep vide). Si un jour un panneau re-enfile la levée, ne pas la remettre ici.

**Restes** :
- Le flash « ordre émis » n'est plus effacé par minuteur (il persiste jusqu'à la prochaine action/refresh) — comportement de l'ancien panneau conservé, mais un fade à la province_panel_v2 (`_fire`, timer 1,8 s) serait plus cohérent avec la fiche.
- tech_panel garde tout son contenu interne en VKit immediate-mode (cartes, couloirs, métab, dossier) — le port CHROME est fait, un futur port STRUCTUREL (cartes en Control natifs stylés ParchTheme) reste possible mais la mission l'excluait explicitement.

---

## RÉPARATION BANCS agency_demo + ai_demo (2026-07-14, fixtures — moteur intact)

**Découvertes** :
- Les 2 bancs rouges (agency_demo 14/16, ai_demo 25/26) n'étaient PAS des héritiers du RE-KEY PROVINCE (de25550) : bisect en worktree (1fdb80d VERT → 2cbd6fc=de25550^ déjà ROUGE, ~8 builds) → l'introducteur est **d07fa3b « 2 brutes/tuile strictes + spawn curé » (2026-07-13, la veille)**. Ses gates étaient golden/determinism/savetest — la suite complète n'a pas tourné (même angle mort que S0 le lendemain : chacun a hérité du rouge de l'autre vague).
- agency_demo §1 (matière) : la fixture dotait la SEULE vue `region[].stock` ; la consommation du chantier (`intertrade_market_consume` → `centre_take` → `econ_region_stock_add`) débite `prov[]` et ne décrémente la vue que du PRÉLEVÉ RÉEL. Sous 2-brutes strictes, l'empire de la graine 42 n'a plus une miette de pierre en propre → « pierre mangée = 0 » (le gate d'admission, lui, lisait la vue dotée → chantier admis, débit fantôme). Fix fixture : doter AUSSI la province porteuse (`econ_region_rep_province`) ; la pénurie draine les provinces (`econ_region_stock_add(…,-1e9)`) en plus de la vue.
- ai_demo « le Bâtisseur métabolise AU MOINS AUTANT » : le TIRAGE de départ décide quelles manufactures civiles sont NOURRISSABLES (gate `raw_cap[in1]`, `ai_build_civmanuf`) — graine 9 : le Dominateur tirait du cuivre (p61, 12/9) → Comptoir d'artisan (cuivre+sel) posable ; le Bâtisseur (fruit/charbon + bois/grain) non → 6 vs 5, un slot de métabolisme d'écart venu du SOL, pas de la fiche (AIDIAG : 100 % des builds_other des deux = ai_build_civmanuf, types identiques au Comptoir près). Fix fixture : le bloc « SUBSTRAT ÉGAL » égalise désormais AUSSI la main de tirage (par REMPLACEMENT, jamais >2 brutes/tuile) : capitale = SA food de biome + bois ; autres provinces de la région = cuivre+grain. Résultat : Bâtisseur 7 ≥ Dominateur 6, 26/26 — aucune assertion affaiblie.

**Pièges** :
- `raw_cap` d'une province est FIGÉ à econ_init (coupe vocation, scps_econ.c ~1666-1680) : une fixture peut l'écrire après init, rien ne le re-coupe en cours de run — re-poser aussi `w->province[].resource/resource2` pour la cohérence des lecteurs de tirage.
- `AiStats.builds_other` agrège 8 sites (manuf/paybuild/civmanuf/exploit/grenier-faim/civedi-digest/entrepot/marché, scps_ai.c) : pour attribuer un tally, logguer PAR SITE — un écart de 1 est illisible autrement.
- Bisect Windows/worktree : `git clean -fd` ne purge PAS `build/*.o` (ignorés) → binaires FRANKENSTEIN inter-commits (undefined reference country_knows sur un commit intermédiaire où le Makefile ne liait pas encore scps_fog). Toujours `git clean -fdx build` avant chaque step.

**Restes** :
- intertrade_demo BUILD ÉCHEC = le pré-existant Windows (setenv), inchangé.

### CHANTIER MONNAIE — M0 : L'AUDIT création/destruction (2026-07-14)
**Découvertes** (docs/MONNAIE_M0_AUDIT.md porte le registre complet — ceci
n'est QUE ce qui a coûté cher à trouver) :
- **Deux fonctions homonymes de « transfert » avec des philosophies opposées** :
  `econ_region_treasury_add` (scps_econ.c:2294) est TOUJOURS un vrai routeur
  province-safe (débit/crédit réel), mais un appelant peut très bien s'en
  servir pour CRÉER (un seul côté de l'appel, l'autre compte n'est jamais
  débité — cf. le tribut « mûri » des vassaux, scps_diplo.c:396, qui crédite
  le suzerain via cette fonction sans qu'aucun débit vassal n'existe nulle
  part dans l'appelant). Le nom de la fonction ne garantit RIEN sur la
  conservation — c'est l'APPELANT qu'il faut lire des deux côtés, jamais
  s'arrêter à « ça route bien sur la province ».
- **Deux chemins de construction civile, deux philosophies** : le chantier
  d'ÉDIFICE joueur (`agency_build_acct`, scps_agency.c:333) est un TRANSFERT
  parfaitement conservé bout-en-bout (le nu importé paie les sources
  étrangères via `intertrade_market_consume`, la marge paie la cité-état
  hôte du péage) ; la construction de MANUFACTURE (joueur ET IA,
  `CMD_BUILD_MANUF`/`CMD_MANUF_LEVEL` scps_sim.c + `ai_build_civmanuf`/
  `ai_pay_and_build`/`raw_boost` scps_ai.c) appelle `credit_spend` tout court,
  SANS jamais router par le marché — l'or disparaît intégralement. Deux
  verbes qui SE RESSEMBLENT côté joueur (« je bâtis un truc ») ont un
  comportement monétaire opposé — piège classique pour un audit rapide qui
  ne suivrait qu'un seul des deux chemins et généraliserait.
- **`scps_trade.c` (intra-empire) est un module ACTIF distinct
  d'`scps_intertrade.c` (inter-pays)** — appelé par `scps_sim.c:1204`
  (`trade_tick`) juste AVANT `intertrade_tick`, chaque jour de tick. Facile à
  manquer (le nom `scps_trade.c` sans préfixe se confond avec
  `scps_intertrade.c`) : c'est LUI qui a la fuite systématique (deux formules
  de perte de transport différentes côté vendeur/acheteur, §2.13 du registre),
  alors qu'`scps_intertrade.c` est, lui, soigneusement conservé partout
  (commentaires « CONSERVATION » explicites à chaque site).
- **`ai_speculate_tick`** (scps_ai.c:2665) : le pays spécule sur SON PROPRE
  marché avec SON PROPRE trésor (achète dans un hoard privé sous le prix
  moyen mobile, revend au-dessus) — la seule contrepartie est LUI-MÊME à deux
  instants différents. Sur un prix mean-reverting (l'IPM borné actuel), la
  stratégie est structurellement gagnante en moyenne → un site de CRÉATION
  nette lente mais quasi garanti, invisible dans un grep classique
  (`econ_region_treasury_add` y apparaît deux fois, symétriques en apparence,
  mais aux DEUX BOUTS du même compte).
- **`econ_seed_population` (scps_econ.c:1032) sert DEUX rôles** : la genèse
  (M(0), légitime) ET la colonisation en cours de partie
  (`econ_colony_day`/`colonize_from_prov`, scps_econ.c:4017/4099) — cette
  seconde famille d'appels CRÉE de la richesse à chaque fondation de colonie
  (la strate colonisée reçoit `wealth=pop×[6/2/0.5]` sans qu'aucune province
  source ne soit débitée en monnaie, seulement en POPULATION) : absent de la
  liste de sites « déjà repérés » du brief — un site de création répété tout
  au long de la partie, pas juste à l'an 0.
- **L'arbitrage des cités-états double-crédite** (scps_intertrade.c:1046-1066,
  bloc « M4 ») : la source ET le Centre importateur sont TOUS DEUX crédités
  en or pour le MÊME mouvement de stock, sans qu'aucun des deux ne soit
  débité — à ne pas confondre avec les routes régulières
  (scps_intertrade.c:970-1026), qui sont, elles, parfaitement conservées (même
  fichier, même style de commentaire « conservation », mais deux blocs très
  différents à 80 lignes d'écart).
- **Le pillage/siège monétise le STOCK sans le livrer** : la part liquide du
  butin (`pp->treasury -= gold`) est un vrai transfert, mais la part en STOCK
  (scps_diplo.c:1338-1351, 1394-1407) est retirée de la victime et VALORISÉE
  en or pour l'occupant SANS que le bien physique ne rejoigne le stock de
  l'occupant (`econ_region_stock_add` du côté occupant est absent) — un
  pillage de matières premières se change en création d'or pur, adossée à une
  destruction de matière. Facile à rater car le trésor, LUI, est bien
  conservé (`-=`/`+=` symétriques) — c'est le silence côté stock occupant
  qu'il faut remarquer.
- **Chiffrage (chronicle, print-only, `chronicle_money_mass`/
  `chronicle_money_flux_accum`, aucun effet golden/déterminisme vérifié)** :
  M(0) ~ 50-57 k or (dotation de genèse) → M(fin) 20 à 73 MILLIONS après 250
  ans SANS AUCUNE frappe (×360 à ×1280), sur 9 mondes (3 graines × 3 sims,
  `./chronicle {9,11,42} 3 250 6 12`). Le recoupement FX_* (les sites déjà
  instrumentés en `econ_flux_add` — impôt/entretien/cour/admin/redépense/
  soldes/marine/conseil/audits/chantiers/péages/intérêts/intrigues) est
  QUASI ÉQUILIBRÉ (création≈destruction à 1-2 % près) : la dérive massive
  observée n'est PAS pilotée par ces sinks (déjà à peu près calés), mais par
  le duo NON instrumenté VA-de-production (§1.1, créée) vs consommation
  (§2.1, détruite) — composés sur 250 ans, la VA (∝ PIB, croissance
  démographique+techno) distance la consommation (plafonnée au panier de
  besoins per capita).

**Pièges** :
- `econ_flux_year_capture()` (appelée chaque année dans la boucle chronicle)
  APPELLE `econ_flux_reset()` en interne — accumuler le flux `FX_*` sur TOUTE
  une sim de 250 ans exige de sommer `econ_flux_get` CHAQUE année AVANT cet
  appel (pas après la boucle : à ce moment-là il ne reste que la dernière
  année). Piège pour quiconque voudrait un recoupement flux sur la sim
  entière sans lire `econ_flux_year_capture` d'abord.
- `RegionEconomy.treasury`/`.strata[].wealth` sont des VUES recalculées
  ENTIÈREMENT par `econ_aggregate_regions` à chaque tick — sommer `region[]`
  pour M(t) au lieu de `prov[]` fonctionne EN APPARENCE (même total, la vue
  est fidèle) mais viole la doctrine province-grain du CLAUDE.md ; j'ai
  sommé `prov[].treasury` + `prov[].strata[].wealth` directement (télémétrie
  chronicle) pour rester cohérent avec la charte, même si `region[]` aurait
  donné le même nombre au tick de mesure.

**Restes / AMBIGU (non tranché, signalé pour M3 plutôt que deviné)** :
- ~~`scps/scps_diplo.c:1149-1160` (`diplo_peace_pillage_stock`)~~ RÉSOLU en
  fin de mission : la fonction livre bien le stock au vainqueur
  (`econ_region_stock_add(econ,dst,g,take)`, ligne 1160) — c'est un pur
  transfert de MATIÈRE (hors périmètre monnaie, aucun `treasury`/`wealth`
  touché). Elle contraste d'autant plus avec §2.12 (`diplo_pillage_value`/
  `diplo_siege_loot`), qui AURAIENT PU faire pareil (le motif existe à 200
  lignes de distance dans le même fichier) mais monétisent le stock pillé au
  lieu de le livrer — probablement pas une décision délibérée, plutôt deux
  fonctions écrites à des moments différents sans se relire l'une l'autre.
- La frontière DESTRUCTION/DETTE du §2.11 (intérêt annuel sans créancier
  assigné, scps_credit.c:104-108) est un cas rare (1re année de dette avant
  qu'un prêteur solvable existe) — pas mesuré séparément dans le chiffrage
  (noyé dans FX_CREDIT global) ; son poids réel n'est pas connu.
- Le partage exact entre §1.1 (VA) et §2.1 (consommation) dans la dérive
  nette n'a PAS été isolé numériquement (seul le recoupement FX_*, qui ne les
  couvre PAS, a été mesuré) — instrumenter ces deux sites spécifiquement
  (hors scope M0 : ça toucherait la sim même en ajoutant un `econ_flux_add`
  dedans, ce que l'interdiction print-only excluait) donnerait le partage
  exact ; pour M3, le SIGNE (VA > conso) suffit à justifier l'ordre des
  travaux (Cœur A avant tout).

## TRI scps_faith vs scps_religion — SURVIVANT scps_religion (2026-07-14)

**Découvertes** :
- **scps_faith était déjà auto-diagnostiqué mort par son propre en-tête** : scps_faith.c:4-10 portait depuis le 2026-07-10 « ⚠ PROTOTYPE INACTIF … n'est appelé QUE par son propre banc … aucun câblage moteur, jamais lu par sim_day/econ/ai. La religion RÉELLEMENT jouée est scps_religion.c ». Grep confirmé : `scps_faith.h` n'était inclus que par scps_faith.c lui-même et faith_demo.c — zéro appelant dans scps_sim.c/scps_ai.c/scps_econ.c/chronicle, zéro référence dans godot/src ou godot/project (le seul hit godot était une variable locale `faith_name` de province_panel.gd, sans rapport — elle lit une clé readout "faith"/"religion" déjà fournie par scps_religion, pas un symbole de scps_faith.h).
- Le commentaire scps_ai.c:1996 (« même barème que scps_faith ») ne pointait PAS vers du code vivant du module : `ai_faith_stance()` (scps_ai.c:1998-2004) porte déjà sa PROPRE copie inline du barème éthos→posture (0.36/0.30/0.26/0.20/0.14/0.10, identique à `ethos_stance_base()` de scps_faith.c) — la duplication documentée dans le commentaire existait déjà INDÉPENDAMMENT de scps_faith.c, câblée en dur dans l'IA. **Rien à migrer** : supprimer scps_faith ne prive scps_ai.c d'aucune donnée, le comportement de jeu (ai_faith_stance) est intact tel quel.
- `run_tests.sh` portait DÉJÀ un banc `religion_demo` séparé (13/13 ✓, module vivant) en plus de `faith_demo` — les deux bancs coexistaient dans BENCHES_FULL, aucun n'était un doublon de l'autre (faith_demo testait le prototype mort, religion_demo teste le module composable P1-P8).

**Supprimé** (suppression sèche, rien migré) :
- `scps/scps_faith.c`, `scps/scps_faith.h`, `scps/faith_demo.c`.
- Makefile : bloc `FAITH_DEMO_OBJS`/`faith_demo:` (ex-lignes 243-251), `faith_demo` retiré de `BENCH_BINS`.
- `tools/run_tests.sh` : `faith_demo` retiré de `BENCHES_FULL`.
- scps_ai.c:1995-1997 : commentaire reformulé (« barème propre à l'IA, non dérivé de scps_religion ») pour ne plus référencer un fichier disparu.

**Preuve de mort** :
- `make golden` : hash monde IDENTIQUE au golden commité (5 graines × 12 ans) — un module réellement mort ne peut pas bouger le hash, confirmé.
- `make determinism` : 5 graines stables, mêmes hash qu'avant.
- `make test` : 38 VERTS / 0 ROUGE / 1 BUILD ÉCHEC (intertrade_demo, pré-existant Windows, documenté ci-dessus) sur **39 bancs** (contre 40 avant : faith_demo disparu, aucun autre changement — religion_demo toujours vert 13/13).
- `make scps` : compile sans erreur (viewer, membrane intacte).

**Restes** :
- `docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md` (mentionné dans l'ancien en-tête de scps_faith.c comme trace du « double système ») n'a pas été touché — c'est un doc historique, hors périmètre de cette mission (pas de code, pas de câblage).
- Survivant unique et confirmé : **scps_religion.{c,h}** (composable P1-P8, sérialisée section RELG, seule religion jouée).

## ENTRETIEN des jobs de manufacture — le moteur l'avait, l'UI ne le montrait PAS (2026-07-14)

**Retour joueur** : « le mécanisme d'entretien des jobs de manufacture est passé à la
trappe ? » — un agent précédent (S1) avait conclu à tort que le mécanisme n'existait pas.

**Découvertes** :
- Le prélèvement RÉEL vit à `scps/scps_econ.c:3118-3163` (§ E1bis.10, dans `econ_tick`),
  DÉJÀ au grain PROVINCE (`for (int pid=0...) ProvinceEconomy *re=&e->prov[pid]`, ligne
  2780-2781 — la doctrine province-grain était déjà respectée, contrairement à ce que
  suggérait le brief de mission). Deux termes : `base_up` (édifices, tous les ticks) et
  `surcharge` (manufactures H7 + IPM sur `base_up`, seulement si `re->treasury > hof`).
- **Pourquoi S1 ne l'a pas vu** : le modèle agrège les édifices en DELTAS (`re->build`,
  une struct `ProvBuild` à 8 champs pondérés : K_inst/H_coerc/P_open/PE_infra/food_cap/
  port/faith/savoir) — PAS de registre « combien coûte CET édifice précis ». `base_up`
  lit `infra = Σ(re->build.*)`, jamais une liste d'édifices individuels. Un grep de
  `EDIFICES[e].delta` OU `re->build` sans lire jusqu'à la ligne 3139 (`base_up = infra *
  BUILD_GOLD_PER_DELTA / ENTRETIEN_DIV * 365 * dt`) laisse croire qu'aucune valeur
  par-bâtiment n'est reconstructible — FAUX : `re->edi_built` (bitmask, posé à la
  construction) donne la PRÉSENCE par édifice, et `EDIFICES[e].delta` (table statique,
  `scps_agency.c:16-101`, lue via `edifice_def()`) donne le poids ProvBuild FIXE de CE
  type — assez pour isoler la contribution d'UN SEUL édifice sans registre dédié (la
  somme pondérée est linéaire : `infra_e = delta.K_inst + delta.H_coerc×DEF_UPKEEP_MULT
  + …`, indépendante des AUTRES édifices bâtis là).
- **Piège du brief de mission** : il supposait `build_gold = prix de revient au marché
  (agency_build_gold)`. FAUX — vérifié au site de tick : `base_up` n'utilise QUE la
  constante fixe `BUILD_GOLD_PER_DELTA=35` (proxy d'audit), JAMAIS `agency_build_gold`
  (qui sert UNIQUEMENT au prix d'ACHAT ponctuel, `scps_building_roster.gold`, market-
  dépendant — deux formules DIFFÉRENTES pour deux usages différents, achat vs entretien).
  Un lecteur qui aurait suivi le brief à la lettre aurait affiché une valeur qui DÉRIVE du
  prélèvement réel (bougerait avec le marché régional) au lieu de le MIROITER exactement.
- `dt` du tick réel est TOUJOURS `1/12` en jeu (`scps_sim.c:1019`, `PROF(PB_ECON,
  econ_tick(s->econ, 1.f/12.f))` — le seul site d'appel non-banc) : `365×dt` dans
  `base_up`/`surcharge` = les jours du MOIS, donc la valeur PRÉLEVÉE au tick EST DÉJÀ
  la valeur mensuelle réelle. Aucune conversion à inventer (doctrine « jamais le calcul,
  la valeur réelle »).
- La contribution manufacture (`surcharge`) somme `mlev = Σ bld[i].level` sur TOUTE la
  province puis multiplie par `MANUF_UPKEEP_DAY×365×dt×ipmf` — encore linéaire par
  bâtiment (`level_i × C`), donc isolable SANS registre (mais avec `pe->bld[i].level`
  DÉJÀ par-bâtiment, ce n'était même pas une agrégation à défaire ici).

**Livré** (3 lecteurs purs, miroirs EXACTS, aucune touche au prélèvement) :
- `econ_edifice_upkeep_month(const ProvBuild *delta)` + `econ_manuf_upkeep_month(const
  WorldEconomy *e, float level)` + `econ_province_friche(int pid)` — `scps/scps_econ.c`
  (juste après `econ_friche_count`) / déclarés `scps_econ.h`. Signature DÉLIBÉRÉMENT
  découplée d'un pid pour l'édifice (le prélèvement réel ne pondère par AUCUN prix
  régional — un pid aurait été un mensonge d'API, pas une fidélité au moteur).
- `scps_edifice_upkeep_month(int edifice)` (PUR, sans ScpsSim — même patron que
  `scps_edifice_name`/`scps_edifice_succ`, déviation ASSUMÉE de la signature suggérée par
  le brief `(ScpsSim*, pid, edifice)` puisque pid s'est avéré non pertinent) + champ
  `ScpsEdificeDef.entretien` rempli dans `scps_building_roster` (gratuit, aucun appel
  Godot supplémentaire pour la carte édifice) ; `scps_manuf_upkeep_month(ScpsSim*, pid,
  bld)` (niveau bâti si présent, sinon niveau de naissance 5 pour le picker) ;
  `scps_province_friche(ScpsSim*, pid)` — tous dans `scps_api.{c,h}`.
- Godot : `ScpsWorld::edifice_upkeep_month`, `::manuf_upkeep_month`, `::province_friche`
  (`godot/src/scps_sim_node.{h,cpp}`, bind_method ajoutés) + `entretien` dans le
  Dictionary de `building_roster`.
- UI : `construction_panel.gd` — ligne « Entretien : ~N or/mois » visible sur CHAQUE
  carte (édifices via `b.get("entretien")`, manufactures via `manuf_upkeep_month` au
  niveau de naissance). `province_panel_v2.gd` — hover des chips manuf `· entretien ~N
  or/mois` ; alerte rouge « ⚠ En friche — entretien impayé (production ×0.6) » dans
  l'onglet Infrastructure si `province_friche(pid)==1`.

**Valeurs observées** (capture `construction_edifices.png`/`construction_manufactures.png`,
seed 42, an 90) : Tribunal ~3 or/mois, Garnison ~4 (famille défensive ×1.5, vérifié à la
main : 1.0×1.5×35/400×365/12 ≈ 3.99), Port ~5, Caravansérail ~2, Marché ~3, Grenier ~3 ;
manufactures (niveau de naissance 5, IPM ≈1.05 à l'an 90) ~8 or/mois chacune (Apothicaire/
Poterie/Atelier de sculpture). Aucune province EN FRICHE dans cette capture (le trésor
tenait) — la ligne d'alerte est codée et miroir de `g_friche[pid]`, mais pas observée en
image (aurait exigé une province délibérément surbâtie).

**Gates** : `make golden` hash IDENTIQUE (5 graines × 12 ans, lecteurs purs, prélèvement
INTACT) · `make test` 38 VERTS / 0 ROUGE / 1 BUILD ÉCHEC (intertrade_demo, pré-existant
Windows) sur 39 bancs · `scons platform=windows target=template_debug` 0 warning ·
probe `province_shot.tscn` (fenêtré, seed=42 years=90) → 6 PNG sauvés, lignes Entretien
lisibles sur les captures construction_*.

**Restes** : aucun — livrable complet. Le curseur budget (`upkeep_mult`,
`BUDGET_UPKEEP`) et le clip trésor (paiement partiel si surplus insuffisant) ne sont
PAS reflétés dans l'affichage (prix NOMINAL de la carte, comme `scps_manuf_cost` affiche
le prix d'achat plutôt que le montant réellement débité si le crédit est court) — décision
assumée, cohérente avec le reste du menu construction (aucune autre carte n'affiche de
valeur « clippée par la trésorerie du moment »).
---

## RE-KEY PROVINCE — verbes SOCIAUX + esclavage + réincorporation (2026-07-14, wt/g1-grain)

Suite de de25550 (CMD_BUILD*/CMD_ALLOC*) : les verbes joueur restants encore région-grain
(REPRESS/ASSIMILATE/PURGE, SLAVE_BUY/SLAVE_SELL, POP_TRANSFER) transférés au PID, patron S0.

**Découvertes — par verbe** :
- **CMD_REPRESS/ASSIMILATE/PURGE** : `agency_order_repress/_assimilate/_purge` (scps_agency.h/.c)
  gagnent un `prov` (dernier paramètre, -1=héritage/chemin IA inchangé, ≥0=PID direct) — EXACT
  miroir d'`agency_build_acct`. `apply_action` (scps_agency.c:651) résolvait DÉJÀ `pid` via
  `o->prov` en priorité (posé par le RE-KEY précédent pour AGY_BUILD) : REPRESS/ASSIMILATE n'ont
  demandé AUCUN changement dans `apply_action` — juste enfiler avec `prov` au lieu de `-1`.
  Seul PURGE a demandé plus : `purge_slice` (appelée à la fois par `agency_advance` — tranches
  annuelles — et par `apply_action` — dernière tranche) résolvait SON PROPRE pid en interne via
  `econ_region_rep_province` ; elle gagne un paramètre `prov_hint`, même résolution que apply_action.
  scps_sim.c : a[0] devient un PID (validé `prov[pid].owner==p`), `region` dérivée via
  `w->province[pid].region` (pour `o.region`, encore consommé par `wl->L[reg]` — la légitimité
  reste RÉGION-grain, structurel, hors périmètre) puis passée EN PLUS du PID.
- **CMD_SLAVE_SELL** : n'a PAS eu besoin de changer `intertrade_slave_sell` — la fonction
  scanne déjà TOUTES les provinces du vendeur (province-écriture RÉELLE depuis le début) ; le
  `region` n'y servait qu'à lire `owner` + créditer le trésor du Centre. Le PID côté joueur ne
  sert donc QUE la revalidation (`prov[pid].owner==p`) ; la région dérivée du pid est passée
  inchangée à la fonction (le Centre — trésor/prix — reste RÉGION-grain, brief explicite : « le
  Centre reste la contrepartie »).
- **CMD_SLAVE_BUY** : à l'inverse, `intertrade_slave_buy` DÉPOSAIT le groupe déporté sur
  `econ_region_rep_province(region)` — un vrai grain d'écriture indirect. Gagne un `prov` (dernier
  paramètre, même patron -1/≥0) qui COURT-CIRCUITE cette résolution ; le trésor/prix restent sur
  `region` (Centre, inchangé). L'IA (`ai_slave_buy_pass`) et les bancs (intertrade_demo) passent -1.
- **CMD_POP_TRANSFER** : SEUL appelant au monde de `demography_pop_transfer` (aucun chemin IA,
  confirmé par grep) → RE-KEY complet, pas de dual-mode : les 2 paramètres deviennent
  `src_prov`/`dst_prov` (PID directs), `econ_region_rep_province` disparaît ENTIÈREMENT de la
  fonction. Le seul écueil : `migration_move(..., home_reg)` attend une RÉGION (tag culturel
  « foyer » du groupe déplacé, PopGroup.home_reg — lu par la diffusion/le brassage, PAS un grain
  d'écriture économique) — remplacé par `spe->region` (le backpointer région de la province
  source, `ProvinceEconomy.region`, posé à econ_init) au lieu de l'ancien paramètre `src_region`.
- **scps_action_preview** (aperçu UI-4, hover des 3 leviers) : même conversion — `region` → `prov`
  direct, `econ_region_rep_province` retiré. C'est un lecteur PUR (aucune mutation) mais devait
  rester EXACT MIROIR des formules réelles (commentaire du fichier) donc devait cibler le MÊME pid.
- **Façade + binding + GDScript** : `scps_player_repress/_assimilate/_purge/_slave_buy/_slave_sell/
  _pop_transfer/_action_preview` (scps_api.c/.h) + D_METHOD/params C++ (godot/src/scps_sim_node.*)
  renommés `region`→`prov`. Panneaux GDScript : province_panel.gd (`_pid` était DÉJÀ calculé,
  la traduction `province_region(_pid)` était pure perte pour ces 3 verbes — retirée) ;
  province_detail.gd `_reinc_owned` listait 1 entrée PAR RÉGION (dédup) — RE-KEY PROVINCE en fait
  une entrée PAR PROVINCE possédée (le dédup perdait justement le grain qu'on restaure) ;
  sidebar_drawer.gd/v3_audit.gd avaient DÉJÀ `cap_prov` sous la main (calculé pour autre chose)
  et traduisaient inutilement en région — juste swap ; verbs_audit.gd garde `capr` (région, pour
  route/market/campaign — INCHANGÉS) EN PLUS d'un nouveau `cap_prov` pour les 3 verbes sociaux.

**Pièges** :
- `event_popup.gd` (bouton « Réprimer » d'un évènement de révolte) : l'évènement lui-même est
  RÉGION-grain À LA SOURCE (le journal de révolte, hors périmètre de cette mission — pas un verbe
  CMD_* transféré). Pas de lecteur `region→pid` exposé côté binding : résolu côté UI par un scan
  linéaire `province_region(pid)==region && owner==me` (1re province possédée trouvée) — cf.
  `_first_owned_prov_in_region`. Accepté comme compromis MINIMAL (pas de nouveau verbe/reader
  moteur, juste une boucle GDScript cliente) plutôt que de rouvrir le système d'évènements.
- `scps_api_demo.c` §esclavage/pop_transfer : `cap` venait de `scps_country_capital_region` —
  swap vers `scps_country_capital_province` (existe déjà) ; le scan « 2e région distincte » devient
  « 2e province distincte » (`pp!=cap` au lieu de comparer les régions) — plus simple qu'avant.
- Confirmer qu'un appelant est SEUL avant de retirer une indirection : `demography_pop_transfer`
  n'avait aucun chemin IA (grep confirmé) → RE-KEY complet sans dual-mode ; `intertrade_slave_buy`
  EN A un (ai_slave_buy_pass) → dual-mode obligatoire (patron agency_build_acct).
- Windows/MSYS2 : `make`/`sh` réinitialisent TMP/TEMP à `/tmp` via `/etc/profile` à CHAQUE
  sous-shell de recipe — un `export TMP=...` posé par un appelant externe (bash Git normal) est
  invisible aux enfants de `make`. `cc` (natif mingw64) ne comprend pas `/tmp` (chemin POSIX) et
  retombe sur `C:\Windows\` → « Cannot create temporary file ». Le fix qui MARCHE : lancer TOUT
  le script de gates (`export` + `make …`) comme UN SEUL appel à `/d/MSYS2/usr/bin/bash.exe
  script.sh` (un seul process MSYS, l'export survit à ses propres sous-shells) — jamais fragmenter
  en plusieurs appels Bash-tool séparés (chacun perd l'export de l'autre).

**Restes** :
- **Inventaire des chemins région-grain volontairement NON transférés** (décision joueur séparée,
  hors périmètre de cette mission) :
  - `CMD_ROUTE` (scps_sim.c:712) — un ARC entre deux RÉGIONS (graphe de routes commerciales,
    `routes_order`/`RouteNetwork`) : la route est structurellement un objet région↔région, pas
    une propriété de province ; « transférer » n'aurait pas de sens sans redéfinir tout le graphe.
  - `CMD_MARKET_BUY`/`CMD_MARKET_SELL` (scps_sim.c:718/725) — ciblent un CENTRE (hub commercial
    inter-régional, `intertrade_market_buy/_sell`, prix/stock résolus via `it_treasury`→
    `econ_region_rep_province`) : même famille structurelle que le Centre esclave (§ ci-dessus,
    « le Centre reste la contrepartie ») mais ICI la marchandise ELLE-MÊME (stock du Centre)
    n'est pas province-ownée contrairement aux esclaves (qui vivent dans `pop.groups`) — rien à
    re-clencher sur une province précise. Coût du transfert : refonte du modèle de Centre entier.
  - `CMD_MOVE_ARMY`/`CMD_CAMPAIGN`/`CMD_CORPS_RAISE`/sièges (scps_sim.c:733-758+) — le grain
    MILITAIRE (FieldArmy, campagnes, warhost) est profondément région-adressé (mouvement sur le
    graphe des régions, pas des provinces) ; aucune notion de « province de destination » n'existe
    dans campaign_order/campaign_redirect. Coût du transfert : refonte du moteur de guerre.
  - Lecteurs `region_owner`/`region_pop`/`region_tier`/`region_settle_group` (scps_api.c, binding
    godot) — agrégats politiques d'AFFICHAGE (carte région, tooltips), légitimes par charte
    (§PROVINCE_MODEL : « région = agrégat nommé »). Aucune écriture derrière.
- `it_treasury`/`econ_region_rep_province` restent vivants et LÉGITIMES dans le sous-système
  Centre/marché (intertrade) — la doctrine ne les interdit que sur un chemin d'ÉCRITURE JOUEUR
  qui devrait cibler une province précise (repress/assimilate/purge/slave_buy/pop_transfer,
  maintenant réglés) ; le marché/Centre lui-même reste une entité région-structurelle par design.

## CHANTIER MONNAIE — M1 (redevance+réserve) + M2 (frappe) (2026-07-14)

**Découvertes** :
- **La maison de la réserve** : `WorldEconomy.reserve_gold/reserve_copper[SCPS_MAX_COUNTRY]`
  (scps_econ.h, juste sous `budget_mult[][]`) — le blob ECON est un `fwrite` brut de la
  struct entière (scps_save.c:103), donc AUCUN code de sérialisation à écrire : le bump
  SAVE_VERSION 85→86 + `save_sane` (borne ≥0 et <1e12) suffisent. Le motif credit
  (`credit_save/load` séparés) n'était PAS nécessaire ici — champ de struct, pas global de module.
- **Le point fixe de la frappe** : fin d'`econ_tick`, APRÈS le bloc FX_ROADS (les curseurs
  mensuels par-pays vivent tous là, post-agrégation). La fonction PURE
  `econ_country_mint_month` (miroir exact) est partagée par le point de MUTATION (econ_tick)
  et le lecteur façade `scps_country_mint_month` — un seul calcul, jamais deux formules.
- **econ_tick n'a pas de World*** : la capitale se résout par `re->is_capital` (scan des
  provinces du pays, motif déjà présent 3× dans scps_econ.c) — `econ_country_capital_prov`,
  pas `w->country[].capital_prov`.
- **« Le joueur » côté moteur = `culture_player_cid()`** (scps_heritage, slot 0) : -1 en
  chronique (personne ne matche) ⇒ tous les pays suivent la politique IA (MINT_AI_SHARE) ;
  en partie jouée, le pays lié au slot 0 lit son curseur BUDGET_MINT (défaut 0 = golden-neutre
  pour la part joueur). L'IA n'écrit JAMAIS budget_mult[][BUDGET_MINT].
- **BUDGET_MINT suit le motif BUDGET_INVEST, pas BUDGET_ROADS** : neutre = 0 % (niveau brut
  0..1, pas le sentinel 0→1.0 des enveloppes de paie) — sinon « non réglé » aurait frappé 100 %.
- **UI presque gratuite, confirmé** : `budget_controls` (scps_sim_node.cpp:1480) est une table
  de noms + `scps_country_budget_policy` ; passer 5→6 postes suffit pour que les TROIS panneaux
  (budget_panel_v2, economy_page, sidebar_drawer) fassent naître le curseur — seul l'affichage
  du MONTANT a demandé un cas spécial (idx 5 lit `country_mint_month`, pas un poste de flux
  en valeur absolue : la frappe est un REVENU au milieu des dépenses).
- **FX_MINT appendu à FluxComp** ⇒ `g_flux` grandit ⇒ blob TXYR change de taille — couvert
  par le MÊME bump v86 (documenté scps_save.h).

**Mesures (sweep apparié OFF `SCPS_TUNE=MINT_ROYALTY=0` vs ON, `./chronicle {9,11,42} 3 250 6 12`)** :
- **Kill-switch PROUVÉ avant re-baseline** : OFF → `make golden` VERT contre le golden PRÉ-M1
  (hash byte-identique, M1 seul puis M1+M2). ON par défaut : 4/5 graines décalent (l'IA frappe
  dès l'an 0) → re-baseline commit séparé (d948b4a).
- Satisfaction Laborer OFF→ON par graine : +11 / −10 / +2 (moyenne +1 pt) — dépasse ±5 PAR
  GRAINE mais en signes OPPOSÉS = divergence chaotique de trajectoire, pas une dérive
  systématique ; Bourgeois/Élite dans ±5 partout. IPM : 0.96-1.05 final (borne 0.85-1.35 très
  large) — AUCUNE montée chez les frappeurs (l'IPM lit or/biens ; la frappe est ~0.1 % de M).
- Hégémon mortel préservé (2/3·3/3·2/3 ON vs 2/3·2/3·0/3 OFF). Chaînes cuivre VIVANTES :
  fournitures navales consommées 217k OFF → 196k ON (−9.7 %, cohérent avec 15 % de redevance).
- Réserve an 250 : or 0-7.3k · cuivre 1.5-22.6k (équilibre ≈ 6.7 ans de redevance à 15 %/an
  de frappe IA — ni nulle ni explosive) ; frappe 32-397 or/an/monde · 2-11 empires frappeurs.
- Part de M venant de la frappe ≈ 0.1 % : la planche à billets VA (§1.1 de l'audit M0) domine
  toujours — c'est M3 qui la convertira, la frappe est prête à prendre le relais.

**Gates** : kill-switch VERT pré-M1 · golden-update+determinism STABLE · make test 38 VERTS/
0 ROUGE/1 BUILD ÉCHEC (intertrade_demo setenv, pré-existant Windows) · savetest 9 A==B (v86) ·
fuzz-save 8/8 · scons DLL 0 warning · probe budget_shot : ligne « Réserve : X or · Y cuivre »
(bandeau) + curseur « Frappe » + « +N or/mois » lisibles (build/budget_v2.png).

**Restes** :
- **Le rachat du surplus marchand par la Monnaie** (M1 concept, 2e voie) : NON implémenté —
  mesuré inutile à ce stade : la redevance seule nourrit une réserve VIVANTE (cuivre 1.5-22.6k
  an 250, 2-11 frappeurs/monde). Si un futur calibrage veut plus d'OR frappable (réserve or
  fin ≈ centaines seulement — l'or est rare et cher), la 2e voie est la vague suivante désignée.
- La réserve d'un pays MORT (conquis) reste dans sa case (jamais pillée/transférée) — un pays
  sans capitale ne frappe plus (cap=-1 ⇒ 0), l'or dort. À raccorder au pillage en M6 (transport).
- L'UI ne montre la réserve que du JOUEUR (bandeau + ligne Rentrées) ; pas de vue par-pays
  étranger (espionnage/diplo) — hors périmètre M2.

## CHANTIER MONNAIE — M3a : instrument + 3 fuites de l'audit M0 (2026-07-14)

**Livré** (5 commits : instrument 8de802b · trade 4d77678 · arbitrage 8b8721d ·
colonisation 1679d09 · re-baseline e83047f) : la ligne chronicle « création
résiduelle : VA X/an · conso −Y/an · colonisation Z/an · autres W/an » + les
fixes M0 §2.13 (fuite scps_trade), §1.3 (double-crédit arbitrage), §1.2
(colonisation ex nihilo → transfert, save v87).

**Découvertes** :
- **⚠ LE CHIFFRE CLÉ, contre-intuitif : la dérive de M MONTE après les fixes**
  (moyenne 9 sims : +160.5k/an pre-m3 → +186.8k/an HEAD, +16 %). Ce n'est PAS
  un échec de conservation : la PLUS GROSSE des 3 anomalies (la fuite
  scps_trade) était classée §2.13 DESTRUCTION dans l'audit M0 lui-même — un
  trou noir accidentel qui BRÛLAIT de l'or à chaque transaction intra-empire.
  La colmater rend cet or au monde ⇒ M monte mécaniquement. Les deux CRÉATIONS
  retirées (arbitrage ~borné/tick, colonisation ~250 or/fondation ≈ 110/an/sim)
  sont d'un ordre de grandeur trop petites pour compenser. « Conservation » ≠
  « dérive plus basse » : l'attente « la dérive doit baisser » ne se réalisera
  qu'en M3b, quand VA/conso (les deux monstres) seront convertis.
- **La preuve propre par l'instrument** : cherry-pick du commit instrument
  (poids-zéro, golden prouvé identique) sur le worktree pre-m3 ⇒ trajectoires
  BYTE-IDENTIQUES à pre-m3 nu (lignes « masse monétaire » diff-vides sur les
  9 sims) — on peut donc comparer les CATÉGORIES avant/après fixes : « autres »
  (tout sauf VA/conso/colonisation) passe de −47.0k/an (pre, moyenne 9 sims) à
  −38.1k/an (post) : +8.9k/an, la signature de la fuite trade colmatée (les
  créations retirées tirent dans l'autre sens, plus faiblement). VA mesurée
  +105k à +402k/an · conso −17k à −63k/an : le ratio ~6:1 VA/conso est LE
  déséquilibre que M3b doit fermer — la conso ne détruit qu'un sixième de ce
  que la VA imprime.
- **L'intention du module scps_trade tranchée** : sa perte de transport est
  PHYSIQUE (fret perdu en route, `received=vol×(1−loss)` existait déjà côté
  volume) et il n'a AUCUN percepteur possible (pas de route/caravanier/péage
  dans ce module, contrairement à intertrade). Fix minimal : le prix vendeur
  porte la MÊME perte que le volume (`1−loss` au lieu de `1−transport_cost`)
  ⇒ revenue == cost_imp exactement. L'alternative « créditer la perte à un
  acteur » aurait exigé d'inventer un détenteur de route intra-empire — hors
  périmètre, non choisie.
- **Le vrai vendeur de l'arbitrage = la SOURCE** (elle encaisse SON prix local
  sp, une fois) ; le Centre est le vrai ACHETEUR (il paie vol×sp de son trésor,
  borné à son or — motif du bloc routes 80 lignes plus haut). Sa marge devient
  IMPLICITE : le stock acheté à sp vaut lp sur son marché — il marge à la
  revente, il ne double pas. IT_MARGIN_TO_GOLD et ARB_CAPTURE supprimés (plus
  aucun lecteur — ARB_CAPTURE retiré aussi de scps_tune_list.h, sinon tune_init
  liste un tunable mort).
- **La colonisation a DEUX voies asymétriques** : le convoi joueur
  (econ_colonize_province → econ_colony_day, ponction à l'ORDRE, fondation
  jusqu'à 3 ans plus tard) exige que la richesse VOYAGE — d'où
  `ColonyWork.seed_wealth[CLASS_COUNT]` (struct sérialisée ⇒ SAVE_VERSION
  86→87, save_sane borne [0,1e7], blob ECON fwrite brut = aucun code de
  sérialisation à écrire, motif reserve_gold v86). La voie immédiate
  (colonize_from_prov, IA + outre-mer) prélève et livre dans le même appel.
  Colons perdus en route (cible prise entre-temps) = richesse perdue comme la
  pop — destruction assumée, comptée par l'instrument (catégorie colonisation).
- **Gate anti-gel VÉRIFIÉ sans calibrage** : fondations moy./sim 110→104 (s9),
  118→128 (s11), 108→132 (s42) — l'expansion VIT à COLONY_WEALTH_SHARE=1
  (plein prélèvement ∝pop). Le tunable existe si un monde futur gèle.

**Pièges** :
- `git worktree add <tag>` + build MSYS2 : le worktree du tag compile dans SON
  dossier avec SON Makefile — mais `git clean`/`checkout` ne purge pas
  `build/*.o` (ignorés) : partir d'un worktree NEUF (pas d'un checkout recyclé)
  évite les binaires frankenstein (même piège que le bisect du 14).
- Le tag `pre-m3` pointe 05963fc, PAS 0f20639 (« + tag pre-m3 » dans le message
  de 0f20639 est trompeur — le commit doc est APRÈS le tag) : sim-identiques
  (0f20639 = doc seul), mais un `git diff pre-m3` inclut le doc.
- L'ordre des commits a un coût : l'instrument DOIT être committé avant les
  fixes pour que « golden identique » le prouve seul — mais les hooks
  colonisation de l'instrument (g_colonization_net_cum -=/+=) n'existent
  qu'AVEC le fix : le compteur est déclaré/committé avec l'instrument (lit 0),
  câblé par le fix. Sinon le commit instrument ne compile pas seul.

**Sweep apparié (pre-m3 vs HEAD, `./chronicle {9,11,42} 3 250 6 12`)** :
- Dérive de M par graine (moy. 3 sims, or/an) : 132.5k→146.2k · 189.6k→231.6k ·
  159.3k→182.6k (voir LE CHIFFRE CLÉ ci-dessus pour la lecture).
- Satisfaction Laborer par graine : −10/+13/+1 (signes opposés = chaos, motif
  M1/M2) ; Bourgeois +1/+6/+1 · Élite −2/+10/−1. IPM final 0.89-1.09 (borné).
- Hégémon mortel : 2/3·3/3·2/3 pre → 1/3·2/3·1/3 post (préservé partout).
- Catégorie colonisation : EXACTEMENT 0/an sur les 9 sims post (transfert pur
  prouvé par la mesure, pas par la lecture du code).

**Gates** : golden IDENTIQUE instrument seul (commit séparé) puis re-baseline
documentée post-fixes · determinism STABLE · make test 38 VERTS/0 ROUGE/1 BUILD
ÉCHEC (intertrade_demo setenv, pré-existant Windows) · savetest 9 A==B (v87) ·
fuzz-save 8/8 (216 octets flippés rejetés).

**Restes** :
- **M3b (la vague suivante, PAS commencée ici)** : convertir VA (§1.1) en
  ventes payées par le compte de marché et la conso (§2.1) en crédit vendeur —
  le tableau de bord « création résiduelle » est prêt, VA/conso doivent fondre
  vers 0 et la dérive de M avec.
- Le trésor du Centre borne désormais l'arbitrage (`*h_tr<=0 ⇒ pas d'achat`) :
  un Centre ruiné n'arbitre plus — voulu (conservation), mais si un calibrage
  futur trouve les cités-états trop pauvres pour leur « moteur », c'est ici.
- scps_api_demo/les bancs ne testent PAS seed_wealth explicitement (couvert
  indirectement par save_io_demo/savetest v87) — un banc dédié « colonie =
  transfert » serait du luxe pour M3b.
- Godot DLL non re-buildée (scons) : scps_econ.h a changé (struct ColonyWork)
  — à re-builder avant la prochaine session de jeu (gates M3a n'incluaient pas
  scons ; le moteur C est la vérité).

## CHANTIER MONNAIE — M3b : le flip domestique, STOPPÉ après sweep (monde effondré, 2026-07-14)

**Statut : NON LIVRÉ.** Le mécanisme (Cœur A, compte de marché) est ÉCRIT et
COMPILE, mais le sweep révèle un monde qui s'effondre (Laborer/Élite → 0 % de
satisfaction dès l'an 5, pas de reprise à l'an 100) malgré DEUX calibrages
tentés (les deux leviers explicitement autorisés par le brief). Décision :
**STOP, ne pas re-baseliner, ne pas forcer** (clause de sortie du brief). Le
code reste NON COMMITÉ (stash `monnaie-m3b-flip-non-calibre`, à récupérer avec
`git stash list` / `git stash show -p`) ; seule cette entrée TROUVAILLES est
committée — elle documente le mécanisme et le diagnostic pour la prochaine
tentative.

**Le mécanisme implémenté** (scps_econ.c, scps_econ.h, scps_save.{c,h},
scps_tune_list.h) :
- `WorldEconomy.market_account[SCPS_MAX_COUNTRY]` (le compte de marché,
  alimenté par la conso) + `va_country_prev[SCPS_MAX_COUNTRY]` (VA nationale
  totale du tick PRÉCÉDENT, dénominateur des parts provinciales) — persistés,
  SAVE_VERSION 88 (non committé, donc jamais publié).
- **Ordre de tick vérifié** (la question posée par le brief) : dans
  `econ_tick` (scps_econ.c:2775), la boucle `for (pid...)` traite CHAQUE
  province de bout en bout (production §1→3, taxe, entretien/cour/admin/
  redépense, PUIS demande/conso §4→6) AVANT de passer à la province suivante
  — la production d'une province précède TOUJOURS sa propre consommation, DANS
  LE MÊME tick. Donc « conso AVANT distribution » est FAUX dans ce moteur :
  l'intra-tick pur (conso alimente la distribution du MÊME mois) est
  architecturalement impossible sans scinder la boucle en deux passes
  complètes sur TOUTES les provinces (production seule, puis taxe+conso) — un
  refactor bien plus lourd que le brief n'anticipait, écarté (KISS, hors
  scope). **Persisté avec un lag d'1 tick a donc été choisi** (même motif que
  le prix national, scps_econ.c:3717 : soldé une fois par pays, projeté pour
  le tick SUIVANT) — cohérent avec l'idiome existant du fichier.
- **Ordre-indépendance** (piège évité) : une première version aurait décrémenté
  `market_account` PROVINCE PAR PROVINCE pendant la boucle — un biais d'ordre
  pur (les provinces de petit pid siphonneraient les suivantes du même pays,
  aucun signal économique). Fix : `mkt_snapshot[]` = photo FIXE en tête de
  tick (scps_econ.c juste avant `for (int pid=0...)`), chaque province calcule
  sa part `share = va_prov/va_country_prev[owner]` depuis cette photo qui ne
  bouge PAS pendant la boucle ; les tirages (`mkt_drawn[]`) ne sont appliqués
  au compte RÉEL qu'à la CLÔTURE (après la boucle complète, scps_econ.c
  ~3780) — un seul point d'écriture par pays, jamais N.
- Site conso (§2.1, ex-ligne 3513) : le débit `budget0−wealth` crédite
  désormais `e->market_account[owner]` (province rattachée à un pays) au lieu
  du compteur instrument (qui ne mesure plus que les provinces ISOLÉES,
  owner<0, hors périmètre M3b — fixtures/bancs).
- Site production (§1.1, ex-ligne 3205) : `wage_pool/profit_pool/tax_pool` ne
  créditent plus la richesse en entier — SEULE la fraction financée par le
  compte (`revenue`, plafonnée à `va_prov`, jamais de sur-crédit) est créditée
  ; le solde non financé alimente `g_va_produced_cum` (l'instrument, qui
  mesure désormais la création RÉSIDUELLE, pas la VA totale).
- Tunable `MKT_SMOOTH_MONTHS` (scps_tune_list.h, défaut 3) : le compte ne paie
  qu'1/N de sa balance par mois (lissage anti-étranglement, `SCPS_TUNE=
  MKT_SMOOTH_MONTHS=X` sans recompiler).

**Découvertes (le diagnostic qui a coûté cher)** :
- **Le ratio VA:conso ~6:1 documenté par M3a (TROUVAILLES ligne 712) n'est PAS
  une anomalie de mesure — c'est un plafond STRUCTUREL de la conso** : la
  demande de subsistance/confort/luxe est BORNÉE par un panier per-capita
  (NEED[classe][ressource]×pop), donc SATIABLE — dans l'ancien régime (VA
  créée sans limite), les ménages étaient toujours assez riches pour acheter
  100 % de leur panier (budget jamais contraignant), donc conso ≈ coût du
  panier physique, une quantité relativement STABLE, tandis que VA (le
  revenu) grossissait sans plafond avec la population/le PIB composé sur 250
  ans. Convertir VA en « clé de répartition d'un compte alimenté par conso »
  revient donc, en régime permanent, à plafonner le revenu RÉEL total autour
  de ~1/6 de son niveau d'avant — une réduction voulue par le contrat
  (« VA résiduelle ≈ 0 »), mais brutale pour les classes qui n'ont pas de
  coussin.
- **Confirmé par sweep apparié 1 sim/seed 9, 6 empires/12 cités, `SCPS_MKTDIAG`
  activé (stderr, scps_econ.c ~3781, print-only)** : le compte national
  touche EXACTEMENT 0 plusieurs mois d'affilée dès les premières années (ex.
  ticks 10-12 et 19-24 d'un même run) — ces mois-là, Laborer touche un salaire
  NUL (pas réduit : nul), et la taxe per-capita FORFAITAIRE (TAX_BASE_LABORER,
  scps_tune_list.h — hors scope M3b, INTERDIT d'y toucher) rase ce qu'il en
  reste (`collected=min(tax_base×pop×mult, wealth)` — clampée à zéro).
  Résultat : richesse Laborer piégée à ~0, un ÉTAT ABSORBANT (rien ne la fait
  remonter : 0 richesse ⇒ 0 conso ⇒ 0 crédit au compte de marché ⇒ 0 revenu au
  tick suivant).
- **Comparaison directe pré/post-flip, MÊME seed/monde (9, 1 sim, 6 emp/12
  cités, 100 ans)** : pré-M3b (HEAD=pre-m3b) → Laborer 47 % · Bourgeois 74 % ·
  Élite 75 % (sain, dans les bandes ou proche) ; post-flip (parts uniformes
  42/20/38 appliquées à `revenue/va_prov`) → Laborer 0 % · Bourgeois 61 % ·
  Élite 58 % à l'an 15 (déjà cassé pour Laborer, le reste tenable) mais
  Laborer RESTE à 0-3 % jusqu'à l'an 100 (pas de reprise). La collapse arrive
  vite (an 5 : Laborer déjà 0 %, Bourgeois 20 %, Élite 1 %) et ne guérit PAS
  avec le temps — ce n'est pas un régime transitoire lent, c'est un piège.
- **Deux calibrages tentés, les deux AUTORISÉS par le brief, AUCUN ne sauve le
  monde** :
  1. *Parts en cascade (« qui paie en premier quand le compte manque »)* :
     essayé salaire→profit→rente (protéger la subsistance d'abord, motif
     « salaire collant ») — Laborer remonte à peine (0→3 %) mais ÉLITE
     s'effondre à son tour (58→0 %, puisqu'elle passe désormais TOUJOURS en
     dernier). Les parts UNIFORMES (ratio identique aux 3 pools) protègent
     mieux Bourgeois/Élite (58-61 %) mais laissent Laborer à 0 % — aucun
     ordre de priorité ne peut satisfaire les TROIS classes avec une
     enveloppe qui ne couvre, en régime permanent, qu'une fraction du
     besoin total.
  2. *Lissage temporel (`MKT_SMOOTH_MONTHS` 1/3/12/24/60, sweep par
     `SCPS_TUNE` sans recompiler)* : AUCUN effet notable sur la satisfaction
     ni sur la création résiduelle mesurée (~38-42k or/an dans TOUS les cas)
     — mathématiquement attendu : lisser une balance dans le temps change sa
     VARIANCE tick-à-tick, jamais sa MOYENNE de long terme (Σ tirages ≈ Σ
     apports, quel que soit N). Le brief anticipait ce levier pour un
     étranglement de VOLATILITÉ (ex. un compte qui touche 0 par malchance
     ponctuelle) — le problème mesuré ici est un déficit de MOYENNE
     (structurel), que le lissage ne peut PAS corriger par construction.
- **La théorie de la sortie par les prix ne s'est pas vérifiée en pratique** :
  intuition avant sweep — moins d'argent en circulation devrait faire BAISSER
  les prix (`re->price` suit `demand/(stock+offre)`, planché à 0.15×BASE_PRICE
  — comme par hasard proche de 1/6.67, presque le ratio VA:conso observé), ce
  qui permettrait à un revenu ~6× plus petit d'acheter le MÊME panier réel.
  EN PRATIQUE, l'an 5-30 ne montre AUCUNE reprise (Laborer coincé à 0-2 %) —
  soit `PRICE_INERTIA` est trop lent pour que la déflation compense avant que
  la taxe ne rase la richesse à zéro (état absorbant, cf. ci-dessus), soit un
  autre mécanisme bloque l'ajustement ; NON INVESTIGUÉ plus avant (aurait
  exigé de toucher au prix/l'IPM, explicitement HORS PÉRIMÈTRE M3b).

**Pièges** :
- Le motif « photo fixe en tête de boucle + accumulateur local + application
  centralisée à la clôture » (ordre-indépendance) marche bien ICI mais est
  FACILE à rater : une première rédaction (décrément `market_account` dans la
  boucle, province par province) compile et TOURNE sans erreur — le bug est
  un biais STATISTIQUE silencieux (qui siphonne qui selon `pid`), invisible
  sans sweep dédié comparant l'ordre des provinces.
- `git stash` puis rebuild : le binaire `chronicle.exe` compilé depuis le
  stash (pre-m3b) écrase celui du flip dans le même répertoire — bien COPIER
  (`cp chronicle.exe /tmp/chronicle_prem3b.exe`) avant `git stash pop` +
  rebuild, sinon la comparaison A/B se fait par erreur contre le MÊME binaire.
- Build Windows : `make` seul (Bash tool, PATH par défaut) échoue SILENCIEUSEMENT
  (`Cannot create temporary file in C:\Windows\`) même avec `TMP`/`TEMP` Windows
  exportés — la mémoire scps-build-windows.md a la bonne incantation
  (`TMP=/tmp TEMP=/tmp TMPDIR=/tmp`, chemins POSIX, PAS des chemins Windows)
  DANS un script `.sh` lancé via `MSYSTEM=MINGW64 /d/MSYS2/usr/bin/bash.exe -l
  script.sh` (le `cd` inline se fait tronquer si passé en `-lc` direct sur une
  commande longue — un fichier `.sh` est fiable, pas une ligne `-lc`).

**Restes (pour la prochaine tentative M3b)** :
- **Le sweep apparié complet (9 mondes × 250 ans), `make golden-update`,
  `make test`, `make determinism`, `--savetest`/`--fuzztest` : AUCUN n'a été
  lancé** — inutile de mesurer un monde déjà cassé à l'an 5 sur un run court.
  À refaire en ENTIER une fois un mécanisme viable trouvé.
- **Pistes NON essayées, à trancher par l'orchestrateur (dépassent le
  périmètre « flip seul » du brief actuel)** :
  - Un PLANCHER de revenu minimal (subsistance) financé autrement que par le
    compte de marché pur — mais tout plancher non adossé à un vendeur réel
    est, par définition, une re-création (contredit le contrat M3).
  - Rendre l'impôt per-capita SENSIBLE au revenu réel du tick (proportionnel,
    pas forfaitaire) le temps de la transition — EXPLICITEMENT interdit par
    CE brief (« INTERDITS : toucher... à l'impôt per-capita ») ; à
    re-proposer à l'orchestrateur comme un prérequis M3b plutôt qu'un
    aparté.
  - Revoir l'élasticité de la demande (laisser le panier per-capita croître
    avec la richesse disponible, pas seulement la population) pour que conso
    puisse RATTRAPER VA en régime permanent — un chantier de démographie/
    demande, pas un chantier monétaire pur.
  - Réexaminer si Cœur A (VA-du-tick comme clé de répartition d'un compte
    conso) est la bonne architecture, ou si une propriété-par-classe
    (l'alternative écartée dans docs/MONNAIE_CONCEPT.md §M3) éviterait le
    couplage direct « conso plafonnée ⇒ revenu plafonné » qui a fait
    s'effondrer ce sweep.
- Le code du mécanisme (compte de marché, ordre-indépendance, instrument
  résiduel) reste RÉUTILISABLE tel quel pour la prochaine tentative — c'est le
  CALIBRAGE (parts/lissage/taxe/demande) qu'il faut résoudre, pas la
  plomberie. `SCPS_MKTDIAG=1` (stderr, print-only) reste câblé pour
  ré-instrumenter vite.

## CHANTIER MONNAIE — M3b-v2 : le circuit d'État achat/revente (fusion M3b+M4, 2026-07-14)

**Statut : LIVRÉ (3 commits : audit 3a9834c · circuit 815ee1a · fix savetest 65ccb02),
calibrage PARTIEL — pas de re-baseline golden, gate 1 (sweep apparié aux critères durs)
pas formellement franchi.** Contrairement à M3b v1 (STOPPÉ, monde effondré — Laborer à
0 % dès l'an 5, aucune reprise jusqu'à l'an 100), ce mécanisme ne collapse JAMAIS sur
les 9 sims du sweep (3 seeds × 3 sims × 250 ans) : satisfaction converge
progressivement, sans piège de pauvreté irréversible. Mais elle n'entre pas fermement
dans les bandes cibles pour Laborer sur 2/3 seeds — décision : livrer en l'état,
documenter le residu, PAS de golden-update (décision de l'orchestrateur à reprendre).

**Le mécanisme livré** (remplace intégralement le Cœur A de la v1, stash
`monnaie-m3b-flip-non-calibre` — le stash a été POP puis le mécanisme RETRAVAILLÉ, pas
réutilisé tel quel malgré la consigne de réutilisation : le brief v2 change
l'architecture — « l'État ACHÈTE » remplace « le compte de marché nourri par la conso » —
seuls les MOTIFS de plomberie ont survécu, pas le code) :
- `price_level[pays]` = `clampf(caisse_disponible/va_country_prev, 0, 1)` — la CAISSE est
  le trésor PROVINCIAL EXISTANT agrégé (Σ surplus au-dessus de SINK_FLOOR), AUCUN pool
  neuf (contrairement à `market_account` en v1, retiré du struct) — calculé UNE FOIS,
  en LECTURE SEULE, avant la boucle des provinces (aucun biais d'ordre possible, à la
  différence de la v1 qui mutait un pool partagé PENDANT la boucle).
- L'État ACHÈTE : wage_pool/profit_pool/tax_pool sont scalés UNIFORMÉMENT par
  `price_level` (jamais un rationnement en cascade — LA leçon de v1, où les DEUX
  calibrages avaient chacun fait s'effondrer une classe différente, cf. entrée M3b
  ci-dessus). C'est un facteur de PRIX qui baisse, pas une file d'attente.
- L'État REVEND : `price_level` remplace l'IPM GLOBAL au site du prix national (par
  PAYS désormais, pas par MONDE) — IPM neutralisé LÀ seulement (double emploi retiré),
  PAS supprimé ailleurs (provinces isolées hors-empire, surcharge d'entretien `ipmf`).
- Métaux monétaires (or/cuivre) EXEMPTÉS de `price_level` au site du prix (pl=1 pour
  eux) — l'étalon (v5, parité fixe) reste un numéraire stable pendant que tout le reste
  flotte contre lui.
- Le plancher/plafond de prix (0.15×..8×) SUIT désormais `price_level` au lieu
  d'ancrer sur `BASE_PRICE` nu (docs/MONNAIE_M3B2_AUDIT_SEUILS.md §2) — sinon
  l'ancrage à la genèse re-crée le piège « prix RIGIDES » nommé par le postmortem v1.
- Exonération sous le panier vital (brief §4, docs/MONNAIE_M3B2_AUDIT_SEUILS.md §1) :
  le forfait fiscal per-capita ne mord plus un ménage déjà sous le coût du panier/tête
  (`g_basket_pc`, lag 1 tick) — la conversion EXACTE du seul seuil qui avait tué v1.

**Découvertes (les DEUX bugs trouvés PAR sweep, invisibles à la relecture)** :
- **Crowd-out d'ordre (friche ×22)** : un premier essai débitait l'achat d'État
  IMMÉDIATEMENT après la production (§3, même endroit que le crédit) — la MÊME réserve
  (SINK_FLOOR) que l'entretien (§E1bis.10), qui s'exécute PLUS TARD dans la boucle,
  se retrouvait asséchée avant son tour (5→110 régions impayées, seed 9/250 ans, sweep
  1 sim). Un plancher plus haut (COURT_FLOOR=4000 au lieu de SINK_FLOOR=500) corrige la
  friche mais épingle `price_level` à 0 quasi en permanence pour les jeunes économies
  (le trésor dépasse rarement 4000 en début de partie) → cycles boom/bust au
  franchissement du seuil, VISIBLES au diagnostic `SCPS_MKTDIAG` (price_level alternant
  exactement 1.0/0.0 tick à tick pendant 50+ ans). **Le vrai fix est l'ORDRE, pas le
  plancher** : le débit de l'achat est DIFFÉRÉ (stocké dans une variable locale
  `pending_buy_debit`) et appliqué seulement APRÈS entretien/court/admin/redépense —
  ces sinks EXISTANTS gardent la priorité sur la caisse de début de tick, SINK_FLOOR
  redevient le bon niveau pour `price_level` (pas besoin d'un plancher inventé).
- **Spirale de trésor négatif (friche à 38 % des provinces même avec l'ordre fixé)** :
  même différé, un débit NON clampé pouvait pousser le trésor très négatif ; le test de
  friche du tick SUIVANT (`upkeep_order > re->treasury`) est vrai dès que le trésor est
  négatif QUEL QUE SOIT le montant dû (pas seulement si le montant dépasse le trésor
  positif) — un creux d'UN tick devenait plusieurs ticks de friche en cascade avant que
  la taxe ne renfle assez. Fix : clamper le débit à `max(0, treasury)` — le reliquat
  non financé rejoint l'instrument (`g_va_produced_cum`, un résidu MESURÉ) au lieu
  d'endetter silencieusement une province. Résultat : friche seed 9/250 ans revenue à
  13 régions sur 246 colonisées (pré-M3b2 : 5/246 — comparable, plus l'ordre de
  grandeur).
- **`can_buy = budget/cost` est bien scale-invariant sous `price_level` UNIFORME, mais
  ça ne suffit pas à égaliser les classes** : en théorie, scaler budget ET cost par le
  MÊME facteur ne change PAS le ratio (donc pas la satisfaction RÉELLE) — vérifié vrai
  en pratique. L'écart Laborer/Bourgeois-Élite observé (Laborer stagne plus bas, plus
  longtemps) vient d'AILLEURS : (a) la friche pénalise la PRODUCTION (0.6×) avant même
  que `price_level` n'entre en jeu, frappant plus les provinces à fort `wage_pool` ; (b)
  Bourgeois/Élite ont un COUSSIN de richesse accumulée (moins souvent budget-contraints,
  `can_buy` sature déjà à 1 avant le choc) alors que Laborer, subsistant panier par
  panier, encaisse chaque tick de plein fouet — un effet de RICHESSE ACCUMULÉE, pas du
  mécanisme prix lui-même.
- **`g_basket_pc` a cessé d'être scratch** : documenté depuis 2026-07 comme « recalculé
  ET consommé dans le MÊME passage d'econ_tick, avant toute lecture » (donc non
  sérialisé, par symétrie avec econ_prodcap_save) — l'exonération fiscale (§4 ci-dessus)
  le LIT désormais AVANT sa propre écriture du tick (le panier du tick PRÉCÉDENT),
  exactement le même profil que `g_friche`/`g_lowsat_streak` (déjà sérialisés, même
  famille de bug). `--savetest 9` divergeait sur le trésor (or=3958.6 vs 3941.1 à
  day=2095) tant que `g_basket_pc` n'a pas rejoint le blob EMOB — fix committé
  séparément (65ccb02), aucun nouveau tag ni bump séparé (couvert par le v88 déjà en
  cours pour `va_country_prev`).

**Pièges** :
- `git stash pop` sur un fichier où le commit v5 (parité, `scps_tune_list.h`) avait
  entre-temps ajouté des lignes AU MÊME endroit que la v1 (`MKT_SMOOTH_MONTHS` juste
  après `MINT_PARITY_COPPER`) → conflit de merge trivial mais RÉEL (les deux blocs
  coexistaient, juste mal ordonnés) — résolu en gardant les deux, puis
  `MKT_SMOOTH_MONTHS` a été retiré plus tard (le lissage temporel n'existe plus en v2,
  remplacé par le facteur de prix).
- Le worktree `pre-m3b2` (`git worktree add`) pour le sweep apparié se construit sous
  `C:\Users\Charl\AppData\Local\Temp\wt-...` (pas `/tmp` littéral — le Bash tool et
  MSYS2 bash.exe ont des racines `/tmp` DIFFÉRENTES sur cette machine ; `git worktree
  list` donne le vrai chemin Windows, à reconvertir en chemin POSIX `/c/...` pour
  MSYS2) — piège déjà entrevu par M3a, reconfirmé ici.
- `SCPS_MKTDIAG` élargi (`c<4` au lieu de `c==0`, la condition d'origine de la v1) a
  servi à distinguer un VRAI bug (persistance cassée) d'un comportement ORGANIQUE
  (cycles boom/bust d'une petite économie) — sans élargir à plusieurs pays, le motif
  « price_level alterne 1.0/0.0 » aurait pu passer pour un bug de persistance de
  `va_country_prev` alors que c'était `va_country_prev` qui se comportait CORRECTEMENT
  (persisté, non remis à zéro) et `caisse_snapshot` qui restait authentiquement à 0
  (petite économie, trésor sous le seuil).

**Mesures — sweep apparié `./chronicle {9,11,42} 3 250 6 12` (pré-M3b2 = tag
`pre-m3b2`, worktree, vs HEAD)** :

| seed | dérive M/an pré→v2 | réduction | Laborer pré→v2 | Bourgeois pré→v2 | Élite pré→v2 | hégémon pré→v2 |
|---|---:|---:|---:|---:|---:|---:|
| 9  | 130.1k→15.1k  | −88 % | 51→44 % | 78→77 % | 73→78 % | 2/3→3/3 |
| 11 | 217.9k→22.6k  | −90 % | 53→44 % | 77→70 % | 74→71 % | 2/3→0/3 |
| 42 | 208.7k→19.6k  | −91 % | 64→50 % | 88→79 % | 79→76 % | 2/3→0/3 |

Bandes cibles : Laborer 50-75 %, Bourgeois/Élite 70-90 % (moyenne sur 250 ans, PAS le
critère « sur toute la durée » du brief — non vérifié dans ce sweep réduit). Lecture :
la dérive de M chute de 88-91 % partout (l'instrument « création résiduelle » confirme :
conso ≈ 0/an sur les 9 sims, VA résiduelle ~15-99k/an — en baisse mais PAS à 0, la
caisse ne suit pas encore parfaitement la VA). Bourgeois/Élite restent dans ou près de
la bande sur les 3 seeds. **Laborer est SOUS bande sur 2/3 seeds** (44 % vs plancher
50 %, seed 42 pile à 50 %) — amélioration continue sur la durée (seed 9 : 22 %→51 % de
l'an 15 à l'an 250, JAMAIS de collapse) mais n'atteint la bande que tardivement
(~an 200-250 selon la trajectoire an-par-an vérifiée hors-sweep, seed 9 seul). L'hégémon
mortel, préservé sur seed 9 (3/3, contre 2/3 pré), DISPARAÎT sur 11/42 (0/3, contre 2/3
pré) — un effet secondaire NON expliqué à ce stade (stabilité politique amortie par le
nouveau circuit ? à creuser, pas dans le scope de cette mission).

**Gates réellement passés** : `make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo,
pré-existant Windows) sur 39 bancs · `make determinism` STABLE (5 graines × 12 ans,
hashes identiques run A/B) · `scps_viewer --savetest 9` 2/2 (A==B exact, fix EMOB
appliqué) · `make fuzz-save` 8/8 (216 octets flippés, aucun crash). **Gates PAS
passés/PAS lancés** : `make golden`/`golden-update` (attendu différent, aucune
re-baseline committée — décision : ne pas re-baseliner tant que le sweep n'est pas
formellement conforme, cf. l'interdiction du brief) · `make determinism-deep` (200 ans,
pas lancé, budget de session) · sweep « sur toute la durée » (seulement moyennes 250
ans + une trajectoire an-par-an ponctuelle vérifiée à la main sur seed 9 seul — pas les
9 sims).

**Restes (pour la prochaine tentative)** :
- **Calibrage fin de Laborer** : deux pistes NON essayées ici (budget de session) —
  (a) réduire encore la friche résiduelle (13/246 provinces, seed 9/250 ans — la source
  la plus probable de l'écart Laborer, cf. Découvertes ci-dessus) en creusant POURQUOI
  la caisse ne suit pas la VA d'assez près (le résidu `g_va_produced_cum` reste
  ~15-99k/an, pas 0) ; (b) un plancher de `price_level` (ex. 0.7-0.8) qui empêcherait
  l'achat de descendre trop bas en début de partie — RISQUE : reproduit potentiellement
  le piège de v1 si mal borné, à sweeper prudemment.
- **Item 5 du brief (dépenses d'État par famille — entretien/admin/encadrement → gages
  locaux, cour → élites de la capitale) NON commencé** — hors budget de cette session ;
  la « famille par famille, commits séparés » du brief reste entièrement à faire.
- **Le sweep apparié COMPLET aux critères DURS (bandes sur TOUTE la durée, pas la
  moyenne) n'a pas été exécuté** — seulement un sweep de moyennes (9 sims) + une
  trajectoire an-par-an (15/50/100/150/250) sur seed 9 seul. À refaire en entier avant
  toute décision de golden-update.
- **L'effet hégémon-mortel amorti (11/42 : 2/3→0/3) n'est PAS expliqué** — noté pour un
  futur diagnostic, pas creusé ici (hors périmètre monétaire strict).
- `SCPS_MKTDIAG=1` (stderr, print-only, élargi à c<4) reste câblé pour ré-instrumenter
  vite ; le motif « photo figée avant la boucle, appliquée en lecture seule, débit
  différé après les sinks existants, clampé à 0 » est le patron RÉUTILISABLE pour toute
  future dépense d'État distribuée par province (item 5 notamment).

## CHANTIER MONNAIE — M3b-v2.1 : item 5, le dispatch des dépenses d'État (2026-07-14)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT (99ef478), gates complets passés.**
La fin du circuit d'État : les dépenses d'État qui DÉTRUISAIENT la monnaie (les puits
M0 §2) deviennent des TRANSFERTS vers les classes qui les servent. 6 familles, 6 commits
(f732347 état-local · 9e611b4 militaire · 8d2ddbc manufactures · 4ceeac0 péages ·
06b8c1b conseil · b5e4bf8 événements) + levier neutre (24c0c89) + re-baseline (99ef478).

**L'hypothèse directrice de l'orchestrateur VÉRIFIÉE puis nuancée** : oui, recycler les
puits vers les classes remonte le Laborer (44→53/49/59 sur {9,11,42}, moyenne photo
an-250, 3 sims/graine) — mais PAS par le canal prévu. Le canal prévu (« la caisse fuit
⇒ price_level<1 ⇒ revenus<VA ») n'est PAS le canal mesuré : les transferts ne changent
RIEN à la caisse (l'État dépense pareil, seul le DEVENIR de l'or change), et la VA
résiduelle (price_level<1) n'a PAS fondu — elle a même GROSSI (économies plus riches ⇒
plus de VA ⇒ écart caisse/VA stable en ratio). Ce qui a remonté le Laborer, c'est le
REVENU DIRECT : solde, entretien, chantiers, routes créditent la classe qui manque
sans passer par le goulot du price_level.

**La table du dispatch (qui touche quoi)** :
- entretien édifices → 3 classes de LA province, 33/33/33 (UPKEEP_SHARE_LAB/BOURG, reg. J) ;
- encadrement manufactures (surtaxe IPM+H7) → gages 42/20/38 locaux (econ_wage_split) ;
- cour → ÉLITES de la CAPITALE (seule famille non-locale : la cour est un siège) ;
- admin → BOURGEOIS locaux · Conseil/décrets (FX_CONSEIL) → ÉLITES capitale ;
- solde+recrutement (warhost) + marine (navy) → LABORERS (capitale/rade) ;
- chantiers de manufactures (4 sites IA + 2 joueur + raw_boost) → LABORERS du chantier
  (pas de table de matériaux pour les manufactures : tout le coût est des gages) ;
- curseurs INVEST → 42/20/38 · ROUTES → LABORERS (cantonniers), grain région via le
  NOUVEAU helper econ_region_wealth_add (miroir exact de econ_region_treasury_add) ;
- péages (marge d'import agency, levy des routes, détroit) → BOURGEOIS de l'hôte
  (décision : les marchands, PAS le trésor — l'État y perd un revenu, compensé) ;
- coûts d'ÉVÉNEMENTS (d_treasury<0, d_treasury_mois<0) → 42/20/38 région sujette ;
  les GAINS d'événements restent une création pure (trier la contrepartie = un site
  par événement, hors budget — RESTE).
- RESTENT des destructions documentées : FX_AUDIT (probité), l'achat d'armes doctrinal
  (scps_ai.c ~1466 : il CRÉE le stock ex nihilo — payer un vendeur créditerait sans
  retirer de bien), FX_INTRIGUE, la part libre du chantier d'ÉDIFICE (FX_BUILD paie
  déjà sources+péage via intertrade_market_consume ; le reliquat des matériaux pris
  gratis dans l'empire est complexe à isoler — RESTE).

**Mesures (sweep {9,11,42} × 3 sims × 250 ans, photo an-250, vs HEAD-avant-conversions)** :

| seed | Laborer | Bourgeois | Élite | hégémon mortel | dérive M/an |
|---|---|---|---|---|---|
| 9  | 44→53 | 77→76 | 78→77 | 3/3→2/3 | 15.1k→24.3k |
| 11 | 44→49 | 70→69 | 71→72 | 0/3→2/3 ✓ | 22.6k→30.3k |
| 42 | 50→59 | 79→75 | 76→79 | 0/3→1/3 ✓ | 19.6k→17.3k |

Réf. pre-m3b2 : hégémon 2/3 partout, dérive 130-218k/an. L'hégémon mortel est RESTAURÉ
(l'amortissement 11/42 diagnostiqué par la vague précédente venait des classes appauvries
= monde politiquement TROP docile ; les transferts re-suscitent la contestation).
Trajectoire seed 11 (photos 50/100/150/250) : L 37→42→50→49 — convergence monotone,
bande atteinte ~an 150, AUCUN collapse. Seed 9 an-100 : 41 (pre-m3b : 47 — l'early game
reste plus pauvre qu'avant le circuit, converge au-dessus ensuite).

**Calibrage : les leviers balayés et REJETÉS (les conversions SONT le calibrage)** :
- MINT_ROYALTY/AI_SHARE 0.15→0.25 : L 50/50/55 (perd 3-4 pts sur 9/42), hégémon amorti
  sur 42 (1/3→0/3) — rejeté ;
- TAX_EXEMPT_BASKET_MULT ×1.5 : monte seed 11 (49→54) mais 9 (53→48) et 42 (59→54)
  plongent, hégémon down — le levier REDISTRIBUE le chaos au lieu d'améliorer ; ×2.0
  pire partout. Rejeté ; le levier reste au registre J (neutre 1.0) pour l'avenir.
- Leçon : sur un système aussi chaotique, ±5 pts entre deux configs d'UNE graine est
  du BRUIT de bifurcation — ne juger que sur les 3 graines × 3 sims, jamais une seule.

**Critères durs — le solde honnête** :
- Laborer ≥50 « sur toute la durée » : NON TENU STRICTO SENSU par AUCUNE config mesurée
  (pre-m3b2 inclus : 47 à l'an 100) — l'early game d'une jeune colonie est pauvre par
  STRUCTURE. Tenu en lecture raisonnable : converge en bande (~an 150), moyenne an-250
  50.3, zéro collapse. Seed 11 à 49 = bruit (cf. leçon ci-dessus).
- B/E 70-90 : tenu partout sauf B seed 11 à 69 (1 pt, même bruit).
- conso ≈ 0 ✓ (0-500/an) · colonisation vivante ✓ (109-127 fondations/sim) · friche
  non épidémique ✓ (5-11 rég/246) · pop ±10 % ✓ (mêmes ordres de grandeur).
- VA ≈ 0 : NON — 47-105k/an de création résiduelle (price_level<1 structurel, la
  caisse ne suit pas la VA d'une économie en croissance). Dérive ≈ frappe : NON
  (frappe ~1-3k/an, dérive 17-30k/an). Ces deux critères exigent que la CAISSE
  couvre la VA en régime de croissance — un problème de fond du Cœur A (le trésor
  provincial n'est pas dimensionné pour acheter TOUTE la production), pas un tunable.
  RESTE MAJEUR pour M3c/M4 : soit la frappe finance la croissance de M (la banque
  centrale émergente), soit price_level devient le vrai régulateur déflationniste
  (M4 prix locaux) et la VA nominale converge d'elle-même.

**Pièges** :
- WAGE_SHARE/TAX_RATE sont des #define FILE-LOCAL de scps_econ.c — un module externe
  (events, intertrade) ne les voit PAS : econ_wage_split(amount,&l,&b,&e) est le
  passage obligé (ajouté à scps_econ.h), ne PAS dupliquer les constantes.
- Le péage de détroit crédite DÉJÀ le trésor du tenant via it_treasury AVANT ma
  conversion — la conversion remplace ce crédit (wealth au lieu de treasury), ne pas
  ADDITIONNER les deux (double création silencieuse).
- `tasklist | grep chronicle` avant tout rebuild : un chronicle.exe encore vivant fait
  échouer le link (« Permission denied ») en silence dans un script — le build dit
  BUILD DONE mais le binaire est l'ANCIEN (mesures fantômes). Vécu deux fois.
- La « satisfaction moy » de la SYNTHÈSE chronicle est la PHOTO de fin de sim (an N),
  pas une moyenne temporelle — pour une trajectoire, relancer à horizons 50/100/150.

**Gates passés (tous)** : golden VERT (re-baseliné 99ef478, diff 5 lignes revu) ·
make test 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo, pré-existant Windows) ·
make determinism STABLE (5 graines × 12 ans) · scps_viewer --savetest 9 : 2/2 A==B
byte-identique (v88, aucun nouvel état sérialisé — les transferts sont instantanés,
zéro accumulateur inter-ticks ajouté) · make fuzz-save 8/8.

**Restes** :
- VA résiduelle 47-105k/an (voir critères durs ci-dessus) — LE chantier suivant.
- Gains d'événements (d_treasury>0) : encore une création pure, à trier par événement.
- FX_AUDIT/intrigue/part libre de FX_BUILD édifices : destructions documentées.
- Early game Laborer <50 avant ~l'an 150 : structurel (jeunes économies), à revisiter
  quand la frappe financera la croissance (cf. RESTE MAJEUR).

## CHANTIER MONNAIE — M3c : LE CRÉDIT RÉEL (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, tous les gates passés.** Le dernier
canal magique (scps_econ.c:3516-3519 v88 : le débit de l'achat d'État clampé au trésor
LOCAL, le reliquat créé sans prêteur) est fermé : péréquation nationale → emprunt aux
propres classes → emprunt à une cité-état → épuisement mesuré (jamais créé). La dette
est un PASSIF SÉPARÉ, sérialisé (SAVE_VERSION 89), ventilé par créancier (classes/
cité-état), avec intérêt réel, amortissement, et rachat de crédit (le marché secondaire).

**L'architecture livrée (scps_credit.c, refonte complète)** :
- `credit_borrow_local(e,c,need)` — péréquation (Σ surplus des AUTRES provinces du pays,
  >SINK_FLOOR) PUIS emprunt aux PROPRES classes (élites+bourgeois, ∝ richesse pondérée
  ELITE_LEND_WEIGHT/BOURGEOIS_LEND_WEIGHT, capacité CLASS_LEND_SHARE/tick). AUCUN
  World* requis (province-grain pur — econ_tick n'en a pas, ProvinceEconomy::region
  l'explique déjà). DÉBIT SEUL des deux étages : le crédit a DÉJÀ eu lieu (la VA payée
  en plein à price_level, scps_econ.c §3) — la chaîne ne fait que trouver QUI, dans le
  monde, a réellement avancé les pièces. Vérifié algébriquement (pending_buy_debit =
  local_debit + covered_local + covered_cs + remain_final, ΔM = remain_final SEUL).
- `credit_borrow_citystate(e,w,c,need)` — emprunt à une cité-état/mercantile-pacifiste
  solvable (créancier EXISTANT prioritaire — sans quoi `pick_lender` élit le PLUS RICHE
  CE tick, presque jamais le même d'un tick à l'autre, cf. Pièges). Requiert World*
  (rôle/éthos du prêteur) : c'est le SEUL étage qui en a besoin.
- `econ_va_shortfall_pending/resolve` (scps_econ.h) — le pont entre les deux mondes :
  econ_tick (sans World*) tente péréquation+classes en interne et STOCKE le besoin
  résiduel dans un snapshot (non sérialisé, écrasé chaque tick) ; `credit_settle_
  monthly` (scps_sim.c, juste après econ_tick — LUI a w) tente l'étage cité-état et
  RETRANCHE ce qu'il finance de l'instrument (g_va_produced_cum) — ce qui a été
  financé après coup n'était PAS une création.
- `credit_spend` (refonte) : débite le trésor local (peut passer négatif TEMPORAIREMENT),
  puis appelle `credit_borrow` (la chaîne complète, local+cité-état) pour REMPLIR le
  trou — contrairement à econ_tick, credit_spend a déjà un World* donc peut le faire
  en UN appel synchrone (pas de round-trip settle_monthly).
- `credit_year_tick` (refonte) : intérêt payé du SURPLUS COURANT SEUL (jamais via
  credit_borrow* — piège n°1 ci-dessous), réparti aux DEUX créanciers ∝ leur part de
  dette (élite rentière + cité-état) ; amortissement du principal depuis le surplus
  >COURT_FLOOR ; rachat de crédit (cité-état au trésor oisif rachète la dette-classes
  à valeur faciale, devient le créancier).

**Pièges (les DEUX bugs de calibrage, invisibles à la relecture)** :
- **Intérêt payé via credit_borrow* = dette fabriquée sans contrepartie.** Premier jet :
  `credit_year_tick` finançait l'intérêt en appelant `credit_borrow(e,w,c,interest)`
  (la MÊME chaîne que credit_spend). Ça COMPILE, ça PASSE credit_demo, mais c'est FAUX :
  `credit_borrow_citystate` DÉBITE le prêteur (conservateur SEULEMENT si la classe/
  cité-état débitée "finance" un crédit DÉJÀ accordé ailleurs, cf. l'algèbre ci-dessus)
  — pour l'intérêt, RIEN n'a été crédité au préalable, donc emprunter pour payer
  l'intérêt fait DOUBLE emploi : le principal grossit du montant emprunté (g_debt+=
  covered) ET ce même montant est immédiatement versé au créancier comme "intérêt
  payé" — la dette explose SANS mouvement réel net côté débiteur. Trouvé PAR le banc
  invariant (chronicle : "autres" cumulé à 269 % de M(fin) en 250 ans, seed 9). Fix :
  l'intérêt se paie EXCLUSIVEMENT du surplus courant (`country_surplus`/
  `debit_surplus_prorata`, jamais credit_borrow*) — s'il manque, l'intérêt de l'année
  est simplement PLUS PETIT (auto-limité), jamais capitalisé.
- **Rachat de crédit à COURT_FLOOR (4000) : 0 rachat sur 9 sims.** `pick_lender` élit
  le PLUS RICHE prêteur éligible CE tick — un créancier-cité-état déjà ACTIF (prêtant
  chaque mois via credit_settle_monthly) redéploie continûment son capital, il redescend
  RAREMENT au-dessus de COURT_FLOOR (le seuil de HOARDING, motif FX_COURT/admin) —
  contrairement à un trésor qui dort. Fix : le seuil d'oisiveté du rachat devient
  SINK_FLOOR (500, le même bar qu'un prêt normal) — "racheter est un placement sûr,
  pas moins attractif qu'un nouveau prêt". Passé de 0 à 79-2424 rachats/sim.
- **Le banc invariant CUMULÉ contre M(t) dérive sans borne.** Premier jet :
  `autres_cum/M(t)` (le brief dit "M(t)=M(0)+frappe") — mesuré : croît MONOTONE de
  0 % (an 1) à 269 % (an 249, seed 9), aucun seuil "serré" n'est jamais stable (les
  composantes ne se COMPENSENT pas d'une année sur l'autre, elles s'ACCUMULENT). Fix :
  DELTA ANNUEL (pas cumulé depuis la genèse), normalisé par l'ÉCHELLE d'activité
  CONNUE de l'année (Σ|ΔVA·Δconso·Δcoloniz·Δfrappe|, pas M(t) ni le delta NET — qui
  peut être petit par pure COMPENSATION de signes opposés, cf. découverte suivante).
  Résultat : le ratio reste STABLE (quelques dizaines à ~300 % selon l'année), un
  détecteur de RÉGRESSION viable (1 pic isolé/2200 vérifs à 301 %, sweep {9,11,42}×3×250).

**Découverte structurelle — "autres" (missions/tribut mûri/arbitrage/gains d'événements/
pillage-stock, M0 §1.3-1.5/1.7/2.12, HORS SCOPE M3c) est DU MÊME ORDRE DE GRANDEUR que
la VA elle-même** (mesuré : VA +31-95k/an, autres -18 à -80k/an — PAS 10× plus petit
comme supposé). Un seuil "serré" (quelques %) sur la conservation TOTALE du jeu n'est
donc PAS atteignable tant que ces sites ne sont PAS convertis à leur tour — le banc
invariant livré est un DÉTECTEUR DE RÉGRESSION (une EXPLOSION soudaine signale un
NOUVEAU canal magique), pas une preuve de conservation totale. INVARIANT_DRIFT_FRAC=4.0
(400 %) au registre J, à resserrer une fois ces sites convertis (M3d ou équivalent).

**Mesures — sweep apparié `./chronicle {9,11,42} 3 250 6 12` (pre-m3c = tag `pre-m3c`,
worktree, vs HEAD)** :

| seed | dérive M/an pré→v3c | VA résiduelle pré→v3c | Laborer pré→v3c | hégémon pré→v3c |
|---|---:|---:|---:|---:|
| 9  | 20536/33251/19133 (moy³) → 10150/10686/10587 | 59288/71140/46695 → 35633/31333/33762 | 53%→49% | 2/3→1/3 |
| 11 | 35882/35467 (2 sims) → 13539/19607/13648 | 105584/94292 → 88485/61523/48136 | 49%→46% | 2/3→1/3 |
| 42 | 15786/14648/21486 → 13123/10106/7631 | 73047/60945/105503 → 88276/67425/55278 | 59%→61% | 1/3→0/3 |

Lecture : la dérive nette de M chute de ~45-60 % partout ; la VA résiduelle (LE chiffre
que M3c cible) baisse de 30-45 % (PAS à ~0 — voir découverte "autres" ci-dessus : le
reliquat restant migre en bonne partie vers la dette réelle, mesurée, plutôt que d'être
imprimé). Laborer/Bourgeois/Élite restent dans ou proches des bandes cibles (46-62 % /
66-79 % / 70-82 %), comparables au pré-M3c (bruit de bifurcation, cf. leçon M3b-v2.1).
**Hégémon mortel s'affaiblit sur les 3 graines** (2/3→1/3 ×2, 1/3→0/3 ×1) — effet
MODÉRÉ, RÉCURRENT depuis M3b-v2 (déjà noté "non expliqué" à cette étape-là), plausible :
des classes qui ne peuvent plus perdre TOUTE leur richesse en création pure (elles la
prêtent contre un actif, la dette réelle) restent globalement plus riches/stables →
monde politiquement moins volatil. Hors scope de diagnostiquer plus avant ici.

**La dette VIT (télémétrie chronicle, nouvelle ligne `dette (M3c)`)** : 1.3-4.6M or de
dette totale/sim en fin de partie (25-34 pays débiteurs/sim), 86-97 % due aux PROPRES
classes, 3-14 % à une cité-état — RATIO STABLE et cohérent avec le brief ("l'État
emprunte D'ABORD aux classes") ; 79-2424 rachats/sim (le marché secondaire, actif) ;
~9-12k épisodes d'épuisement/sim (des tentatives d'emprunt PARTIELLEMENT non couvertes
— le canal se ferme : mesuré, jamais créé, cf. g_va_produced_cum qui absorbe le résidu).
AUCUN buyback n'aboutissait avant le fix SINK_FLOOR (voir Pièges).

**Gates passés (tous)** : golden RE-BASELINÉ VERT (diff 5 lignes, hash changé — attendu,
le circuit monétaire change) · `make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo,
setenv, pré-existant Windows) · `make determinism` STABLE (5 graines × 12 ans) ·
`make determinism-deep` STABLE (200 ans × 2 graines, NOUVEAU — pas lancé aux vagues
précédentes) · `scps_viewer --savetest 9` : A==B byte-identique (v89, dette sérialisée
via credit_save/credit_load — SVT_CRDT grandit : 2 float + 1 int16/pays au lieu d'un
seul int16 g_creditor) + altération d'un octet REFUSÉE · `make fuzz-save` 8/8 (216
octets flippés, save_sane rejette chaque forge, aucun crash).

**Piège de plomberie (Makefile)** : `scps_econ.c` inclut désormais `scps_credit.h`
(credit_borrow_local, appelé par econ_tick) — TOUT binaire qui lie `scps_scps_econ.o`
sans `scps_scps_credit.o` échoue au LINK ("undefined reference"). 22 cibles du Makefile
(readout_demo, agency_demo, ai_demo, statecraft_demo, la plupart des *_demo) ne
l'avaient pas — ajouté un `$(OBJDIR)/scps_scps_credit.o` juste après CHAQUE référence à
`scps_scps_econ.o`, BLOC-AWARE (les continuations `\` du Makefile groupées, pas juste
la ligne courante — un premier essai en `sed` global a produit des "multiple definition"
en dupliquant credit.o dans des blocs qui l'avaient DÉJÀ ailleurs, ex. AI_DEMO_OBJS ;
awk avec un test `$0 ~ /\\[ \t]*$/` s'est avéré capricieux dans ce shell — perl,
plus prévisible, a fait le job en un passage).

**Restes (hors scope, documentés)** :
- "autres" (missions/tribut mûri/arbitrage/gains d'événements/pillage-stock, M0
  §1.3-1.5/1.7/2.12) reste NON converti, du même ordre de grandeur que la VA — LE
  prochain palier pour un banc invariant réellement "serré" (M3d ?).
- Épisodes d'épuisement (~9-12k/sim) : le canal ne ferme pas à 100 % — un pays peut
  encore avoir un besoin que péréquation+classes+cité-état ne couvrent pas ; le
  reliquat rejoint g_va_produced_cum (mesuré, documenté), jamais un trésor négatif.
  Non creusé plus avant (bornage suffisant pour le sweep, cf. gates).
- Hégémon mortel affaibli (2/3→1/3 récurrent) : effet MODÉRÉ, plausiblement lié à la
  dette réelle (les classes ne s'appauvrissent plus par pure création ratée — elles
  prêtent contre un actif), NON diagnostiqué en profondeur (hors scope monétaire strict,
  même verdict que M3b-v2's hégémon amorti).
- Gameplay de défaut profond (bankruptcy) : explicitement HORS SCOPE (brief) — les
  hooks existants (credit_of, la dette lisible) restent la seule surface pour une
  vague future.

## CHANTIER MONNAIE — M4-IP : L'INITIATIVE PRIVÉE (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés.** La réponse
à la thésaurisation née de la boucle fermée M3 : deux exutoires SPONTANÉS du surplus
(aucun verbe joueur — le peuple agit seul, joueur/IA traités PAREIL). 5 commits :
colonisation du peuple (8224910) · investissement privé (799073b) · télémétrie
(b642b09) · calibrage IP_SHORTAGE (c3549a1) · re-baseline (be1810c).

**Le mécanisme livré (scps_econ.c, fin de section colonisation)** :
- `econ_ip_colonize_tick` : les JOURNALIERS d'une province où richesse/tête >
  IP_COLON_WPC (8, registre J) émigrent vers la meilleure province VACANTE adjacente
  — mirror étroit de colonize_from_prov restreint à CLASS_LABORER (pop ET richesse,
  ∝ mêmes fractions ⇒ transfert PUR, net 0 par construction, jamais dans l'instrument).
  Gardes existants réutilisés : COLONY_MIN_POP (500), COLONY_FOOD_GATE, take ≤ 25 %.
- `econ_ip_invest_tick` : la classe (BOURGEOIS prioritaire ; ÉLITE si les bourgeois
  n'ont pas la surface — le « plus simple » du brief retenu, pas de co-financement)
  dont richesse/tête > IP_INVEST_WPC (12) finance la manufacture du bien de SON
  panier (NEED_ORDER, rang croissant = urgence) en pénurie ICI (prix ≥ IP_SHORTAGE×base)
  — miroir des gates civiles CMD_BUILD_MANUF (civil seul, tier, staffage, nourrissable
  royaume). Paiement = TRANSFERT (classe débite → gages laborers locaux, motif item 5) ;
  JAMAIS credit_spend (M3c intact par construction).
- Cadence MENSUELLE (juste après credit_settle_monthly, scps_sim.c) ; AUCUN état neuf
  sérialisé (relit wealth/pop/price du tick — pas d'accumulateur inter-ticks, la
  jurisprudence COLC évitée par construction) ; télémétrie cumulative non sérialisée
  (motif econ_colony_stats) + lignes chronicle « richesse/tête » et « initiative privée ».

**Découvertes (ce qui a coûté cher)** :
- **§NF v2 court-circuite tout investisseur qui attend NF_SHORTAGE** : econ_build_tick
  (DANS econ_tick, mensuel) sème un bâtiment GRATUIT niveau 1 le mois même où le prix
  franchit 1.8× — un mécanisme payant au même seuil, exécuté APRÈS econ_tick, ne trouve
  plus de slot libre. Deux fixes empilés : (a) la voie RENFORCER (slot occupé ⇒
  econ_manuf_level_delta +1, motif CMD_MANUF_LEVEL « injection de capacité DÉLIBÉRÉE,
  payante ») — sans elle ~2 manufactures privées/sim ; (b) IP_SHORTAGE=1.4 < 1.8 : la
  fenêtre s'ouvre AVANT le semis gratuit (« c'est LEUR besoin qui appelle ») — s9 passe
  de 170 à 611 manufactures privées/sim.
- **La cadence ANNUELLE était un puits négligeable** : première version au motif
  econ_colonize_tick (day%365) → 2-5 actes/sim sur 250 ans, invisibles contre la
  croissance MENSUELLE composée du salaire/profit. Le passage au mensuel (motif
  credit_settle_monthly) est ce qui rend les compteurs vivants (26-36 colonies,
  477-1833 manufactures par graine×3sims).
- **Le paradoxe du stimulus, mesuré** : PLUS d'investissement privé ⇒ richesse/tête
  bourgeoise PLUS HAUTE (s9 : 611 manuf/sim ⇒ B 81.5 vs 64.4 à 170 manuf/sim) — chaque
  manufacture ajoute de la VA, dont 20 % renfle le profit bourgeois : l'exutoire
  auto-alimente la classe qui le finance. L'investissement augmente la VÉLOCITÉ
  (B→gages L→conso→trésor), pas une baisse mécanique du stock.
- **IP_COLON_WPC=8 est un seuil d'OUTLIERS voulu** : la richesse/tête Laborer de régime
  est 2-6 — seules les provinces exceptionnellement prospères essaiment (9-12
  colonies du peuple/sim). L'abaisser à ~4 ferait saigner 25 % des journaliers CHAQUE
  MOIS des provinces riches (le wpc ne baisse PAS au départ — prélèvement proportionnel
  ⇒ la porte reste ouverte) : non tenté, risque démographique documenté ici.

**Sweep apparié (pre-m4ip=c122d1d, worktree, vs HEAD final IP_SHORTAGE=1.4,
`./chronicle {9,11,42} 3 250 6 12` — le pre INSTRUMENTÉ par cherry-pick print-only
du patch télémétrie, motif M3a « l'instrument poids-zéro »)** :

| seed | richesse/tête fin L/B/E pré | L/B/E post | satisf. L pré→post | prov colonisées fin | hégémon |
|---|---|---|---|---|---|
| 9  | 3.26 / 65.4 / 24.0 | 3.57 / 81.5 / 29.6 | 49→49 | 244→253 | 1/3→1/3 |
| 11 | 4.83 / 75.5 / 44.6 | 5.66 / 74.0 / 40.9 | 46→49 | 245→269 | 1/3→2/3 |
| 42 | 3.81 / 41.3 / 32.9 | 2.75 / 40.0 / 37.0 | 61→65 | 207→248 | 0/3→0/3 |

- **Le critère « richesse/tête STABILISÉE » : NON ATTEINT stricto sensu, et le sweep
  explique pourquoi** — l'exutoire recycle ~50-60 or/acte × quelques centaines d'actes/
  sim ≈ 1-3 % du stock de classe par partie, contre des moteurs d'accumulation (VA
  42/20/38 + intérêts M3c aux élites rentières) de deux ordres de grandeur au-dessus.
  Le débit maximal (1 acte/province/mois, borné par la fenêtre de pénurie qui se
  referme dès que l'offre suit — la boucle voulue) ne peut PAS épingler un stock de
  centaines de milliers d'or. Les différences pré/post par classe (B −8 % à +7 % selon
  la config) sont DANS le bruit de bifurcation (leçon M3b-v2.1 : σ par-sim B = 37-100
  sur une même graine). Pour un vrai plafond, il faudrait un débit ∝ surplus (pas un
  coût fixe de manufacture) — un autre design, hors brief.
- **Ce qui est PROUVÉ tenu** : l'expansion VIT (+9/+24/+41 prov colonisées fin de sim ;
  les colonies du peuple s'AJOUTENT : 23-34/graine×3sims) · manufactures privées > 0 et
  PERTINENTES par construction (le bien cherché est un palier NEED_ORDER du panier de
  la classe investisseuse, en pénurie locale réelle) · satisfaction stable/mieux
  (L 49→49 · 46→49 · 61→65) · hégémon mortel pas dégradé (1/1/0 → 1/2/0 par graine) ·
  invariant M3c VERT (dérive M comparable 10-17k/an, dette vivante 55-1840 rachats/sim,
  conso ≈ 0, colonisation exactement 0 dans l'instrument — transferts purs prouvés par
  la mesure).

**Pièges** :
- `econ_seed_population` (réutilisée pour semer la colonie du peuple) écrit une
  richesse EX NIHILO (genèse) — l'ÉCRASER juste après par la richesse réellement
  emportée (toutes classes : wealth_taken pour Laborer, 0 pour les autres) est
  obligatoire, sinon on recrée la planche à billets M0 §1.2 fixée par M3a.
- Le pre-worktree n'a PAS les readers de télémétrie (econ_ip_stats n'existe pas
  avant M4-IP) : cherry-pick du patch chronicle PUIS suppression à la main des 2 blocs
  qui l'appellent — un `git apply` naïf du patch complet ne compile pas sur le tag.
- Les instantanés « richesse/tête an N » divergent dès l'an 0 entre pré et post :
  le mécanisme mord au premier tick (pas de warm-up) — ne PAS s'en étonner en
  comparant les lignes an-0 (golden re-baseliné pour la même raison).
- Golden : seeds 108/310 IDENTIQUES au pré-M4-IP même après les deux vagues — aucune
  initiative ne franchit ses seuils en 12 ans sur ces graines : le hash prouve que le
  mécanisme est GATÉ, pas ambiant.

**Restes** :
- Le plafond de richesse/tête (le vrai « thésaurisation vaincue ») demande un débit
  ∝ surplus — soit des actes multiples par fenêtre, soit un coût d'acte qui monte avec
  la richesse de l'investisseur (co-financement ∝ richesse du brief, non retenu ici
  par simplicité). À re-proposer comme M4-IP2 si le joueur veut le plateau dur.
- Les ÉLITES investissent rarement (les bourgeois passent leur gate presque toujours
  en premier) — leur revenu rentier M3c reste sans exutoire dédié ; le co-financement
  ∝ richesse les brancherait directement.
- IP_COLON_WPC=8 : seuil d'outliers assumé (cf. Découvertes) ; un monde futur qui veut
  des migrations de masse abaissera le seuil ET ajoutera un répit (motif COLONY_CD) —
  les deux ensemble, jamais le seuil seul.
- UI : aucune surface façade (readout « le peuple a essaimé/bâti ») — hors brief,
  la membrane n'expose encore rien de M4-IP au joueur.

## CHANTIER MONNAIE — M3d : LA SOUTENABILITÉ + LA BANQUEROUTE (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ, `make test` 38/39 (1 build-échec Windows
pré-existant, intertrade_demo/setenv), determinism + determinism-deep STABLES, savetest
v90 A==B, fuzz-save 8/8.** 6 commits : re-tarif jobs (demande joueur, pré-calibrage) ·
le moteur plafond/refus/taux/tranche/banqueroute (scps_credit.c) · le câblage inter-
modules (débuff/verbe/dotations, SAVE_VERSION 90) · la mesure (chronicle) · golden ·
ce TROUVAILLES.

**AJOUT JOUEUR intégré AVANT le calibrage (« les jobs sont chers, jamais retouchés
depuis le rework ») — H7 RE-TARIFÉ** : `econ_job_upkeep_month` (scps_econ.c) remplace
`MANUF_UPKEEP_DAY` (flat 0.05/j/niveau) par `JOB_UPKEEP_TAX_FRAC(0.60) × (ouvriers ×
TAX_BASE_LABORER) × ipmf / max(prix du bien produit, JOB_UPKEEP_PRICE_FLOOR×prix de
base)`. **Le choix IPM, documenté** : sous prix libres (M3b-v2), `price_level[c]`
(par-pays, gouverne `re->price` au dénominateur ICI) a remplacé `e->ipm` pour le PRIX
DES BIENS — mais `ipmf` reste EXPLICITEMENT actif pour la surcharge d'entretien (déjà
noté par M3b-v2 lui-même, scps_econ.c §PRIX NATIONAL) : PAS de double emploi, deux axes
distincts (marché local du bien vs débasement monétaire séculaire des dépenses
d'État). `scps_manuf_upkeep_month` (façade) est le MIROIR EXACT — ⚠ la DLL Godot doit
être re-buildée (scons -C godot, hors scope de cet agent, signalé à l'orchestrateur).

**L'architecture livrée (scps_credit.c, extension de M3c)** :
- `credit_debt_ceiling(c)` = `DEBT_CEILING_YEARS(3.0) × econ_country_tax_year(c)` (le
  revenu annuel NOMINAL, la MEMBRANE DE DÉCISION déjà établie par M3b-v2/§6-7 — pas une
  nouvelle notion de revenu). `debt_draw_cap(c)` = `min(plafond−dette_actuelle,
  DEBT_TRANCHE_FRAC(0.20)×revenu)` — gate CHAQUE draw de dette RÉELLE (classes ET
  cité-état, INDÉPENDAMMENT — deux tranches séparées, pas un budget partagé entre les
  deux étages : choix « le plus simple », documenté ci-dessous). La PÉRÉQUATION
  (transfert entre les provinces d'un MÊME pays, `credit_borrow_local` §1) reste
  EXEMPTÉE — ce n'est pas un prêt, brief §1 vise « plus personne ne prête ».
- `credit_year_tick` : le TAUX devient `clamp(0.02 + 0.03×(dette/plafond), 0.02, 0.05)`
  — remplace ENTIÈREMENT la formule de l'incrément 1 (`CREDIT_RATE_BASE×(1+ratio+
  (10−légitimité)/10)`, ratio sur la ligne ∝pop, `CREDIT_RATIO_CAP` anti-emballement).
  La légitimité NE PONDÈRE PLUS le taux (brief §3 ne la mentionne pas) ; `CREDIT_RATIO_
  CAP` sur l'ASSIETTE d'intérêt devient inutile — le plafond structurel (§1) borne déjà
  `debt_total` en amont, l'assiette d'intérêt EST `debt_total` sans reclamp séparé.
- `credit_bankruptcy(e,c,forced)` : répudiation TOTALE (`to_class=to_cs=0`, `cs_id=-1`)
  — AUCUNE mutation de richesse côté créancier (« leur monnaie est déjà partie au prêt » :
  le prêt originel a DÉJÀ débité le prêteur au moment de l'emprunt, credit_borrow* — la
  répudiation efface juste la CRÉANCE, un compte hors-M(t), l'invariant M3c ne bouge
  pas). Pose `bankruptcy_scar=1` sur toutes les provinces actives du pays (débuff −75 %
  câblé scps_econ.c/scps_campaign.c, commit « câblage »). `insolvent_streak` (sérialisé,
  motif EMOB/COLC) compte les années CONSÉCUTIVES au plafond ; ≥`BANKRUPTCY_GRACE_YEARS`
  pose `g_forced_pending[c]`, consommé par scps_sim.c juste après `credit_year_tick`
  (boucle sur tous les pays, joueur ET IA — brief §5b : « FORCÉE » n'est pas réservée au
  joueur, seule la voie VOLONTAIRE, CMD_BANKRUPTCY, l'est).

**Décisions « le plus sain » (licence explicite du brief en cas d'incohérence)** :
- **L'escompte du banquier ÉCARTÉ.** Le brief §4 propose « PASSIF inscrit = tranche ×
  (1+taux) ». Implémenté littéralement, ce serait EXACTEMENT le bug n°1 déjà documenté
  par M3c (« intérêt payé via credit_borrow* = dette fabriquée sans contrepartie ») :
  le prêteur ne débite que `tranche` (le débit RÉEL, ce que credit_borrow_local/
  citystate transfèrent), donc inscrire `tranche×(1+taux)` au passif créerait
  `tranche×taux` de dette PURE (aucun prêteur n'a avancé cette part) — violerait
  l'invariant M3c que le gate 2 exige VERT. Retenu : la tranche plafonne le DRAW
  reçu = le débit réel du prêteur = le passif inscrit (les trois égaux, conservateurs
  par construction) ; l'intérêt reste EXCLUSIVEMENT le rôle de `credit_year_tick`
  (jamais capitalisé, motif M3c intact). FLAG explicite au rapport final.
- **LE REFUS des cités-états (brief §2) : le motif EXISTANT, pas un nouveau canal.**
  `credit_borrow_citystate` ne reçoit ni `DiploState*` ni `WorldProsperity*` (seulement
  `World*`) — appeler `diplo_relation` pour un vrai signal « hostile/embargo » aurait
  exigé de les faire voyager à travers `credit_spend`/`credit_settle_monthly`/
  `econ_tick`, touchant des dizaines de sites hors sujet. Le motif `country_surplus(e,
  L,floor_)>CR_EPS` (déjà existant, M3c) EST « un prêteur sous son propre plancher
  opérationnel refuse » — documenté comme satisfaisant le brief littéralement (« choisis
  au motif existant »), pas une esquive.
- **La tranche est PAR SOURCE (classes/cité-état), pas un budget d'épisode partagé** :
  `debt_draw_cap` s'applique INDÉPENDAMMENT dans `credit_borrow_local` (étage classes)
  et `credit_borrow_citystate` (étage cité-état) — un pays peut donc drainer jusqu'à
  ~2×tranche/mois en cumulant les deux étages dans le PIRE cas. Choix pragmatique (les
  deux étages avaient DÉJÀ des capacités indépendantes, `CLASS_LEND_SHARE`/
  `CITYSTATE_LEND_SHARE` — composer un nouveau plafond dans le MÊME esprit, pas une
  refonte). Non re-testé séparément ; la mesure (dette/revenu PLAFONNE en régime,
  ci-dessous) suggère que ce n'est pas un problème pratique.

**Piège de calibrage (mesuré, pas supposé)** : premier jet `BANKRUPTCY_GRACE_YEARS=2`
→ 210 banqueroutes forcées/250 ans pour 7 pays débiteurs (seed 9, 1 sim) — un pays
rebondissait au plafond et re-défaultait tous les ~8 ans, PLUS VITE que la cicatrice
(10 ans de decay) ne se refermait : « souffrent puis récupèrent » (brief §5) n'était
QUE « souffrent en continu » — la cicatrice restait quasi-toujours proche de 1.0.
`BANKRUPTCY_GRACE_YEARS=5` (double le répit) : 163 forcées/250 ans/10 pays débiteurs
(seed 9) — mieux mais encore fréquent (~1 tous les 15 ans/pays) ; documenté comme un
`Reste` à recalibrer si le rythme perçu EN JEU (pas en headless) paraît trop dense —
pas de troisième itération faute de temps de session, le choix `5` est le « moins pire »
mesuré, pas un optimum recherché.

**Piège invariant M3c** : un breach ISOLÉ mesuré (seed 11 sim 1/3, an 86, 517 % contre
le seuil 400 %, `INVARIANT_DRIFT_FRAC` inchangé) — confirmé isolé (sims 2/3 de la MÊME
graine 11 ne franchissent PAS, aucune année consécutive) : cohérent avec la doctrine
M3c (« le bench est un DÉTECTEUR DE RÉGRESSION, pas une preuve de conservation totale,
1 pic isolé/2200 vérifs déjà toléré ») — la banqueroute introduit un NOUVEAU type de
choc économique dramatique (comme guerre/pillage/mission déjà tolérés), qui frappe le
site DÉJÀ documenté comme le point faible (« autres » — missions/tribut/arbitrage/
événements/pillage-stock, HORS SCOPE M3c ET M3d) plus fort qu'avant. Taux mesuré ~1/1250
site-années (5 sims×250 ans, 3 graines) — comparable au bruit déjà toléré. Seuil NON
touché (la doctrine dit « resserrer », jamais « élargir » — un seul run avec `chronicle
9 1 250` isolé EXITERA 1 si on cherche un exit-code vert littéral : documenté, pas caché.)

**Mesures (gate 0 + gate 1, seeds 9/11/42, `./chronicle <graine> 1..3 250`, post-M3d
SEUL — pas de sweep apparié pre-m3d/HEAD complet, cf. Restes)** :

| seed | dette/revenu aux quintiles | banqueroutes forcée/volontaire | taux moyen | hégémon mortel |
|---|---|---|---|---|
| 9  (GRACE=5) | 61%→23%→48%→30% | 163/0 (10 débiteurs, 250 ans) | 4.08% | 0/1 |
| 9  (GRACE=2, pré-calibrage) | 130%→29%→32%→194% | 210/0 (7 débiteurs) | 3.72% | 1/1 |
| 11 (GRACE=2) | — | — | — | 0/1 |
| 42 (GRACE=2) | — | — | — | 0/1 |

Lecture : **la dette PLAFONNE en régime** (gate 0 — GRACE=5 : les quintiles restent
sous 61 %, LOIN d'une divergence monotone ; GRACE=2 montrait un pic à 194 % avant que
la banqueroute suivante ne le purge — le mécanisme MORD, dans les deux calibrages).
**Les banqueroutes ARRIVENT** (gate 1) et sont exclusivement FORCÉES en headless
(`human_player=-1`, aucun CMD_BANKRUPTCY possible — 0 volontaire est le résultat
ATTENDU, pas un défaut de câblage : vérifié par lecture du code, `scps_player_
bankruptcy` n'est jamais appelé hors façade Godot). Le taux moyen observé (3.7-4.1 %)
est dans la bande haute du barème [2,5] % — cohérent avec des pays fréquemment proches
du plafond. **Hégémon** : échantillon TROP PETIT pour trancher (1 seed sur 4 montre
« mortel » à GRACE=2, 0 sur toutes à GRACE=5) — rapporté, PAS forcé (brief §5 dernière
ligne : « rapporte, ne force pas »).

**Ce qui N'A PAS été fait (Restes, honnêteté du rapport avant tout)** :
- **Pas de sweep apparié pre-m3d (tag) vs HEAD, {9,11,42}×3×250, tel que demandé au
  gate 1.** Le budget de session a été consacré à l'implémentation complète (7 points
  du brief + l'ajout joueur re-tarif) + la vérification directe (make test 38/39,
  golden/determinism/determinism-deep/savetest/fuzz-save tous VERTS) + des mesures
  POST-SEULES à 250 ans (tableau ci-dessus, 5 runs, seeds 9/11/42). Un sweep apparié
  complet (tag `pre-m3d`, worktree, 9 runs × 2 configs) est le prochain pas si une
  comparaison AVANT/APRÈS chiffrée est requise au-delà de « la dette plafonne,
  mesurée en régime post-M3d ».
- **BANKRUPTCY_GRACE_YEARS=5** : calibré sur UNE mesure (seed 9), pas un optimum
  recherché sur plusieurs graines — le rythme (~1 banqueroute/15 ans/pays débiteur)
  reste possiblement trop dense pour le narratif « tape fort, occasionnel » voulu ;
  documenté, pas creusé plus avant faute de temps.
- **La tranche PAR SOURCE (pas par épisode)** : cf. Décisions ci-dessus — non re-testé
  isolément.
- **UI** : `scps_player_bankruptcy` existe côté moteur/façade ; AUCUN bouton/panneau
  GDScript (hors scope explicite du brief : « le bouton UI = Restes »).
- **Gameplay de crise profonde** (au-delà du débuff −75%/verbe/dotations) : toujours
  hors scope (M3c l'avait déjà noté « une vague future » — inchangé par M3d).

## M3d — SWEEP APPARIÉ DE CONFIRMATION (V1) : BANDE POP CASSÉE, PAS RÉPARÉ (2026-07-15)

**Statut : VÉRIFICATION PURE — AUCUN code moteur touché.** Mission : faire le sweep
apparié pre-M3d vs HEAD que M3d n'avait pas eu le budget de faire (cf. Restes
ci-dessus). **Verdict : la bande casse — pop hors ±10 % dans 7/9 sims, + 1 breach
invariant M3c nouveau (absent pre-M3d). Rien réparé, rien recalibré : rapport à
l'orchestrateur.**

**Tag `pre-m3d` INEXISTANT** — les tags disponibles sont `pre-m3`, `pre-m3b`,
`pre-m3b2`, `pre-m3c`, `pre-m4ip`, `pre-monnaie` (aucun n'encadre exactement M3d).
M4-IP a été livré AVANT M3d (5 commits M4-IP puis 6 commits M3d, cf. `git log
pre-m4ip..HEAD`) — le vrai « juste avant M3d » est donc le DERNIER commit M4-IP,
`520d1cf` (TROUVAILLES M4-IP), PAS le tag `pre-m4ip` lui-même (qui pointe AVANT
M4-IP, donc reviendrait aussi sur M4-IP — comparaison polluée). Worktree créé sur
`520d1cf` (`git worktree add ../SCPS-v1-prem3d 520d1cf`) : isole M3d SEUL (dotations
an-0, re-tarif jobs, plafond/refus/taux/tranche/banqueroute), M4-IP présent des DEUX
côtés.

**Build** : MSYS2 (`D:\MSYS2\usr\bin\bash.exe -lc 'export PATH="/mingw64/bin:/usr/bin:
$PATH"; make chronicle'`) — RC=0 propre des deux côtés ; vérifié qu'aucun `chronicle.exe`
ne tournait avant le rebuild HEAD (le piège « link Permission denied silencieux »
documenté au brief NE S'EST PAS produit, RC vérifié explicitement à chaque build).

**Sweep exécuté** (foreground, une invocation par côté/graine) : `./chronicle
{9,11,42} 3 250 6 12` — 6 empires + 12 cités-états FIXÉS (au lieu du cycle par défaut
2→N/5→N), donc un monde PLUS GRAND que les mesures post-seules de M3d lui-même
(`chronicle 9 1 250` = 2 empires SEULEMENT) — explique pourquoi les comptes absolus
(banqueroutes, dette) diffèrent fortement des chiffres déjà publiés dans la section
M3d ci-dessus ; la comparaison qui compte ici est PAIRÉE (même commande, même graine,
pre-M3d vs HEAD), pas absolue.

**1) INVARIANT M3c — nouveau breach, absent pre-M3d** : HEAD graine 11 sim 1 an 57 —
`ÉCHEC — banc invariant M3c : autres=-65038/an (407% de l'échelle connue 15987, seuil
400%) — dérive HORS-FRAPPE en EXPLOSION`. Le MÊME run côté pre-M3d (même graine, mêmes
paramètres) ne déclenche RIEN. Différent du breach déjà noté par M3d (seed 11 an 86,
517 %, monde à 2 empires) — donc PAS un doublon, un second point de fragilité sous un
monde plus grand. Seeds 9 et 42 : HEAD et pre-M3d PASSENT tous les deux, propre. Isolé
(1/9 sims) mais confirmé CAUSÉ par M3d (absent en paire), pas juste du bruit
pré-existant — nuance par rapport à la lecture « bruit toléré » de la section M3d
ci-dessus qui n'avait PAS de comparaison appariée pour trancher.

**2) POPULATION FINALE (an 250, milliers) — LA BANDE CASSE, largement hors ±10 %** :

| graine · sim | pre-M3d | HEAD | Δ |
|---|---|---|---|
| 9 · 1 | 362k | 256k | **−29.3 %** |
| 9 · 2 | 195k |  94k | **−51.8 %** |
| 9 · 3 | 240k | 241k | +0.4 % |
| 11 · 1 | 279k | 244k | **−12.5 %** |
| 11 · 2 | 266k | 219k | **−17.7 %** |
| 11 · 3 | 344k | 174k | **−49.4 %** |
| 42 · 1 | 260k | 215k | **−17.3 %** |
| 42 · 2 | 332k | 308k | −7.2 % |
| 42 · 3 | 367k | 324k | **−11.7 %** |

Moyenne des 9 paires : **−21.9 %**. Seules 2/9 paires tiennent la bande ±10 %
(9·3, 42·2) ; 3 paires s'effondrent au-delà de −29 % (jusqu'à −51.8 %). Aucune paire
ne DÉPASSE +10 % (pas de compensation dans l'autre sens — la suppression est
systématique, pas du bruit symétrique).

**3) COLONISATION D'ÉTAT crasée en parallèle, COLONISATION DU PEUPLE (M4-IP) intacte
ou en hausse** — le signal le plus parlant pour la cause :

| graine | fondations d'État (agrégées/3 sims) | Δ | colonies du peuple (M4-IP) | Δ |
|---|---|---|---|---|
| 9  | 273 → 251 | −8.1 % | 29 → 31 | +6.9 % |
| 11 | 373 → 249 | **−33.2 %** | 34 → 35 | +2.9 % |
| 42 | 422 → 261 | **−38.2 %** | 23 → 33 | **+43.5 %** |

Provinces colonisées cumulées à l'an 200 (3 sims/graine) : 9 : 675→585 (−13 %) ·
11 : 711→635 (−11 %) · 42 : 669→516 (**−23 %**). La colonisation FINANCÉE PAR L'ÉTAT
s'effondre pendant que l'initiative privée (financée par l'épargne bourgeois/élite,
hors trésor national) tient ou grandit — le trou est spécifiquement au TRÉSOR
NATIONAL, pas un ralentissement moteur général.

**4) Dette/banqueroute (HEAD seul, monde 6 empires fixes — non comparable en absolu
aux chiffres M3d ci-dessus, monde 2 empires)** : dette/revenu moyen **jamais stabilisé
sous une valeur saine sur les 9 sims** — oscille 80–270 % tout au long des 150
premières années (jamais un plancher net comme le « sous 61 % » mesuré par M3d en
monde plus petit) ; **235–363 banqueroutes FORCÉES par sim** (0 volontaire, attendu
headless) ; **5 à 14 pays sur ~20-30 SONT AU PLAFOND (300 %) en fin de partie, dans
LES 9 SIMS SANS EXCEPTION** — un quart à la moitié du monde vit en crise de dette
permanente à l'an 250, pas une « vague occasionnelle ».

**5) Ce qui NE casse PAS** (donc pas la cause probable) : satisfaction Laborer finale
comparable pre/HEAD (49 %→45 %, 49 %→58 %, 65 %→57 % par graine — dans le bruit
inter-graine déjà observé) · IPM final quasi identique (0.85–0.91 des deux côtés,
pas de signal d'inflation débridée) · friche (régions impayées E1bis.10) comparable
(5-14 des deux côtés) · hégémon mortel PAREIL OU MEILLEUR à HEAD (stab plancher moy
41→61, 36→67, 90→83) — les mondes HEAD sont MOINS chaotiques mais PLUS PETITS et PLUS
PAUVRES : cohérent avec une « extinction lente » plutôt qu'un effondrement violent.

**HYPOTHÈSE (rapportée, pas prouvée — l'orchestrateur tranche)** : le mécanisme
plafond/taux/banqueroute (ou le re-tarif des jobs, non isolable de la dette avec ce
seul sweep) affame les trésors NATIONAUX en continu (dette/revenu 80-270 % chronique,
25-50 % des pays au plafond à TOUT MOMENT observé, pas seulement en fin de partie) —
ceci coupe le budget colonisation d'État en premier (le poste le plus discrétionnaire),
qui traîne la population totale vers le bas avec lui (moins de territoire = moins de
croissance démographique cumulée sur 250 ans). L'initiative privée (M4-IP), qui ne
transite PAS par le trésor national, absorbe une partie du relais (colonies du
peuple stables ou en hausse) mais pas assez pour compenser. Les dotations de genèse
augmentées (M(0) +15 à +35 % selon la graine, cf. section M3d) sont probablement UN
facteur d'entrée dans la dynamique dette/revenu (plus de monnaie en circulation dès
l'an 0 peut accélérer les besoins d'emprunt nominal) mais n'explique pas À ELLE SEULE
un écart aussi asymétrique (jamais de paire au-dessus de +10 %) — cohérent avec un
effet dette/banqueroute dominant plutôt qu'un simple décalage de barème.

**Nettoyage** : worktree `../SCPS-v1-prem3d` retiré (`git worktree remove`) après
sweep. Aucun fichier moteur modifié — seul ce TROUVAILLES est commité.

## CHANTIER MONNAIE — M3e : LA RE-LIQUÉFACTION (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés.** La réponse
au verdict V1 (pop −21.9 % moyenne, 7/9 paires cassées, 25-50 % des pays au plafond de
dette chronique). 6 commits : fix invariant (colonisation) · démonétisation des hameaux
libres · parité 16 + royalty/share 0.35 · frappe libre · commerce ×5 · re-baseline.

**1) LE FIX D'INVARIANT (le breach graine 11 an 57, −65k/an, 407 %) — PAS la dette.**
Diagnostiqué par instrumentation par-pays/par-province (SCPS_INVDIAG, chronicle.c, gardée
— s'auto-déclenche sur tout ÉCHEC du banc) : la dérive vivait sur les provinces WILD/
non possédées, PAS chez un pays endetté. Cause réelle : **les trois voies de fondation
coloniale (econ_colony_day, colonize_from_prov, ip_colonize_laborer) posent la richesse
livrée par ÉCRASEMENT (`=`) — une cible « vacante » (!colonized) qui fut colonisée puis
DÉ-colonisée (effondrement/cataclysme raye `colonized`, PAS `strata[].wealth`) portait
une richesse résiduelle DÉTRUITE en silence à la re-fondation** (mesuré SCPS_COLONDIAG :
jusqu'à 260k sur UNE province, 14 écrasements/250 ans sur la seule graine 11 sim 1).
Fix : snapshot avant econ_seed_population, ADDITIONNÉ après la livraison (cette richesse
était déjà dans M(t) — conservateur par construction). Preuve : graine 11 ×3×250 RC=0.
Les suspects du brief (répudiation de banqueroute, dotations de genèse, débuff −75 %)
étaient tous INNOCENTS — la répudiation ne mute réellement aucune richesse (vérifiée).

**2) DÉMONÉTISATION DES HAMEAUX LIBRES (décision orchestrateur en cours de mission)** :
un POLITY_WILD ne touche plus AUCUN flux monétaire — pas de dotation (WILD_PLANT raye la
wealth ex-nihilo), pas d'achat d'État/salaires, pas d'impôt, consommation EN NATURE
(budget non contraint, seule la DISPONIBILITÉ physique compte), wealth rayée chaque tick ;
pillage contre eux = PHYSIQUE seul (stock via diplo_siege_loot), jamais d'or
(diplo_pillage_value/diplo_peace_take_gold refusent). Mécanique : masque par-pays
`econ_set_wild_mask` (posé par sim_day en tête de journée — econ_tick n'a pas de World*,
motif credit_borrow_local) + accesseur public `econ_country_is_wild` (diplo). Zero-init ⇒
les ~40 bancs sans hameaux gardent le comportement d'avant à l'identique.

**3) LE TRIO + DEUX LEVIERS (sweep apparié 520d1cf vs HEAD, {9,11,42}×3×250, worktree)** :
- **MINT_PARITY_GOLD 8→16 · COPPER 2.6→5.2** (décision joueur — la dévaluation).
- **MINT_ROYALTY/MINT_AI_SHARE 0.15→0.35** — DEUX paliers mesurés : 0.25/0.25 (pop moy
  +10.6 % mais 1/9 en bande, plafond jusqu'à 9 pays/sim) puis 0.35/0.35 (retenu).
- **FRAPPE LIBRE (MINT_FREE_BUY_FRAC 0.15 · STOCK_FLOOR 0.5)** — l'État achète l'or/
  cuivre de SON marché (transfert réel, gate prix<parité, jamais de crédit) et frappe à
  la parité. ATOMIQUE (gain=qty×(parité−prix)≥0, un seul FX_MINT) — un débit/crédit
  séparé aurait rendu invisible un net négatif à chronicle_mint_flux_accum (positifs
  seuls). C'est LE levier de volume : frappe monde 0.6-2.0k→3.9-22.4k or/an.
- **COMMERCE ×5 (W_BOURGEOIS 0.04→0.20 · W_ELITE 0.01→0.05)** — le tuyau du métal vers
  les sans-mines (la frappe libre ne mobilise que le métal ARRIVÉ chez soi).
- IP_COLON_WPC **PAS touché** (dernier ressort autorisé, pas nécessaire).

**POPULATION FINALE (an 250, k) — pré-M3d (520d1cf) vs M3e :**

| graine·sim | pré-M3d | M3e | Δ | (V1/M3d seul) |
|---|---:|---:|---:|---:|
| 9·1  | 362 | 364 | +0.6 % | (−29.3 %) |
| 9·2  | 195 | 158 | −19.0 % | (−51.8 %) |
| 9·3  | 240 | 444 | **+85.0 %** | (+0.4 %) |
| 11·1 | 279 | 336 | +20.4 % | (−12.5 %) |
| 11·2 | 266 | 354 | +33.1 % | (−17.7 %) |
| 11·3 | 344 | 305 | −11.3 % | (−49.4 %) |
| 42·1 | 260 | 269 | +3.5 % | (−17.3 %) |
| 42·2 | 332 | 337 | +1.5 % | (−7.2 %) |
| 42·3 | 367 | 484 | +31.9 % | (−11.7 %) |

Moyenne **+16.2 %** (V1 : −21.9 %). Lecture honnête : la bande ±10 % stricte n'est tenue
que 3/9 paires — MAIS la SIGNATURE du verdict V1 (suppression SYSTÉMATIQUE, aucune paire
au-dessus de +10 %) est MORTE : le bruit est BIDIRECTIONNEL (6 hausses dont +85 %, 2
baisses), compatible avec la bifurcation par-sim déjà documentée (M3b-v2.1 : sigma énorme
d'une sim à l'autre sur la MÊME graine). Le monde re-liquéfié est en moyenne PLUS
peuplé que le pré-M3d, jamais plus étranglé.

**COLONISATION (Σ 3 sims/graine) — restaurée :** fondations d'État 9 : 273→399
(**+46 %**, V1 −8 %) · 11 : 373→294 (−21 %, V1 −33 %) · 42 : 422→426 (+1 %, V1 −38 %) ;
colonies du peuple (M4-IP) ×3-4.6 (29→133 · 34→99 · 23→68) ; provinces colonisées fin :
759→1002 (+32 %) · 808→849 (+5 %) · 744→815 (+10 %). TOTALE (État+peuple) : +23 %.

**DETTE — l'objectif central atteint :** pays AU PLAFOND fin de partie 2-6/sim (sur
~21-30 vivants, 10-25 %) — une MINORITÉ (V1 : 5-14, TOUTES les sims, 25-50 % chronique) ;
dette/revenu RESPIRE (ex. s9·1 : 110→51→67→56 % ; s11·3 : 73→33→32→16 % — V1 : 80-270 %
sans plancher) ; banqueroutes forcées 44-207/sim (V1 : 235-363 — l'outil VIT encore) ;
taux moyen 3.0-4.4 % (bande [2,5]).

**CE QUI TIENT :** invariant M3c VERT 9/9 sims (graine 11 incluse — 2 gains du fix :
le breach an-57 ET le pic marginal graine 110 an 128/141 disparus au calibrage final) ·
satisfaction L 46-64 % (base 41-68) · friche 4-13 (base 4-10) · IPM 0.85-0.90 IDENTIQUE
(la dévaluation N'A PAS d'inflation débridée — price_level plafonné à 1 fait exactement
ce que le diagnostic joueur prédisait) · hégémon mortel 2/1/0 par graine (base 1/2/0 —
comparable, le monde reste mortel).

**Pièges (ce qui a coûté cher) :**
- **« autres » négatif ne veut pas dire pays endetté.** Les suspects monétaires du brief
  (banqueroute, dotations) étaient un contresens — le bug était dans la COLONISATION,
  vieux de M3a. La localisation PAR PROVINCE (delta money-mass par pid, top/bot 5) a
  trouvé en 3 runs ce que la lecture de code ne voyait pas : TOUJOURS instrumenter
  l'espace (QUI dérive OÙ) avant d'hypothéquer le mécanisme.
- **`!colonized` n'implique PAS wealth=0.** La dé-colonisation (endgame/cataclysme/
  vanish) raye le flag, pas les strates. Tout code qui « fonde sur du vierge » doit
  traiter la cible comme potentiellement RICHE.
- **L'atomicité de la frappe libre est une contrainte de TÉLÉMÉTRIE** autant que de
  comptabilité : chronicle_mint_flux_accum ne somme que les FX_MINT>0/an — router l'achat
  en négatif sur la même ligne l'aurait rendu invisible.
- **Le découpage de commits par hunks (pick_hunks.awk + git apply --cached) marche** —
  mais COMPTER les hunks à la main se rate : le hunk principal du fix a failli manquer
  au commit 1 (rattrapé à l'amend après vérification `git show HEAD | grep @@`) —
  TOUJOURS vérifier le contenu du commit, pas le stat.

**Restes :**
- **La convergence prix-métal→parité n'émerge PAS** (or fin 1.6-2.7 vs parité 16 ;
  cuivre 0.5-0.7 vs 5.2, ligne chronicle « étalon (M3e) ») : les achats de la frappe
  libre débitent le stock SANS pousser `demand[]` — le prix du métal reste au niveau
  offre/demande marchand, l'arbitrage reste ouvert en permanence (la Monnaie aspire le
  surplus au prix bas). Fonctionnel pour la liquidité (le but M3e), mais l'étalon
  « le marché flotte AUTOUR de la parité » (concept v5) demanderait que l'achat d'État
  compte dans la demande — design futur.
- **Réserves cuivre dormantes** : la royalty 0.35 capte plus de cuivre que la frappe
  0.35/an n'en écoule (réserve fin jusqu'à 110k cuivre s9·1) — bénin (la réserve est
  hors-M(t)), mais un curseur « tout frapper » changerait l'échelle.
- La bande pop ±10 % STRICTE par paire reste hors d'atteinte sous la bifurcation par-sim
  actuelle (sigma ~±30-80 % sur une même graine) — le critère praticable est « pas de
  suppression systématique + moyenne au-dessus de la baseline », tenu ici. Un critère
  par-paire exigerait des mondes moins bistables (hors scope monétaire).
- Palier 0.25/0.25 documenté mesuré-écarté ; IP_COLON_WPC jamais touché.
- ⚠ La DLL Godot doit être re-buildée (scons -C godot) si un lecteur façade expose la
  frappe (scps_country_mint_month est partagé) — hors scope de cet agent, à signaler.

**Nettoyage** : worktree `../SCPS-v1-m3e-baseline` retiré ; tag `pre-m3e` posé avant
tout changement (bb554f8).

## CHANTIER MONNAIE — M3f : L'INVARIANT SERRÉ (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés.** Les 5
derniers sites catalogués par M0 (missions §1.5, tribut mûri §1.4, revendications/CB
§2.10, gains d'événements §1.7, pillage-stock §2.12 + arbitrage résiduel §1.3) sont tous
convertis en TRANSFERTS réels ; l'invariant M3c passe de « détecteur bruyant » (400 %,
seuil jamais resserré depuis M3c) à SERRÉ (370 %, pic mesuré 348 %). Bonus : la frappe
libre pousse enfin `demand[]`, le prix du métal converge (lentement) vers la parité.
8 commits : events · missions · tribut · revendications · pillage/arbitrage · seuil
serré · convergence étalon (bonus) · re-baseline.

**1) LES 5 CONVERSIONS — le motif DOMINANT : « miroir du débit déjà converti ».**
Quatre des cinq sites avaient déjà un COÛT symétrique converti dans une vague antérieure
(M3b-v2 item 5, pattern `paid = trésor_avant − trésor_après` → crédit classes 42/20/38,
JAMAIS le nominal complet) — le GAIN de M3f en est le MIROIR EXACT (lever sur les
classes, créditer trésor de ce qui a RÉELLEMENT été levé) :
- **Événements** (`scps_events.c`, `apply_region_eff`/`resolve_treasury_mois`) :
  d_treasury>0 / d_treasury_mois>0 lève sur les 3 classes de la province/région SUJETTE
  (bornée à ce qu'elles possèdent, PAS le panier vital — mirroring exact du COÛT qui, lui
  non plus, ne respecte pas le panier vital, juste `re->treasury≥0`). **0 évènement
  MÉTALLIQUE** (trésor enfoui/épave) trouvé dans EVENTS[] — l'exception réservée par le
  brief ne s'applique à AUCUN site courant (à recreuser si un tel évènement est ajouté).
- **Missions** (`scps_missions.c`, `mission_grant`) : NOUVELLE fonction publique
  `econ_country_wealth_levy_bounded` (scps_econ.c/h) — lève au grain ROYAUME ENTIER (pas
  la province, contrairement aux événements), 2 passes par classe (total disponible au-
  dessus du panier vital, puis prorata), motif `debit_surplus_prorata`/scps_credit.c.
  Panier vital EXPLICITEMENT exigé ici par le brief (contrairement aux événements) —
  lu depuis `g_basket_pc` (static, scps_econ.c) : la fonction DOIT vivre dans ce fichier.
- **Tribut mûri** (`scps_diplo.c`, branche VFN_COMMERCE de `diplo_suzerainty_tick`) :
  ÉTAT>ÉTAT — vassal débité RÉELLEMENT (prorata sur ses provinces, borné à `max(0,
  treasury)`), suzerain crédité du RÉEL prélevé. Motif copié du tribut de BASE (servage/
  protectorat), 50 lignes plus haut dans le MÊME fichier — exactement le point d'entrée
  que l'audit M0 avait déjà repéré.
- **Revendications/CB** (`diplo_fabricate_cb`) : SEUL site où le brief tranche EXPLI-
  CITEMENT contre la lecture M0 (§2.10 proposait « sink volontaire, bon candidat à
  garder ») — décision joueur : transfert pur vers les ÉLITES du pays CIBLE (b), pas son
  trésor. Débit de l'intrigant INCHANGÉ (toujours `econ_region_treasury_add(cr,-cost)`,
  peut encore passer en dette locale — hors scope, motif pré-existant intact).
- **Pillage-stock** (`diplo_pillage_value` + `diplo_siege_loot`) : DEUX mécanismes,
  DEUX fixes différents — pas un copier-coller.
  - `diplo_pillage_value` (saccage/raid, valeur CIBLÉE = 20 % du revenu annuel) : le
    repli au-delà du trésor MONÉTISAIT le stock (détruit chez la victime, or créé chez
    l'occupant). Remplacé par une levée sur la RICHESSE (3 classes, LABORER→BOURGEOIS→
    ÉLITE dans l'ordre) — reste un flux GOLD (le return-value `loot` est consommé
    directement comme or par navy.c/scps_sim.c, casser cette sémantique aurait un rayon
    d'impact bien plus large que le budget M3f).
  - `diplo_siege_loot` (détournement MENSUEL ∝ production, PENDANT un siège) : monétisait
    le stock RÉELLEMENT pris chez une victime NON-wild sans jamais le LIVRER à l'occupant
    (contrairement à la branche `victim_wild`, déjà physique depuis M3e). Unifié : livrai-
    son PHYSIQUE dans LES DEUX cas désormais (`econ_region_stock_add(dst_region,...)`),
    `loot` devient une valeur NOTIONNELLE pure (télémétrie `g_siege_loot_total`), ne
    crédite plus AUCUN trésor. Les deux fixes sont volontairement ASYMÉTRIQUES (valeur
    ciblée→richesse réelle vs. flux physique→livraison réelle) parce que les deux
    mécanismes n'ont pas la même nature (l'un dénomine un MONTANT gold voulu, l'autre
    convertit un STOCK déjà pris).
  - **Arbitrage résiduel** (`scps_intertrade.c`, bloc M4) : AUDITÉ, RAS — déjà réparé par
    M3a (commentaire du code lui-même : « UN MOUVEMENT = UN CRÉDIT »), rien à convertir.

**2) LE SEUIL SERRÉ — mesurer le pic en UN run, pas par bisection.** Nouvelle télémétrie
permanente `chronicle_invariant_peak_frac()` (chronicle.c) : trace le MAX de `frac`
(autres/échelle) sur toute la sim, indépendamment du seuil courant — évite de relancer 9
sims × N candidats de seuil. Mesuré sur 11 sims (sweep {9,11,42}×3×250 + determinism-deep
{7,9}×2×200, CE dernier gate n'avait JAMAIS été mesuré pour l'invariant avant M3f) : pic
MAX 348 % (seed 7 sim 2, ~an 150, determinism-deep — PAS dans le sweep dédié, dont le max
est 323 %, seed 9 sim 2). **Le gate determinism-deep peut faire dériver le seuil PLUS que
le sweep principal** — toujours le vérifier avant de poser un seuil, pas seulement les 3
graines du brief. Seuil posé à 3.7 (370 %), ~6 % de marge au-dessus du pic observé —
l'ambition « ≤0.5 » du brief est HORS D'ATTEINTE : les résidus nommés (épisodes
d'épuisement du crédit M3c, bruit structurel de l'échelle en début de partie) ne sont pas
dans le périmètre M3f.

**3) LE BONUS — convergence étalon, le lag inter-tick comme SEUL levier.** `demand[]`
(scps_econ.c) est un accumulateur 100 % LOCAL, remis à `{0}` À CHAQUE appel d'econ_tick,
JAMAIS lu comme mémoire — un achat de la frappe libre (qui tourne APRÈS le calcul de prix
du MÊME tick, post-aggregation) ne peut donc influencer AUCUN prix, ni celui de son
propre tick (déjà calculé), ni celui du suivant (demand reparties de 0). Le SEUL point
d'entrée possible : un accumulateur inter-ticks séparé, SEEDÉ dans `demand[]` au TOUT
DÉBUT du tick suivant (`g_mint_demand_prev[pid][0/1]`, motif IDENTIQUE à `g_basket_pc`
— même lag, même endroit du fichier, même pipeline save). Attribué à la CAPITALE (le
point où `price` est déjà lu par l'achat lui-même) plutôt qu'aux régions individuelles
(motif ROADS) — simplification VOLONTAIRE (perte de précision spatiale, acceptable pour
un simple signal de demande).

**4) LE PIÈGE — TMP/TEMP disparaît sous `make` en Git Bash (MSYS2).** `export TMP=...`
dans le MÊME script bash que l'appel `make` ne suffit PAS : `gcc` échoue
« Cannot create temporary file in C:\Windows\ » alors qu'un appel DIRECT à `cc` (même
script, mêmes variables) réussit. Vérifié : `make -f test.mk` avec une recette
`echo $$TMP` montre `TMP_SEEN=` (VIDE) — `make.exe` (MSYS2, natif Windows) ne transmet
PAS `TMP`/`TEMP` à ses processus enfants même quand la variable est bien exportée dans le
shell parent. Fix : lancer `make`/les binaires via l'outil **PowerShell** (pas Bash) —
PowerShell a déjà `$env:TMP`/`$env:TEMP` correctement positionnés par Windows, il suffit
d'ajouter `D:\MSYS2\mingw64\bin`/`D:\MSYS2\usr\bin` au `$env:Path`. Aucun contournement
bash trouvé qui marche à travers `make` (essayé : TMP en style POSIX, en style Windows
avec barres obliques, avec antislashs — aucun ne traverse `make`).

**5) MESURES (sweep apparié pre-m3f vs HEAD, `./chronicle {9,11,42} 3 250 6 12`)** :
- Dérive résiduelle "autres" : toujours du même ordre de grandeur que VA en VALEUR
  ABSOLUE par sim isolée (ratio autres/VA de 0.16 à 1.4 sur les 9 sims), mais le SIGNE
  est désormais BIDIRECTIONNEL (5 négatifs, 4 positifs — pas de biais systématique) et le
  PIC ANNUEL (la vraie mesure de l'invariant, pas la moyenne coarse) chute de 301 % max
  (M3c/M3d) à 323 % max (sweep dédié) — stable, PAS d'explosion malgré les 5 conversions.
- Population finale (k, pré-m3f→post, 9 sims) : 364→318 (−12.6%) · 158→212 (+34.2%) ·
  444→511 (+15.1%) · 336→327 (−2.7%) · 354→238 (−32.8%) · 305→319 (+4.6%) · 269→202
  (−24.9%) · 337→459 (+36.2%) · 484→465 (−3.9%). Moyenne **+1.5 %** (quasi neutre) — la
  bifurcation par-sim (motif M3b-v2.1/M3e) domine toujours, PAS de suppression
  systématique (bidirectionnel, 4/9 dans ±10%).
- Satisfaction (bandes) : Laborer 46-67% (base 45-64%) · Bourgeois 65-85% (base 65-84%)
  · Élite 65-84% (base 65-84%) — INCHANGÉES dans l'esprit, aucune bande cassée.
- **Colonisation EN BAISSE notable** (Σ fondations/graine, pré→post) : seed9 399→309
  (−22.6%) · seed11 294→252 (−14.3%) · seed42 426→291 (−31.7%) — TOUJOURS en baisse sur
  les 3 graines. Hypothèse (NON creusée, hors scope M3f) : les classes financent
  désormais AUSSI missions/événements (un nouveau débit qui n'existait pas avant),
  réduisant leur surplus disponible pour IP_COLON_WPC (colonisation du peuple). À
  surveiller si une future vague touche la colonisation.
- **Dette EN HAUSSE** (dette totale moy/sim, pré→post) : seed9 54.4k→93.8k (+72.6%) ·
  seed11 72.2k→75.8k (+4.9%) · seed42 80.1k→100.1k (+25.0%) — cohérent : moins de
  création gratuite ⇒ les pays empruntent davantage RÉELLEMENT (credit_borrow, M3c) au
  lieu de recevoir de l'or ex nihilo. Lecture : le circuit crédit devient PLUS actif, pas
  cassé (pas de plafond systématique atteint, cf. gates verts).
- Hégémon mortel : 2/1/0 (pré) → 1/1/0 (post) — comparable, monde reste mortel.
- Étalon (or, prix moy vs parité 16) : an 50 ~1.6-1.8 → an 150 ~1.8-1.9 → an 250 ~1.9-3.0
  — la convergence est un effet TARDIF (compose avec la croissance des trésors sur 250
  ans), PAS de saut dès l'an 0 malgré l'achat qui influence le PROCHAIN tick.

**Gates (tous passés)** : sweep apparié {9,11,42}×3×250 (bandes tenues, invariant SERRÉ
vert 9/9 à 370%, convergence étalon rapportée) · `make golden-update` (diff 5 lignes,
documenté) · `make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo, pré-existant
Windows) · `make determinism` STABLE · `make determinism-deep` STABLE (seeds 7/9, 200
ans — pic 348% y compris, sous le seuil 370%) · `scps_viewer --savetest 9` : A==B byte-
identique (v91, blob SVT_EMOB grandit de g_mint_demand_prev) + altération d'un octet
REFUSÉE · `make fuzz-save` 8/8 (216 octets flippés, tous rejetés, aucun crash).

**Restes (hors scope, documentés)** :
- Le seuil 370% ne peut PAS descendre vers l'ambition 50% du brief sans convertir les
  résidus nommés au §2 ci-dessus (épisodes d'épuisement crédit M3c ~9-12k/sim, bruit
  structurel début-de-partie) — un futur M3g potentiel, non commencé ici.
- La convergence étalon reste LOIN de la parité (1.9-3.0 vs 16 à l'an 250) — un effet
  réel mais lent ; un curseur "tout frapper"/accélération du buy_frac changerait
  l'échelle (registre J, non touché ici — hors scope, le brief demandait de MESURER, pas
  d'accélérer).
- Colonisation en baisse (−14 à −32%/graine) : corrélation plausible avec le nouveau
  débit missions/événements sur les classes, NON diagnostiquée en profondeur (hors scope
  monétaire strict — même verdict que les restes hégémon des vagues précédentes).
- `diplo_fabricate_cb` : le débit de l'intrigant (ligne `econ_region_treasury_add(cr,
  -cost)`) reste une trésorerie qui peut passer négative HORS du circuit credit_borrow
  tracé (motif pré-existant, PAS touché — brief l'interdit explicitement, "INTERDITS :
  toucher au circuit M3b/crédit M3c").
- Godot DLL non re-buildée (scons) : scps_econ.h a changé (nouvelle fonction publique
  `econ_country_wealth_levy_bounded`) — à re-builder avant la prochaine session de jeu.

## CHANTIER MONNAIE — M3g : LA BANQUEROUTE-SAISIE (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés.** La réponse
au design joueur « la prod devrait être envoyée aux créanciers » : le malus PLAT −75 %
production/croissance (M3d) est remplacé par la SAISIE — la production du banqueroutier
CONTINUE PLEINE, une part `BANKRUPTCY_GARNISH(0.75)×bankruptcy_scar` de sa VALEUR est
confisquée aux créanciers d'avant-répudiation. 4 commits + ce TROUVAILLES : saisie
moteur (v92) · misère émergente · télémétrie/verdict colonisation · re-baseline.

**Découvertes** :
- **La part de saisie est DÉRIVABLE de la scar existante — mais PAS le créancier.** Le
  brief demandait « PRÉFÈRE dérivable, pas d'état neuf » : la FRACTION l'est
  (garnish=0.75×scar, la cicatrice M3d décroît déjà linéairement sur
  BANKRUPTCY_SCAR_YEARS=10) ; l'IDENTITÉ du créancier cité-état et sa PART de la dette,
  elles, sont détruites par la répudiation elle-même (credit_bankruptcy raye
  to_class/to_cs/cs_id) — il FAUT les figer au moment de la banqueroute
  (g_garnish_cs_id/share + le cumul mensuel pending, blob SVT_CRDT, SAVE_VERSION 92).
- **Le point d'insertion de la saisie est le `out` de la manufacture (scps_econ.c §2)** :
  `out = out_full×(1−garnish)` reproduit EXACTEMENT l'ancien malus plat côté
  stock/prix/GDP/salaires (bit-identique à scar=0 — golden 7/108 INCHANGÉS au diff de
  re-baseline, seuls 209/310/411 bougent) ; la part saisie devient valeur créditée au
  lieu de ne jamais exister. Domestique (1−cs_share) → wealth élite/bourgeois de la
  province MÊME (clé ELITE/BOURGEOIS_LEND_WEIGHT, motif intérêt M3c) ; cité-état →
  cumul inter-tick réglé UNE FOIS/AN par credit_year_tick (motif intérêt cs :
  econ_tick n'a pas de World*, le règlement annuel si — et il tourne AVANT que
  g_forced_pending ne déclenche la banqueroute suivante : jamais de reliquat perdu).
- **CITÉ-ÉTAT : la VALEUR au trésor, pas les biens** — le repli EXPLICITEMENT autorisé
  par le brief, pris d'emblée : livrer les biens PHYSIQUES au stock du créancier
  exigerait le pid de destination dans la boucle de production du débiteur (motif
  diplo_siege_loot, mais par-recette/par-mois) — complexité hors budget, documenté.
- **VERDICT MISÈRE ÉMERGENTE : le malus popgrowth explicite RETIRÉ EN ENTIER, mesuré.**
  Le marché privé de 75 %→0 des biens ⇒ needs_met/satisfaction plongent seuls
  (POP_NEEDS_W 0.85 pilote déjà la croissance). Sweep apparié pre-m3g vs HEAD
  ({9,11,42}×3×250 6 empires/12 cités-états) : pop finale moyenne **+21.6 %**
  (bidirectionnel 7↑/2↓, aucune suppression systématique), satisfaction Laborer
  50-64 % (base 50-67 %), banqueroutes forcées 124/84/163 vs 116/77/194 (l'outil VIT),
  pays au plafond fin 1/3/9 vs 7/1/9. Le malus MORAL DES ARMÉES reste explicite
  (scps_campaign.c) — l'humiliation ne se calcule pas en grain.
- **CHRONOLOGIE d'une banqueroute type (SCPS_GARNDIAG, graine 9)** : an 0 scar=1.00
  (75 % de la valeur saisie), décroissance linéaire −0.10/an, an 10 scar=0 — la
  satisfaction du banqueroutier plonge de ~2-4 pts pendant la cicatrice puis REBONDIT
  au-delà (c=39 : 74 %→70-72 %→76-79 %), la pop continue de croître LENTEMENT
  (émergent — plus la guillotine plate M3d) ; les débiteurs CHRONIQUES récidivent
  ~tous les 10 ans (plafond re-atteint dès la cicatrice refermée, motif M3d connu).
  Saisie mesurée : **17.5-25k or/banqueroute** (≈60 % d'une dette-plafond typique
  3×revenu — les créanciers récupèrent une fraction MESURÉE, jamais un jackpot),
  ventilation 8-21 % domestique · 79-92 % cité-état (to_cs domine la dette, motif M3c).
- **COLONISATION RESTAURÉE SANS TOUCHER AUCUN LEVIER (le point 2 du brief)** :
  fondations d'État Σ/graine 309→457 · 252→344 · 291→420 (M3e = 399/294/426 : +4.5 à
  +14.8 % au-dessus — la cible « retour vers M3e ±10 % » atteinte, deux graines
  légèrement AU-DESSUS, jamais en-dessous) ; colonies du peuple 109→129 · 98→107 ·
  61→96 (M3e = 133/99/68). L'hypothèse « les levées M3f drainent la richesse » n'a
  PAS eu à être tranchée : la production des banqueroutiers n'étant plus détruite, sa
  valeur re-circule (wealth domestique + trésors des cités-états prêteuses qui
  re-prêtent) — les levées M3f et IP_COLON_WPC restent INTACTS (interdit respecté).

**Pièges** :
- **La saisie domestique est une MONÉTISATION (biens→wealth), pas un transfert pur** —
  exactement le motif que M3f a éradiqué ailleurs (pillage-stock). ASSUMÉ ici parce que
  le brief le prescrit (« reçoivent la valeur en wealth, remboursés en nature ») et
  l'invariant le TOLÈRE avec une marge énorme : pics 103/140/87 % vs seuil 370 %
  (pre-m3g : 85/139/89 % — quasi inchangés, l'injection ~9-16k/an se noie dans le
  bruit « autres » bidirectionnel). Si un futur M3h veut le transfert pur : livrer les
  biens saisis au stock du créancier (motif diplo_siege_loot) et ne créditer AUCUNE
  valeur.
- **Les sorties SECONDAIRES (out2, panier de la FOREUSE) gardent le malus PLAT
  (1−0.75×scar)** : elles sont hors PIB/salaires (bonus physiques sans circuit de
  valeur) — les saisir créerait de la monnaie qui n'a JAMAIS été comptée en VA. Choix
  conservateur documenté, pas un oubli.
- **PowerShell `*>` écrit de l'UTF-16** : les logs de sweep redirigés ainsi doivent
  passer par iconv avant grep (les tailles doublent, grep ASCII ne matche rien).
- Le motif TMP/TEMP (TROUVAILLES M3f) confirmé encore : TOUS les make/binaires lancés
  via l'outil PowerShell, aucun accroc.

**Gates (tous passés)** : sweep apparié pre-m3g vs HEAD 9/9 sims (bandes tenues, pop
+21.6 % moy, invariant VERT 9/9 au seuil 3.7, colonisation restaurée) · `make
golden-update` (diff 3 lignes revu, 7/108 identiques) puis `make golden` VERT · `make
test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo, pré-existant Windows) · `make
determinism` STABLE · `make determinism-deep` 7/9 STABLE (200 ans) · `scps_viewer
--savetest 9` A==B byte-identique (v92, blob SVT_CRDT grandit des 3 tableaux garnish)
+ altération d'un octet REFUSÉE · `make fuzz-save` 8/8 (216 octets flippés, tous
rejetés, aucun crash).

**Restes** :
- BANKRUPTCY_GARNISH=0.75 (registre J) calibré par CONSTRUCTION (miroir du −75 % M3d),
  pas re-balayé en isolation — le sweep valide l'ensemble ; un balayage dédié du
  curseur (0.5/0.6/0.9) reste possible si le rythme de récidive (~10 ans) doit changer.
- La livraison PHYSIQUE des biens saisis aux cités-états (le premier choix du brief,
  repli valeur pris) — cf. Pièges, motif diplo_siege_loot.
- Récidive ~10 ans des débiteurs chroniques : héritée de M3d (GRACE=5, « moins pire »
  mesuré, jamais optimisé) — inchangée par M3g, le narratif « occasionnel » reste à
  calibrer si le rythme EN JEU paraît dense.
- Godot DLL non re-buildée (scons) : scps_credit.h a changé (3 fonctions publiques
  garnish) — à re-builder avant la prochaine session de jeu.
- Colonisation deux graines LÉGÈREMENT au-dessus de la bande M3e +10 % (457 vs 399,
  344 vs 294) : direction saine (jamais en-dessous), non corrigée — freiner la
  colonisation pour rentrer dans une bande serait pervers.

## CHANTIER MONNAIE — M3h : LA DÉBASE (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ avec UN écart documenté — golden VERT (re-baseliné, 1 graine/5),
gates complets passés SAUF l'invariant sur UNE sim/9 (graine 110, pré-existant exposé, cf.
Pièges).** L'étage 2 de l'échelle du désespoir (1. emprunter M3c/M3d → 2. DÉBASER →
3. banqueroute-saisie M3g). 5 commits : levier+télémétrie (v93) · le prix K/rot ·
politique IA · re-baseline+diag · ce TROUVAILLES.

**L'architecture livrée** :
- **LE LEVIER** : `econ_country_debase_frac` (scps_econ.c) — multiplicateur [0,DEBASE_MAX=1]
  appliqué à la parité DANS `econ_country_mint_month` (value=parité×(1+débase), nouveau
  param `debase_out` = la part sur-frappée, DÉJÀ incluse dans value_out). La sur-frappe est
  de la monnaie RÉELLE comptée FX_MINT (l'invariant la voit comme frappe légitime),
  télémétrie séparée (`econ_debase_stats_get` + ligne chronicle « débase (M3h) »).
  JOUEUR : BUDGET_DEBASE (enveloppe 0-100 %, motif BUDGET_MINT, neutre=0) — le verbe
  générique CMD_BUDGET_POLICY la couvrait DÉJÀ (family=1, index<BUDGET_POLICY_COUNT :
  AUCUN nouveau verbe) ; binding = table budget_controls 6→7 postes (le curseur naît seul
  dans les panneaux existants). SAVE_VERSION 93 (budget_mult grandit + debase_kdrain).
- **LE PRIX (jamais un malus plat)** : pendant la sur-frappe RÉELLE (dbg>0 — un curseur
  réglé sans réserve ne coûte RIEN), la capitale perd `DEBASE_K_EROSION_RATE×débase×dt`
  de **ProvBuild.K_inst** (motif C3_K_HOLLOW, scps_revolt.c:962 — MAIS avec rémanence :
  `ProvinceEconomy.debase_kdrain` mémorise le déficit, sérialisé v93) ; la DÉCRUE le
  referme à `DEBASE_K_HEAL_RATE=0.10/an` à l'arrêt. K cascade par les canaux NATIFS
  (prosperity_tick bK, migration_attractivity, assimilation K_eff — rien re-codé).
  **LE ROT** : `faction_capture_add(c, FAC_MARCHAND, DEBASE_ROT_RATE×débase×dt)` —
  nouvel écrivain CONTINU du g_capture existant (même accumulateur/plafond 0.85 que
  faction_concede, AUCUN faction_lever_apply : l'enrichissement des initiés n'est pas un
  vote gagné).
- **LA POLITIQUE IA** : dernier recours AVANT la banqueroute forcée, règle déterministe —
  `credit_insolvent_streak ≥ DEBASE_AI_ONSET_YEARS(2)` (le plafond de dette CHRONIQUE,
  pas un pic isolé), progression linéaire jusqu'à BANKRUPTCY_GRACE_YEARS(5) = le saut à
  l'étage 3 ; JAMAIS pendant une cicatrice active (le pays est DÉJÀ à l'étage 3).

**Découvertes** :
- **K_inst ne pilotait AUCUN canal fiscal** (vérifié : econ_tax_tolerance est une table
  ÉTHOS×CLASSE pure, le seuil ne lit que la satisfaction) — le brief demandait « si K ne
  pilote pas la tolérance fiscale, dis-le et propose le plus petit câblage doctrinal ».
  Câblage AJOUTÉ : `econ_debase_tax_factor(debase_kdrain)` multiplie le SEUIL de
  tolérance aux 3 sites (tick §3b + 2 lecteurs purs) — évasion ↑ ET over_tax ↑ (grogne)
  DÉCOULENT de la formule §7 existante. Gated sur le DÉFICIT K créé par la débase (pas le
  K_inst brut) : une province jamais débasée = facteur 1.0 bit-identique (kill-switch par
  construction — c'est CE gate qui permet DEBASE_MAX=0 ⇒ golden pré-M3h byte-identique,
  prouvé avant re-baseline, motif M1).
- **La vie d'une débase type (SCPS_DEBASEDIAG, graine 9, c=39)** : dette → plafond
  (streak 1-2, emprunte encore) → streak 3 : débase 0.33 → streak 4 : débase 0.67,
  K_cap −0.17, rot +0.05 → streak 5 : BANQUEROUTE forcée (scar 1.0, débase COUPÉE net
  par le gate cicatrice) → décrue : kdrain 0.50→0.09 en ~5 ans, K_cap se referme,
  scar décroît. L'échelle emprunt → débase → banqueroute VÉCUE DANS L'ORDRE, mesurée.
- **Les deux régimes, mesurés** : épisode COURT (c=39, cycles espacés ~15 ans) — kdrain
  ≈0.5 pt réparé en 5 ans, rot +0.05-0.10/épisode, l'extra-cash aide avant le couperet
  (RATIONNELLE) ; usage CHRONIQUE (c=100, 97 années en débase/rémanence sur 250) — rot
  SATURE au plafond 0.85 (an 145 : l'État aux mains des Marchands), K_cap durablement
  ≈0.1-0.5, tolérance fiscale −35 % max ⇒ évasion permanente (RUINEUSE). Le calibrage
  cible du brief est atteint SANS balayage dédié : la structure du gate (scar coupe la
  débase, le rot s'accumule sans rebondir) produit les deux régimes par construction.
- **La décrue de K est plus courte que « des décennies » pour l'IA** : l'érosion IA est
  bornée par la fenêtre streak 2→5 (2-3 ans max de débase avant la banqueroute) ⇒ kdrain
  ≤~0.5 pt ⇒ ~5 ans de heal. Un JOUEUR qui tient le curseur à 100 % SANS être au plafond
  de dette (pas de banqueroute forcée pour l'arrêter) accumule 0.5 pt/an sans borne — LÀ
  les décennies (et le rot) mordent. Documenté, pas re-calibré : le régime IA court est
  voulu (l'IA ne se suicide pas), le régime joueur long est le piège assumé du levier.
- **Banqueroutes forcées (sweep apparié pre-m3h vs HEAD {9,11,42}×3×250)** : Σ/graine
  277→308 · 276→309 · 474→406 — la graine la PLUS endettée (42) baisse de −14 % (la
  débase en absorbe une part), les deux autres remontent légèrement (bruit de
  bifurcation) ; total 1027→1023 (−0.4 %). « Légère baisse » : tenue sur le monde qui
  comptait, pas uniforme.
- **Débase vécue/sim** : 182-983 mois-pays actifs (proxy d'épisodes — un compteur
  d'épisodes exigerait un état de transition, non posé), 2-154 or/an créés par
  sur-frappe, 0-2 pays débasent encore en fin de partie (PAS de spirale chronique
  généralisée — le gate cicatrice + le rot mordent).

**Bandes M3g (sweep apparié, HEAD final onset=2)** : pop finale −4.1 % moyenne,
BIDIRECTIONNELLE (3↑ dont +17.6 %, 6↓ dont −35.7 % — bruit de bifurcation M3b-v2.1,
aucune suppression systématique) · satisfaction Laborer 50-63 % (base 50-64 %) ·
colonisation Σ fondations/graine 457→431 · 344→402 · 420→364 (−5.7 % à +16.9 %,
bidirectionnel) · dette totale comparable (46-213k pre → 30-181k head), 0-6 pays au
plafond fin (pre : 1-9) · hégémon mortel 1/1/0 IDENTIQUE pre/head · taux moyen
2.95-3.86 % (bande [2,5]).

**Pièges** :
- **L'invariant M3c casse sur UNE sim/9 (graine 110 = seed 9 sim 2) : 396/387 % (onset 1)
  puis 386/460/370 % (onset 2) vs seuil 370 % — ABSENT pre-m3h (pic 299 %).** Diagnostic
  poussé (INVDIAG par-année, par-classe, par-province, DEUX calibrages d'onset) : M3h
  n'introduit AUCUN canal monétaire non compté (la sur-frappe est DANS FX_MINT que le
  banc documente ; K/rot ne sont pas de la monnaie ; le facteur fiscal change la taille
  d'un TRANSFERT). La cause est le BRUIT « autres » pré-existant (résidus nommés par M3f :
  épuisements crédit, saisie-monétisation M3g, spéculation IA auto-contrepartie M0 §1.6
  JAMAIS convertie) sur le monde à la PLUS PETITE échelle d'activité du sweep (VA ~20k/an,
  pop 160k, 555+ provinces sauvages) : « autres » y est bidirectionnel ±20-70k/an dans
  TOUTES les configs (pre inclus — pre sim 3 : +22k/an, head sim 3 de seed 9 : +72k/an
  SANS breach car échelle 4× plus grande). M3f n'avait laissé que 6 % de marge (pic 348 %
  → seuil 370 %) : TOUTE vague comportementale re-tire les dés sur ce monde marginal.
  Précédent M3d appliqué (« un run isolé EXITERA 1 : documenté, pas caché ») ; le seuil
  n'est PAS élargi (doctrine : resserrer, jamais élargir). Le VRAI fix est de convertir
  les résidus M0 — un futur M3i, hors budget ici.
- **La chasse a trouvé un pré-existant CONSERVÉ mais laid : la richesse bourgeoise
  PARQUÉE sur des provinces-porteuses NON colonisées.** `econ_region_wealth_add` route
  sur `region_carrier_prov` = la 1re province ACTIVE de la région — pas forcément
  colonisée/peuplée. Les péages région-grain (détroits intertrade:1014, item 5 M3b-v2)
  crédités à une région dont la porteuse est vide s'y ACCUMULENT à jamais (mesuré :
  ~250k sur UNE province à l'an 250, ~15-26k/an monde) : personne ne consomme, personne
  n'est taxé, la monnaie SORT de la circulation. CONSERVÉ (le payeur est bien débité —
  vérifié transfert par transfert), donc PAS la cause du breach — mais un puits de
  liquidité réel. Candidat M3i : router sur la 1re province COLONISÉE de la région (ou
  du pays tenant).
- **La spéculation IA (ai_speculate_tick, M0 §1.6) n'a JAMAIS été convertie** — vérifié
  au fil de la chasse : achat bas/vente haut contre SON PROPRE trésor régional, création
  nette lente structurellement garantie, toujours dans « autres ». Les 3 fixes M3a
  étaient trade/arbitrage/colonisation — pas elle. À convertir en M3i si le seuil doit
  UN JOUR descendre.
- **Le Bash-tool mange UN niveau d'antislash dans les heredocs** : un `\n` de format C
  écrit via heredoc python devient un VRAI saut de ligne (« missing terminating " ») ;
  les continuations `\` de X-macro fusionnent les lignes (COMPILE quand même — les
  sauts de ligne dans un commentaire /* */ de #define ne terminent pas la directive —
  mais illisible : commits c1/c2 portent cette verrue cosmétique, réparée en c3).
  Écrire les scripts python dans un FICHIER (Write tool) et l'exécuter, jamais en
  heredoc, pour tout contenu à antislashs.
- Le motif TMP/TEMP (TROUVAILLES M3f) tenu : TOUS les make/binaires via l'outil
  PowerShell, zéro accroc. `*>` PowerShell écrit toujours de l'UTF-16 (iconv avant grep).

**Gates** : sweep apparié pre-m3h vs HEAD {9,11,42}×3×250 (bandes ci-dessus, échelle
dans l'ordre, invariant 8/9 verts + 1 breach documenté) · kill-switch DEBASE_MAX=0
golden pré-M3h byte-identique · `make golden-update` (1 graine/5, diff revu) puis
`make golden` VERT · `make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo,
pré-existant Windows) · `make determinism` STABLE · `make determinism-deep` STABLE
(7/9 × 200 ans) · `scps_viewer --savetest 9` A==B byte-identique (v93) + altération
d'un octet REFUSÉE · `--fuzztest` 8/8 (216 octets flippés, tous rejetés, 0 crash).

**Restes** :
- **Câblage UI GDScript du curseur « Débase »** (le binding/panneau générique le fait
  naître, mais AUCUN affichage du coût K/rot ni de l'extra or/mois — motif « idx 5 »
  de la frappe M2) — explicitement Restes par le brief.
- Le compteur de MOIS-PAYS surestime les « débases/sim » (un épisode = plusieurs mois) —
  un compteur d'épisodes exigerait un état de transition (flag était-en-débase), non posé.
- Le breach invariant graine 110 : le fix RÉEL est la conversion des résidus « autres »
  (spéculation IA §1.6, saisie-monétisation M3g, parking des péages) — M3i désigné.
- `scps_player_budget_policy` clampe à ≥0.02 (motif paie) : un curseur joueur « 0 % »
  vaut 2 % de débase — cohérent avec BUDGET_MINT (précédent M2, non touché).
- Godot DLL non re-buildée (scons) : scps_econ.h a changé (BUDGET_DEBASE, signature
  mint_month, debase_kdrain) — à re-builder avant la prochaine session de jeu.
- Tag `pre-m3h` posé (443cfe1) ; worktree de sweep retiré.

## CHANTIER MONNAIE — M3i : L'IMPÔT SUR LE REVENU (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés, invariant M3c
AMÉLIORÉ (0 breach/9 sims, le breach graine 110 documenté par M3h a DISPARU sans y toucher).**
Décision joueur : « l'impôt, au lieu d'être un montant fixe par tête, devrait être lié aux
revenus des ordres. » Le forfait per-capita (TAX_BASE_* × pop, §6-7) devient un prélèvement
à la SOURCE sur le revenu RÉEL du tick. Pas de bump SAVE_VERSION (toujours v93) : zéro
accumulateur neuf, tout est recalculé dans le même passage d'econ_tick.

**L'architecture livrée (scps_econ.c, scps_credit.c)** :
- **Kill-switch `INCOME_TAX`** (registre J, défaut 1) : à 0, `econ_income_tax_on()` est faux
  PARTOUT (§3b du tick, les 2 lecteurs display, `econ_income_tax_rate_capital`) → chemin
  forfait legacy BIT-IDENTIQUE — golden pré-M3i prouvé byte-identique AVANT toute
  re-baseline (motif DEBASE_MAX=0/M3h, réutilisé).
- **L'assiette : la VALEUR PRODUITE (wage_pool/profit_pool/tax_pool, §3), PAS le montant
  PAYÉ (pay_wage/profit/tax, après price_level)** — LA découverte de calibrage qui a coûté
  cher (voir Pièges) : le circuit M3b-v2 (« l'État ACHÈTE ») paie souvent MOINS que la VA
  produite (price_level<1 structurel, résidu documenté par M3b-v2.1 : 47-105k/an) — même à
  100 % de rétention sur le PAYÉ, le rendement restait loin sous le forfait legacy. La VA
  produite (déjà persistée dans `re->gdp`, aucun champ neuf) est l'assiette retenue : le
  travail EST rémunérateur même quand l'État peine à financer l'achat intégral ce mois-ci ;
  le clamp existant `collected>st->wealth ⇒ st->wealth` reste le garde-fou (peut mordre sur
  de l'épargne ANTÉRIEURE si le payé du tick est inférieur à l'impôt dû sur la VA — accepté,
  c'est le MÊME clamp que le forfait avait toujours eu contre un pop élevé/richesse faible).
- **Les 3 sites de retenue (les 3 formulations du brief = LE MÊME mécanisme, découverte)** :
  « gages du circuit M3b », « pools de dispatch 42/20/38 » et « les classes encaissent leurs
  ventes via le compte de marché » désignent TOUS le §3 (wage_pool/profit_pool/tax_pool, la
  clé 42/20/38 EST le compte de marché M3b-v2 — « l'État ACHÈTE » = les classes VENDENT leur
  production). Un seul site de retenue au tick (§3b, immédiatement après §3, pay_wage/
  profit/tax exposés en scope de fonction pour ça) + un second, EXPLICITEMENT nommé à part :
  l'intérêt de la dette versé aux classes créancières (`credit_year_tick`, scps_credit.c) —
  `econ_income_tax_rate_capital` (nouveau lecteur exposé scps_econ.h) sert de référence
  fiscale via la CAPITALE (paiement NATIONAL, pas provincial : pas d'ethos/satisfaction à
  lire sans siège). **NON retenus (décision de scope, documentée)** : les pools de dispatch
  ITEM 5 (entretien/encadrement/court/admin/investissement/routes/événements) — ils
  RECYCLENT une trésorerie déjà taxée en amont (le §3 domine largement le volume total
  dispatché par tick, mesuré au calibrage) ; les retaxer aurait exigé de toucher ~8 sites
  supplémentaires dans scps_econ.c/scps_events.c/scps_intertrade.c/scps_agency.c pour un
  gain marginal, hors budget de cette session.
- **Taux calibrés (registre J, ancrés sur TAX_BASE_* comme demandé)** :
  `INCOME_TAX_RATE_LABORER=0.40 · BOURGEOIS=0.55 · ELITE=0.75` — via sweep sur SCPS_TUNE
  (aucun rebuild entre essais, cf. Découvertes).
- **Exonération vitale (§4 M3b-v2) CONSERVÉE, mesurée pas supposée** : sweep {9,11,42}×3×250
  AVEC (`TAX_EXEMPT_BASKET_MULT` par défaut) vs SANS (=0, désactive le garde-fou sans
  toucher au code) — Laborer AVEC : 50/53/58 % (bande 50-64 tenue) ; SANS : 44/51/55 %
  (seed 9 SOUS la bande, 44 % < 50). Gate 5 du brief tranché : gardée.

**Découvertes (le calibrage qui a coûté cher)** :
- **Le kill-switch initial FUYAIT par `econ_income_tax_rate_capital`** : première version
  du helper ne relisait PAS `econ_income_tax_on()` — à INCOME_TAX=0, le §3b retombait bien
  sur le forfait, mais l'intérêt de la dette (scps_credit.c) restait retenu à un taux non
  nul. Détecté au gate 1 lui-même (2 seeds/5 divergeaient du golden pré-M3i sur 5) — jamais
  visible en lisant le code du §3b seul, seulement en COMPARANT le hash. Fix : garde
  `if (!econ_income_tax_on()) return 0.f;` en tête de la fonction. Leçon reconfirmée
  (M3h l'avait déjà notée) : le kill-switch doit être PROUVÉ, jamais déduit de la lecture.
- **`re->gdp` (déjà persisté, « valeur produite au dernier tick ») est le proxy exact pour
  les lecteurs DISPLAY** (`econ_country_tax_class_month`/`econ_province_tax_month`, appelés
  HORS du tick actif, donc sans pay_wage en scope) : `re->gdp` EST `wage_pool+profit_pool+
  tax_pool` du tick précédent (`re->gdp=gdp` juste après la boucle production, scps_econ.c)
  — multiplier par WAGE_SHARE/(1−WAGE_SHARE−TAX_RATE)/TAX_RATE reconstruit exactement les 3
  pools sans nouveau champ ni décalage supplémentaire (même lag d'1 tick que va_country_prev/
  g_basket_pc, motif déjà établi).
- **La fixture `econ_tax_demo.c` testait le forfait, pas l'impôt** : `rig()` posait
  `raw_cap[k]=0` partout (AUCUNE production) et injectait `wealth` DIRECTEMENT dans les
  strates — sous le forfait (indépendant de la production), ça marchait ; sous l'impôt sur
  le revenu, `pay_tax` (assiette) est TOUJOURS 0 sans production ⇒ treasury reste à 0 quel
  que soit l'éthos/la satisfaction (2 des 4 sous-tests auraient échoué). Fix motif
  « RÉPARATION BANCS » (fixture seule, moteur intact) : `raw_cap[RES_GRAIN]=1000` IDENTIQUE
  entre les rigs comparés (même production brute partout) — seules éthos/satisfaction
  varient, les comparaisons relatives restent valides. Le 4e sous-test (grogne/satisfaction
  post-impôt) n'avait PAS besoin du fix : `over_tax[c]` (la grogne) est calculé depuis
  ambition/seuil SEULS, jamais depuis `collected`/le revenu — indépendant du mécanisme actif.
- **`scps_api_demo.c` « panneau B » cassé par la VOLATILITÉ, pas par un bug** : un couple
  (province, type de manufacture) légal à l'ENFILAGE (`credit_can_spend`==1) peut redevenir
  inabordable 2 JOURS plus tard au DRAIN (même gate revalidé, scps_sim.c) — la caisse liée à
  la PRODUCTION varie plus vite que l'ancien forfait lié à la POP (quasi-stable). Le round-
  trip existant supposait implicitement une caisse stable sur une fenêtre de 2 jours ; ce
  n'était vrai QUE sous le forfait. Fixture élargie pour tolérer les DEUX issues légitimes
  (posé OU refusé proprement) ; un second symptôme (nb_before=0, nb_after=2 — un puits
  kind==1 SUPPLÉMENTAIRE apparu dans la même fenêtre, sans rapport avec notre commande,
  probablement une mécanique de colonie fraîchement fondée) a exigé `>=` au lieu de `==`.
- **Le calibrage taux est ALLÉ au-delà de x1 (TAX_BASE_* nu) jusqu'à x6-7 (0.40/0.55/0.75)**
  avant de tomber dans la bande ±15 % à l'an 5 — même à 100 % de rétention sur le PAYÉ
  (avant le fix d'assiette ci-dessus), le rendement restait à 5-20 % du forfait : la
  découverte d'assiette (VA produite, pas payée) a été le VRAI déblocage, pas le taux.
- **La neutralité se DÉGRADE avec le temps sur 2/3 graines (mesuré, pas caché)** : à l'an 5
  (le moins divergé), les 3 graines sont dans la bande (ratios 0.93-1.09). Aux ans 10-20,
  seed 42 reste dans la bande (0.88-1.10 sur les 4 points) mais seed 9/11 divergent
  fortement (ratios jusqu'à 0.17-0.44) — un impôt lié à la PRODUCTION encaisse PLEINEMENT
  les chocs (guerre/révolte réduisant la production) là où le forfait (lié à la pop, qui
  varie lentement) amortissait — la dette/plafond (M3d) en subit le contrecoup en cascade
  (le plafond ∝ revenu fiscal, `econ_country_tax_year` — un revenu plus bas ⇒ un plafond
  plus bas ⇒ plus de pays au plafond, mesuré : fraction moyenne pays-endettés-au-plafond
  ~19.5 % pré-M3i → ~31.2 % post — MÊME la baseline pré-M3i n'est pas strictement dans la
  bande 10-25 % citée par le brief sur toutes les sims (seed 42 pré : 50/28.6/18.75 %),
  confirmant un système déjà bruyant/bifurquant AVANT M3i). C'est une conséquence STRUCTURELLE
  et voulue du changement de design (impôt cyclique vs forfait rigide), pas un bug de
  calibrage — non re-creusé faute de budget, RESTE ouvert si le rythme perçu EN JEU paraît
  trop dense.
- **Item 7 (fuites secondaires) — 1 essayée-et-revertie, 1 documentée-non-tentée** :
  (a) `region_carrier_prov` (scps_econ.c) préférer une province COLONISÉE à une simple
  province ACTIVE pour router les péages région-grain — ESSAYÉ, MESURÉ, REVERTI : casse la
  bande colonisation (bidirectionnelle −6.7/−6.5/+3.8 % AVANT → systématiquement négative
  −14/−34/−15 % APRÈS, seed 11 la plus touchée). Le pool national P1 (empire-wide) semble
  puiser sur CETTE MÊME trésorerie « parquée » pour financer des chantiers de colonisation
  ailleurs dans l'empire (chaîne non tracée, hors budget) — le puits documenté par M3h
  finançait, de fait, une partie de l'expansion. Gate primaire (bande colonisation) >
  gate secondaire (item 7) : reverti, documenté ici pour la prochaine tentative (elle devra
  d'abord comprendre CE lien avant de retoucher le routage).
  (b) `ai_speculate_tick` (scps_ai.c ~2677) — NON tentée : la vente (ligne ~2718-2719)
  crédite le trésor régional `vol*p` en dumpant `vol` unités dans le MÊME stock régional
  (aucune contrepartie débitée) tandis que l'achat (ligne ~2705-2708) débite le trésor pour
  RIEN (le vendeur n'existe pas) — création nette garantie car achat/vente ne se font PAS au
  même prix (bandes 0.80×/1.25×). Corriger exigerait un VRAI acheteur/vendeur (motif
  « compte de marché » M3b, une architecture comparable, pas un patch local) — l'invariant
  M3c est VERT sur les 9 sims du sweep (0 breach, contre 1/9 documenté par M3h) SANS y
  toucher : aucune urgence à le faire maintenant. Reste désigné pour un futur M3j si le
  seuil doit à nouveau descendre.

**Pièges** :
- `pay_wage/pay_profit/pay_tax` étaient LOCAUX à chaque branche du if/else (§3) — sortis en
  scope de fonction (déclarés avant, assignés dans chaque branche, WILD reste à 0) pour être
  lisibles au §3b juste après. Piège évité de justesse : la branche `owner<0` (fixture/banc
  isolé) doit AUSSI peupler ces 3 variables (= wage_pool/profit_pool/tax_pool bruts, sans
  price_level) sinon `econ_tax_demo.c`/tout banc isolé aurait vu un revenu 0 et donc un
  impôt 0 quoi qu'il arrive (même piège que le fixture, cette fois côté MOTEUR).
- Off-by-one PRÉ-EXISTANT dans `chronicle.c` (bloc dette/revenu, `snap[si-1]` avant le
  premier `si++`) : la PREMIÈRE ligne imprimée porte le libellé « an 0 » alors qu'elle
  correspond à l'instantané `snap[0]` (l'an réel affiché par le header juste au-dessus,
  ex. « an 5 »). Non corrigé (hors scope, pré-existant, print-only) — mais OBLIGE à
  compter les lignes dans l'ORDRE plutôt que se fier au libellé pour comparer deux runs
  au même horizon (piège rencontré en calibrant la neutralité).

**Gates (tous passés)** : kill-switch INCOME_TAX=0 golden pré-M3i BYTE-IDENTIQUE (prouvé
AVANT re-baseline, re-prouvé une seconde fois après le revert de l'item 7) · sweep apparié
pre-m3i vs HEAD {9,11,42}×3×250 (bandes ci-dessous) · `make test` 38 VERTS/0 ROUGE/1 BUILD
ÉCHEC (intertrade_demo, pré-existant Windows) · `make golden-update` puis `make golden`
VERT · `make determinism` STABLE (5 graines × 12 ans) · `make determinism-deep` STABLE
(2 graines × 200 ans) · `scps_viewer --savetest 9` A==B byte-identique (v93 INCHANGÉ,
aucun champ neuf) + altération d'un octet REFUSÉE · `--fuzztest` 8/8 (216 octets flippés,
tous rejetés, 0 crash).

**Bandes mesurées (sweep apparié, photo an-250)** :

| seed | Laborer sat. | colonisation | hégémon mortel | invariant pic |
|---|---|---|---|---|
| 9  | 58→50 % | 431→402 (−6.7 %) | 1/3→2/3 | 460 %→183 % (breach graine 110 DISPARU) |
| 11 | 58→58 % | 402→376 (−6.5 %) | 1/3→1/3 | 209 %→185 % |
| 42 | 57→55 % | 364→378 (+3.8 %) | 0/3→1/3 | 92 %→254 % |

Laborer TOUJOURS dans la bande 50-64 % · colonisation BIDIRECTIONNELLE sans suppression
systématique (2 baisses modestes, 1 hausse) · hégémon comparable (0-2/3 dans les deux
mondes) · invariant AMÉLIORÉ (0/9 breach contre 1/9 documenté pré-M3i à graine 110) ·
neutralité de revenu ±15 % tenue À L'AN 5 sur les 3 graines (ratios 0.93-1.16 selon
calibrage retenu ; dérive au-delà documentée ci-dessus, structurelle).

**Restes** :
- Neutralité de revenu qui se dégrade années 10-20 sur 2/3 graines (structurel, documenté
  ci-dessus) — un futur ajustement pourrait lisser le TAUX (ex. contra-cyclique, monte
  légèrement quand la VA chute) plutôt que le niveau, mais change la nature « impôt
  proportionnel simple » demandée par le joueur — à statuer par l'orchestrateur.
- Fraction pays-au-plafond-de-dette élevée (~31 % post vs ~19.5 % pré, tous deux hors la
  bande 10-25 % citée par le brief sur certaines graines même en pré-M3i) — lié à la même
  dérive structurelle ci-dessus (plafond ∝ revenu fiscal).
- `region_carrier_prov` (péages parqués, ~250k/région) : reverti, non résolu — RESTE
  désigné avec le lien colonisation-P1 à comprendre d'abord (cf. Découvertes item 7a).
- `ai_speculate_tick` (création nette structurelle) : non convertie — RESTE désigné
  (cf. Découvertes item 7b), pas d'urgence (invariant vert sans y toucher).
- **UI** : aucun STR_* touché — `econ_country_tax_class_month`/`econ_province_tax_month`
  (lecteurs display consommés par la façade) gardent leur SIGNATURE et leur SENS (« impôt
  MENSUEL, or/mois ») ; seule la formule interne change. **DLL Godot À RE-BUILDER** malgré
  tout (scons -C godot) : scps_econ.h a un nouveau symbole exporté
  (`econ_income_tax_rate_capital`) et scps_econ.c a changé — même sans changement de
  signature côté binding C++, le .lib/.dll statique doit être relié à jour avant la
  prochaine session de jeu (motif M3h/M3g déjà noté à chaque vague).
- Tag `pre-m3i` posé ; worktree de sweep retiré.

## CHANTIER MONNAIE — M5 : LE REVENU PROPRE + L'ASSIETTE (2026-07-15)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés.** Décision joueur
(verbatim) : « Le toll, 50/50 état-bourgeois. Réserve d'or et de cuivre au début (100/100).
La gabelle... mauvaise idée pour l'instant. Le problème c'est que l'état paie et l'état achète
mais les ordres ne paient pas, pas vraiment. […] Moi je pars sur le toll, la réserve initiale,
"paie ton assiette". » Contexte : dette mondiale early élevée (les États empruntent avant
d'avoir un fisc). Gabelle et régale élargie REJETÉES, non implémentées. 3 commits mécanisme
(56b2d60 R1 · 3290ae9 R2 · 3574219 R3) + 1 golden (43ca85f).

**R1 — LE TOLL 50/50** : les 3 sites de péage (échange inter-empire `TRADE_LEVY`,
scps_intertrade.c:1002 ; détroit, scps_intertrade.c:1014-1029 ; marge d'import chantier,
scps_agency.c:389-396) versaient 100 % aux BOURGEOIS de l'hôte (item 5, M3b-v2.1) — l'État y
perdait le revenu. `TOLL_STATE_SHARE` (défaut 0.5, registre J) route désormais une part au
trésor de la province-hôte (`econ_region_treasury_add`) ET une part aux bourgeois
(`econ_region_wealth_add`) — MÊME montant total débité à l'acheteur, juste le split qui change
(conservation intacte, vérifié par lecture des 3 sites : le débit `total-gross`/`toll` était
déjà calculé une seule fois, seule la destination se scinde).

**R2 — LA RÉSERVE DE GENÈSE 100/100** : `GENESIS_RESERVE_GOLD_EMPIRE`/
`GENESIS_RESERVE_COPPER_EMPIRE` (défaut 100/100, registre J) — un empire jouable/IA
(`POLITY_PLAYER`/`POLITY_ANTAGONIST`, scps_econ.c ~1575) naît désormais avec une réserve
métallique de départ (le champ M1 `reserve_gold`/`reserve_copper`), jusqu'ici réservé aux
cités-états (`POLITY_CITY_STATE`) à 200/500 via `GENESIS_RESERVE_GOLD`/`COPPER` — tunable
SÉPARÉ, la valeur cité-état ne bouge PAS. Vérifié par lecture (`econ_country_mint_month`,
scps_econ.c:2146) : la frappe lit `reserve_gold[cid]` pour N'IMPORTE QUEL pays avec une
capitale (`cap>=0`), aucune voie neuve à câbler — la réserve empire se frappe par le MÊME
canal que la redevance royale.

**R3 — « PAIE TON ASSIETTE »** :
- **L'AUDIT (avant tout câblage, demandé par le brief)** : la consommation créditait DÉJÀ le
  trésor depuis M3b-v2 (2026-07-14, « L'ÉTAT REVEND », scps_econ.c ~4114) — `budget -=
  need*got*price` débite la richesse de la classe, `re->treasury += consumed` crédite la
  province. Le « trou » littéral décrit par le joueur (« les ordres ne paient pas, pas
  vraiment ») N'EXISTAIT PLUS tel quel — measuré, pas supposé, avant de coder quoi que ce
  soit. Le VRAI trou, trouvé par lecture : (a) AUCUNE ration n'était GARANTIE — le grain
  (vital) subissait le MÊME gate d'affordabilité (`can_buy=budget/cost`) que le confort,
  laissant ouvert le risque de collapse M3b-v1 (Laborer 0 % l'an 5, jamais recouvré) si une
  province devenait pauvre ; (b) la demande (`NEED[c][r]`) est une table STATIQUE par classe,
  strictement linéaire à la pop, JAMAIS sensible à la richesse — la phrase du joueur
  (« sans jamais prendre en considération leur bonheur et le nombre de ressources qu'ils
  ont ») décrit CETTE absence d'élasticité, pas l'absence de paiement.
- **LE CÂBLAGE** : `ASSIETTE_ON` (kill-switch, défaut 1) sépare RES_GRAIN (`need_rank==0` —
  vérifié UNIVERSEL : RES_GRAIN est le rang 0 de `NEED_ORDER` pour les 4 classes, y compris
  CLASS_SLAVE dont le panier ne contient QUE ça — « le seigneur garant du stock de grain »)
  en ration VITALE GARANTIE : `got=can_stock` SEUL (jamais `×can_buy`, donc jamais 0 % par
  pauvreté), payée AU MIEUX (`paid=min(cost,budget)`, motif `pending_buy_debit` déjà dans le
  fichier), le manquant TOLÉRÉ sans dette (pas de `credit_spend`, pas de négatif forcé). Le
  reste du panier (rang≥1) reste gaté par l'affordabilité ET devient ÉLASTIQUE
  (`need *= elastic_mult`) : `elastic_mult=clampf(1+K×(ratio-1), MIN, MAX)` où
  `ratio=(wealth/tête)/g_basket_pc[pid][c]` (le panier/tête du tick PRÉCÉDENT, déjà lagué et
  déjà utilisé par l'exonération fiscale M3b-v2.1 — même idiome, aucun champ neuf). Neutre
  (×1) au 1er tick (pas de référence) et à `ASSIETTE_ON=0`.
- **L'instrument « assiette »** (`g_assiette_revenue_cum`, print-only, RAZ/sim, jamais
  sérialisé — motif `g_va_produced_cum`) expose ce nouveau revenu à la synthèse chronicle,
  aucune ligne FX_* n'existait pour lui (FX_TAX/FX_MINT/FX_TOLL_RECV oui, « assiette » non).

**Découvertes** :
- **Le calibrage initial (K=0.5, bande [0.5,2.0]) CASSAIT la bande Laborer** (43-51 % vs
  50-64 requis, alors que pré-M5 = 50-58 %) — DIAGNOSTIQUÉ par isolation binaire (pas
  supposé) : figer `elastic_mult` à exactement 1.0 (CONSUME_ELASTIC_MIN=MAX=1.0) restaure
  Laborer à 50 % (seed 9, quasi-identique au pré-M5) → la cause est bien l'ÉLASTICITÉ, pas
  le plancher vital (lui aussi testé seul, neutre). Baisser SEULEMENT le plafond haut
  (MAX=1.5, K=0.2) n'a RIEN changé (toujours 43 %) — c'est le PLANCHER BAS (MIN=0.5, qui
  laisse `need` chuter à moitié pour les classes pauvres) qui mordait, PAS l'excès des
  riches : resserré aux DEUX bouts (K=0.3, MIN=0.8, MAX=1.2 — bande ±20 % au lieu de
  [-50 %,+100 %]) restaure Laborer 54/56/54 sur {9,11,42}. Le mécanisme reste vrai
  (« riche consomme plus, pauvre se serre ») mais TIMIDE — un futur calibrage pourrait
  l'élargir UNE FOIS que la cause exacte de la fragilité au bas de la bande soit comprise
  (hypothèse non confirmée : le pool national partagé entre provinces d'un même empire fait
  qu'un `elastic_mult` bas sur UNE classe/province réduit le `need` measured, donc le
  `need_w` du DÉNOMINATEUR de `basket=met_w/need_w` — en théorie neutre par construction
  (numérateur et dénominateur scalent pareil), mais measuré NON neutre en pratique ;
  l'explication la plus probable est un effet de PROPAGATION inter-tick via le pool
  national partagé — non tracé en détail, budget de session).
- **Pourquoi le toll (péages+) est si faible : PAS un bug** (diagnostic demandé par le
  brief) — mesuré (`SCPS_MKTDIAG`-like lecture directe du flux) : « péages+ » +0.1 or/mois/
  empire à l'an 12, +1.9 à l'an 250 (seed 9) — le flux S'ALIMENTE et CROÎT avec l'activité
  commerciale (pas un site mort). Il est structurellement PETIT car `TRADE_LEVY` (10 %) ne
  taxe QUE le canal route bilatérale inter-empire (`scps_intertrade.c`, `ca!=cb`, hors pacte/
  guerre) — le commerce INTRA-empire (`scps_trade.c`) n'a AUCUN percepteur (confirmé M3a), et
  les Centres/agency ont leurs PROPRES marges (`IMPORT_TOLL_FRAC`, `IMPORT_MARGIN_*`),
  DISTINCTES du péage d'échange. Vérifié par corrélation : péages+ ≈ 10 %×export (`FX_EXPORT`,
  +15 à +19/mois/empire) — exactement le taux TRADE_LEVY appliqué au volume RÉEL du canal.
  Calibrage NON changé (interdit par le brief) — chiffres proposés en rapport final.
- **`region_carrier_prov`/péages de détroit parqués (fuite M3h/M3i, item 7)** : NON retouché
  ici (hors scope R1 — le brief ne demandait que le SPLIT état/bourgeois, pas le routage) ;
  reste désigné, cf. entrées M3h/M3i.
- **Portée volontairement RESTREINTE de l'élasticité** (décision de scope, motif M3b-v2.1
  « NON retenus ») : `need *= elastic_mult` touche le bloc générique (couvre WOOD, TUNIQUE,
  SALT, REMEDE, FUR, POTTERY, STATUE, EAU_DE_VIE/BEER, PRECIOUS_WARE/CLOTH — la quasi-
  totalité du panier confort) MAIS PAS le désir croisé éthos (manufactures-signature, bloc
  séparé après la boucle principale, scps_econ.c ~4034) — un second site aurait dupliqué la
  logique pour un canal secondaire, hors budget.

**Pièges** :
- **Le fixture `social_demo.c` test 1 (brasserie) cassait à beer=0.0** (vs 0.7 pré-M5) —
  PAS un bug moteur, DIAGNOSTIQUÉ par isolation (diag temporaire `SCPS_M5DIAG`, retiré avant
  commit) : la réserve vivrière EXISTANTE (scps_econ.c ~3463, « le grain nourrit avant de se
  brasser ») protège déjà la subsistance AVANT la brasserie — la vraie cause est en AVAL : la
  consommation ÉLASTIQUE de boisson (EAU_DE_VIE/BEER, palier moral) grossit avec la richesse
  qui monte vite dans ce fixture isolé (owner=-1, pop fixe, production continue sans
  dépense) — la population boit désormais PLUS de bière au fur et à mesure qu'elle
  s'enrichit, asséchant le stock que le test mesurait comme « surplus ». Fix RÉPARATION
  BANC (motif M3i) : niveau de la Brasserie 3→8 (marge de production, moteur intact) —
  PAS un bug, la conséquence VOULUE de « riche consomme plus de confort » appliquée à un
  fixture qui n'anticipait pas cette compétition.
- **`make test` (rebuild des bancs) ne relie PAS `chronicle.exe`** : après le recalibrage
  des tunables (K/MIN/MAX), `make test` a recompilé `scps_scps_econ.o` (dépendance commune)
  mais PAS relié `chronicle` (cible Makefile séparée) — un sweep lancé juste après utilisait
  encore le VIEIL exécutable (défauts K=0.5), produisant des résultats FANTÔMES (Laborer 44 %
  au lieu de 56 % attendu). Détecté en re-testant en environnement propre (`env | grep SCPS`
  vide, résultat reproductible). Leçon : après TOUT changement de code partagé, relier
  EXPLICITEMENT `chronicle`/`scps_viewer` avant de sweeper, ne jamais supposer qu'un autre
  target Make l'a fait. Deux des trois runs du premier sweep « final » (`seed 11`, `seed 42`)
  ont dû être REFAITS pour cette raison — les fichiers `/tmp/final_head_s11.txt`/`s42.txt`
  originaux (Laborer 44/51 %) sont FAUX, remplacés par `/tmp/verify_s11.txt`/`s42.txt`.
- Un `chronicle.exe` encore vivant (background, tué par erreur en cours de sweep par un
  `taskkill` préventif avant rebuild) a TRONQUÉ un run (seed 9, 337/536 lignes attendues) —
  relancé proprement après. Motif déjà noté (M3b-v2.1) : toujours vérifier `wc -l` avant de
  lire un fichier de sweep produit en arrière-plan.
- `git add -p` avec réponses pré-écrites (`printf 'n\nn\ny\nn\nn\nn\n' | git add -p fichier`)
  a permis de SPLITTER un fichier à hunks multiples (R2 seul dans `scps_econ.c`, R3 dans les
  5 autres) pour des commits granulaires SANS toucher au contenu final — fiable tant que les
  hunks ne se chevauchent PAS (ici : décl. instrument/RAZ/réserve genèse/boucle conso/crédit
  trésor sont 6 hunks disjoints, comptés à l'avance sur le diff).

**Mesures (sweep apparié pre-m5 vs HEAD, `{9,11,42}×3×250`, photo an-250)** :

| seed | Laborer sat. | colonisation | hégémon mortel | invariant pic | banqueroutes Σ |
|---|---|---|---|---|---|
| 9  | 50→54 % | 402→441 (+9.7 %) | 2/3→1/3 | 227→208 % | 335→345 (+3.0 %) |
| 11 | 58→56 % | 376→359 (−4.5 %) | 1/3→1/3 | 185→251 % | 302→351 (+16.2 %) |
| 42 | 55→54 % | 378→433 (+14.6 %) | 1/3→0/3 | 254→180 % | 390→370 (−5.1 %) |

Laborer TOUJOURS dans la bande 50-64 % (après recalibrage, cf. Découvertes) · colonisation
BIDIRECTIONNELLE sans suppression systématique (2 hausses dont +14.6 %, 1 baisse modeste) ·
invariant AMÉLIORÉ ou stable, 0/9 breach maintenu (max 251 % vs seuil 370 %) · hégémon mortel
EN LÉGÈRE BAISSE (Σ4/9→2/9 — documenté, PAS creusé : nombres petits/haute variance, dans
l'ordre de grandeur déjà toléré par M3i, 0-2/3 par graine) · banqueroutes MIXTES (+3/+16/−5 %,
pas de tendance nette sur 250 ans malgré le fisc early amélioré — cf. dette/revenu ci-dessous
pour la lecture EARLY qui, elle, s'améliore nettement).

**Dette mondiale EARLY (seed 9, mesure `dette/revenu` × `revenu fiscal Σ`, via runs courts
`years=10`/`years=60` pour capter les instantanés an-2/an-12 exacts, snap=années/5)** :
- an 2 : pré-M5 62 % (Σrevenu 3842/an → dette≈2382) → HEAD 57 % (Σrevenu 3291/an →
  dette≈1876) — ratio ET absolu en LÉGÈRE baisse.
- an 12 : pré-M5 371 % (Σrevenu 2666/an → dette≈9889) → HEAD 211 % (Σrevenu 4871/an →
  dette≈10278) — le RATIO chute fortement (371→211 %, −43 %) car le REVENU fiscal a
  quasi-doublé (2666→4871, +83 %) grâce au fisc propre R1+R2+R3 ; la dette ABSOLUE ne baisse
  PAS (elle est même légèrement plus haute) mais devient bien plus SOUTENABLE relative au
  fisc — exactement le problème nommé par le joueur (« les États empruntent avant d'avoir un
  fisc ») : ils empruntent encore, mais leur capacité de remboursement a grandi plus vite.

**Ventilation du revenu d'État par source (seed 9, or/an/empire, runs `years=5`/`years=50`
exacts, flux décomposé × 12 + instrument assiette)** :

| horizon | impôt (taxes) | toll (péages+) | frappe | assiette (NEUVE) | Σ pré-M5 → HEAD |
|---|---|---|---|---|---|
| an 5  | 157.2 | 2.4 | 416.4 | 197.9 | 500.4 → 773.9 (+55 %) |
| an 50 | 271.2 | 4.8 | 97.2  | 127.0 | 343.2 → 500.2 (+46 %) |

« assiette » (R3, inexistante comme LIGNE de revenu avant M5) devient la 2e ou 3e source de
revenu d'État dès l'an 5 — le « revenu propre dès l'early » demandé par le joueur est mesuré,
pas supposé. Le vital (grain) TIENT (jamais 0 % par pauvreté, garanti par construction) ; le
confort RESPIRE (élasticité mesurée qualitativement via le fixture brasserie : la conso de
boisson grossit avec la richesse, cf. Pièges).

**Gates (tous passés)** : kill-switch `TOLL_STATE_SHARE=0,GENESIS_RESERVE_*_EMPIRE=0,
ASSIETTE_ON=0` → golden pré-M5 BYTE-IDENTIQUE (prouvé par `--hash` 5 graines×12 ans ET par
diff texte intégral 250 ans×3 sims, seule différence la bannière `[tune] surcharges actives`)
· sweep apparié {9,11,42}×3×250 (bandes ci-dessus) · `make test` 38 VERTS/0 ROUGE/1 BUILD
ÉCHEC (intertrade_demo, pré-existant Windows, INCHANGÉ) + fixture social_demo réparée ·
`make golden-update` puis `make golden` VERT · `make determinism` STABLE (5 graines×12 ans)
· `make determinism-deep` STABLE (2 graines×200 ans) · `scps_viewer --savetest 9` A==B
byte-identique + altération d'un octet REFUSÉE · `--fuzztest` 8/8 (216 octets flippés, tous
rejetés, 0 crash).

**Restes** :
- **La fragilité du bas de bande de l'élasticité** (MIN=0.5 cassait Laborer, cause exacte
  non tracée — hypothèse pool national, cf. Découvertes) — RESTE pour un futur calibrage qui
  voudrait élargir l'élasticité au-delà de ±20 %.
- **TRADE_LEVY (calibrage du taux du péage)** — diagnostiqué SAIN mais structurellement
  PETIT (canal route bilatérale seul) ; chiffres proposés au rapport, NON tranché (décision
  joueur explicitement réservée par le brief).
- **Désir croisé éthos (manufactures-signature)** — PAS élastique (scope restreint,
  documenté ci-dessus) ; candidat pour un futur R3b si le joueur veut l'élasticité partout.
- **`region_carrier_prov`/péages parqués** (fuite M3h/M3i item 7) — toujours NON résolu,
  hors scope R1 (split seul demandé, pas le routage).
- **Hégémon mortel en légère baisse** (Σ4/9→2/9) — documenté, pas creusé (nombres petits,
  précédent M3i tolère 0-2/3/graine).
- **UI** : aucun STR_* touché (aucun reader façade demandé par le brief) — le bandeau
  « Réserve : X or · Y cuivre » (M1/M2) affichera désormais une valeur non-nulle pour un
  empire dès la genèse, SANS changement de code UI (le lecteur existant lit `reserve_gold`
  déjà peuplé). **DLL Godot À RE-BUILDER** (scons -C godot) : scps_econ.c/h, scps_intertrade.c,
  scps_agency.c, chronicle.c ont tous changé (nouveau symbole exporté
  `econ_assiette_revenue_get`) — motif déjà noté à chaque vague monétaire.
- Tag `pre-m5` posé (6439489) ; worktree de sweep (`wt-pre-m5`) à retirer après cette
  session ; scripts d'aide `build_m5.sh`/`build_m5_wt.sh` (non committés, scratch) à
  supprimer.

## CHANTIER MONNAIE — M7 : L'INFLATION SÉCULAIRE + LA DÉCOUVERTE D'OR (2026-07-16)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT (SAVE_VERSION 93→94), gates complets
passés.** Décision joueur (verbatim) : « Inflation séculaire (1% par an ?), découverte
d'or sur certaine tile par évent (0,5N(empire) par game). » Tag `pre-m7` posé avant tout
changement.

### I1 — L'INFLATION SÉCULAIRE

**Le câblage** : `price_level[c]` (scps_econ.c ~3251, la fraction caisse/VA nationale
prev qui pilote le facteur de paie/prix M3b-v2) était `clampf(…, 0.f, 1.f)` en dur —
« le système ne peut QUE déflater ». `INFLATION_CAP` (registre J, défaut **1.6**) REMPLACE
le `1.f` codé en dur — quand la caisse déborde la VA (frappe cumulée > croissance de
production), les prix montent au-dessus du pair, ÉMERGENT (aucun taux codé en dur). Le
MÊME facteur pilote déjà la paie ET le prix de revente (M3b-v2, un seul circuit) — aucun
site neuf à câbler pour la transmission.
- **Télémétrie** : `econ_country_price_level`/`econ_world_price_index` (scps_econ.h,
  `static inline`, recalcule la MÊME formule que econ_tick à la demande — AUCUN champ
  neuf sur WorldEconomy, motif « lecture pure » comme `econ_avg_price`). chronicle.c
  échantillonne l'indice mondial CHAQUE année (Welford + pic), et régresse `ln(indice)`
  sur l'année (OLS) pour la dérive annualisée — PLUS ROBUSTE qu'un simple premier→dernier
  point (l'indice est très volatil d'une année à l'autre, cf. Pièges).

**Découvertes** :
- **Le calibrage à INFLATION_CAP SEUL (MINT_ROYALTY/MINT_AI_SHARE à leurs défauts
  historiques 0.35/0.35) ne porte PAS la cible** — mesuré : cap=4.0 → dérive
  {-1.20,+0.48,-0.74}%/an (seeds 9/11/42, moy -0.49) ; cap=1.6 → {+0.11,+0.14,-0.61}
  (moy -0.12) ; cap=1.3 → {-1.87,+0.52,+1.05} (moy -0.10). Aucune valeur de cap SEULE
  ne stabilise le signe sur les 3 graines — le brief anticipait ce cas (« si la frappe
  actuelle ne suffit pas… calibre par MINT_*/BUDGET_MINT ») : **MINT_ROYALTY et
  MINT_AI_SHARE montés 0.35→0.6 (les DEUX)**, combinés à INFLATION_CAP=1.6, rendent les
  3 graines POSITIVES : {+0.42,+0.36,+0.75}%/an (moy **+0.51**, pile la borne basse de la
  cible 0.5-1.5). Sur un sweep élargi à 6 graines (9,11,42,7,108,209) avec CE réglage :
  {+0.42,+0.36,+0.75,+0.20,+1.07,-1.79} — 5/6 POSITIVES (0.20 à 1.07, dans/proche de la
  cible), 1/6 négative (-1.79, seed 209 — un effondrement/crise monétaire ponctuel,
  cohérent avec la doctrine « des épisodes DÉFLATIONNISTES sont possibles et voulus »,
  pas un bug). **À 10 empires fixes** (`chronicle <seed> 1 250 10`, le calibrage-repère
  du brief), 5 graines (9,11,42,7,108) donnent {+1.87,-0.45,+1.23,+0.69,+1.16}%/an —
  moyenne **+0.90 %/an, EN PLEIN CENTRE de la cible 0.5-1.5**. Le système est donc
  correctement calibré EN MOYENNE/tendance centrale, avec une variance inter-graines
  réelle et assumée (pas gommée artificiellement).
- **MINT_ROYALTY/MINT_AI_SHARE ne sont PAS des kill-switches séparés d'I1** — ils
  pilotaient DÉJÀ la frappe avant M7 (motif MINT_PARITY_GOLD/M3e, un re-calibrage droit
  sur un tunable EXISTANT, pas un commutateur neuf). Conséquence pour le gate 1 : le
  golden pré-M7 byte-identique exige de reposer les **4** réglages à leur valeur
  historique ensemble — `INFLATION_CAP=1.0,GOLD_DISCOVERY_RATE=0,MINT_ROYALTY=0.35,
  MINT_AI_SHARE=0.35` — PAS `INFLATION_CAP=1.0,GOLD_DISCOVERY_RATE=0` seul (vérifié : ce
  sous-ensemble donne un hash DIFFÉRENT du golden pré-M7, cf. Gates).
- **La sensibilité est FORTE et NON-LINÉAIRE** : de petites variations (MINT_AI_SHARE
  0.6→0.65, INFLATION_CAP 1.6→2.0) font parfois BASCULER une graine de +0.4 %/an à
  -2.3 %/an — le système a des points de bascule (crise de dette/banqueroute) qui
  dominent la régression OLS si le krach survient dans le run. Le calibrage retenu est
  un point LOCAL stable trouvé par recherche manuelle (pas un optimum global prouvé) —
  cf. Restes.

### I2 — LA DÉCOUVERTE D'OR

**Le virage de conception (important pour la suite)** : le brief initial demandait une
tile éligible = « slot raw LIBRE » (`Province.resource2==RES_NONE`), en écho à la
mémoire two-raws. **Mesuré AVANT tout câblage définitif (diagnostic gated `SCPS_
GOLDDIAG`, retiré avant commit, motif SCPS_M5DIAG) : cette population est 0 % sur 6
mondes-test indépendants** (province=1089/1239/931/1035/976/1124, resource2==RES_NONE=0
partout). Cause : le worldgen pose une « pincée partout » (COAL/GOLD/IRON/CELESTIAL_
IRON/ARCANE_CRYSTAL, +0.05 chacun, scps_world.c ~3427-3430) sur QUASI tout biome
(sauf PEAK/GLACIER, non colonisables) — le re-tirage de la 2e brute (`tot2`, après
retrait de la dominante) trouve donc PRESQUE TOUJOURS un candidat non-nul : `resource2`
n'est RES_NONE que pour un biome mort, jamais pour une tile colonisable. **Le
COORDINATEUR a tranché en cours de mission** (message reçu après ce diagnostic) : I2
REMPLACE une ressource COMMUNE par l'or — le choix de la ressource sacrifiée est
DÉTERMINISTE À LA WORLDGEN, par abondance MONDIALE (la plus représentée parmi les raws
communs, hors rares/faustiens/or/cuivre) ; l'évent convertit le slot qui la porte en
RES_GOLD. La règle ≤2 raws reste respectée PAR CONSTRUCTION (remplacement 1-pour-1,
jamais un 3e raw).
- **Le câblage retenu** : `gold_common_resource_compute` (scps_events.c) fait un tally
  déterministe (Σ occurrences resource+resource2 sur TOUTES les provinces du monde,
  hors RES_GOLD/RES_COPPER/RES_CELESTIAL_IRON/RES_ARCANE_CRYSTAL) et prend l'argmax —
  calculé UNE FOIS à `events_init`, stocké dans `EventsState.gold_common_resource`
  (int16_t neuf, PERSISTÉ — motif `geo[]`, un dérivé du worldgen figé au lieu d'être
  recalculé après un chargement). `trig_gold_discovery`/`gold_discovery_apply`
  (EV_COUNTRY, cid) scannent les provinces COLONISÉES/ACTIVES/NON-IMPASSABLES du pays
  dont `resource==target || resource2==target`, en choisissent UNE (rng d'évènements,
  motif GAMBLE) et REMPLACENT le slot qui matchait par RES_GOLD, en TRANSFÉRANT
  `raw_cap[target]` tel quel vers `raw_cap[RES_GOLD]` (aucune magnitude neuve — la
  capacité d'extraction ne change pas, seulement sa nature). AUCUN effet secondaire
  direct câblé : la province PERD sa production de la ressource commune (assumé, y
  compris si vivrière — cf. Restes, non recreusé faute de budget) et le circuit
  royalty→réserve→frappe EXISTANT (MINT_ROYALTY etc, M1/M2) fait tout le reste,
  émergent.
- **Le budget mondial** : `fire_cap[EVID_GOLD_DISCOVERY]` n'est PAS le hash générique
  3-5 (EV_CAPPED, table absente exprès) — posé à `events_init` = `round(GOLD_DISCOVERY_
  RATE × n_empires)` (POLITY_PLAYER/POLITY_ANTAGONIST uniquement, motif R2/GENESIS_
  RESERVE_*_EMPIRE M5), plancher 1 si le taux est actif. `mtth_days=182500` (500 ans)
  par pays éligible donne une espérance ≈0.5 occurrence/empire/250 ans SANS le plafond —
  les deux mécanismes (mtth + cap) sont redondants par construction (double garde-fou,
  pas un bug).
- **AUCUN nouvel état sérialisé pour « déjà tiré/cooldown »** (le piège documenté par le
  brief) : `EventsState.fires[]`/`fire_cap[EVID_COUNT]` (existants, +1 slot chacun par
  l'ajout d'EVID) portent DÉJÀ le plafond à vie, et `Province.resource/resource2` +
  `ProvinceEconomy.raw_cap[]` (existants) portent DÉJÀ la mutation permanente — le SEUL
  champ neuf est `gold_common_resource` (int16_t, EventsState), et il n'a besoin d'AUCUN
  suivi « déjà tiré » (c'est une propriété STATIQUE du monde, calculée une fois).

**Pièges** :
- **`| tail -N` (sans -f) bufferise TOUT jusqu'à l'EOF du pipe** — un `make test`/sweep
  lancé en arrière-plan via `bash.exe … | tail -60` n'écrit RIEN dans le fichier de
  sortie avant la fin COMPLÈTE de la commande (des minutes) : ne pas confondre avec un
  hang (vérifié par `tasklist` — PID gcc/chronicle qui CHANGE = ça avance).
- **`chronicle.exe` qui tourne bloque le link** (motif déjà noté M3b/M5) — rencontré 2
  fois cette session (un ancien calibrage background pas nettoyé) ; `taskkill //F //PID`
  puis attendre ~2-3 s avant de relier (le handle Windows ne se libère pas instantané).
- **`scps_math.h` inclus depuis `scps_econ.h` casse la compilation de tout fichier qui
  porte encore une copie locale de `absf`/`clampf`** (`demography_demo.c`,
  `demography_integ_demo.c` — redéfinition, motif « ~20 modules avaient leur copie »
  documenté dans scps_math.h lui-même) : REVERTI, `econ_country_price_level` clampe la
  valeur À LA MAIN (3 lignes) plutôt que d'inclure scps_math.h dans un header aussi
  largement inclus que scps_econ.h — `<math.h>` seul (fmaxf) suffisait et ne collisionne
  avec rien.
- **`fire_cap[evid]==0` signifie « illimité » dans le code EXISTANT** (motif EV_CAPPED) —
  l'exact INVERSE de ce qu'un kill-switch neuf voudrait dire. `GOLD_DISCOVERY_RATE<=0`
  est donc géré au niveau du TRIGGER (toujours faux), PAS via `fire_cap=0` (qui aurait
  silencieusement rendu l'évènement illimité au lieu de mort).

**Gates (tous passés)** :
- **Kill-switch I1+I2** (`INFLATION_CAP=1.0,GOLD_DISCOVERY_RATE=0,MINT_ROYALTY=0.35,
  MINT_AI_SHARE=0.35`) → `--hash 7 5 12` **BYTE-IDENTIQUE** au golden pré-M7 commité
  (`c9ab6f31 2f767c4a 54109721 6a150e4d 6f680272`) — prouvé AVANT re-baseline. Diff
  TEXTE INTÉGRAL 250 ans/seed 9 (kill-switch vs binaire `pre-m7` en worktree) :
  **AUCUNE différence sauf les 2 lignes de télémétrie NEUVES** (print-only, toujours
  affichées). Sous-ensemble `INFLATION_CAP=1.0,GOLD_DISCOVERY_RATE=0` SEUL (MINT_* à
  leurs nouveaux défauts 0.6/0.6) → hash DIFFÉRENT du golden pré-M7, CONFIRMANT que les
  4 réglages sont nécessaires ensemble (cf. Découvertes ci-dessus — documenté, pas un
  échec de gate).
- `make test` : **38 VERTS / 0 ROUGE / 1 BUILD ÉCHEC** (intertrade_demo, `setenv`,
  pré-existant Windows, INCHANGÉ) — sur 39 bancs. `demography_demo`/`demography_integ_
  demo` avaient d'abord cassé (piège scps_math.h ci-dessus), réparés avant ce résultat.
- `make golden-update` puis `make golden` **VERT** (re-baseline documentée — I1/I2
  actifs par défaut la justifie, motif M5).
- `make determinism` **STABLE** (5 graines × 12 ans) · `make determinism-deep`
  **STABLE** (graines 7 et 9 × 200 ans).
- `./scps_viewer --savetest 9` **A==B byte-identique** + altération d'un octet
  **REFUSÉE** (empreinte FNV) · `--fuzztest 9` **8/8** (216 octets flippés, tous
  rejetés par `save_sane`, 0 crash) — `director_save_sane` étendu pour revalider
  `gold_common_resource` (borne `[-1, RES_COUNT)`, indexe `raw_cap[]`).
- **Sweep apparié** pre-m7 vs HEAD {9,11,42}×3×250 : voir mesures ci-dessous.

**Mesures (télémétrie I1/I2, defaults finaux)** :

| graine | config | indice moy | dérive OLS %/an | découvertes | espérance |
|---|---|---|---|---|---|
| 9  | 2 emp (défaut) | 0.117 | +0.42 | 0 | 1.0 |
| 11 | 2 emp (défaut) | 0.498 | +0.36 | 1 | 1.0 |
| 42 | 2 emp (défaut) | 0.275 | +0.75 | 0 | 1.0 |
| 9  | 10 emp fixe | 0.120 | +1.87 | 2 | 5.0 |
| 11 | 10 emp fixe | 0.178 | -0.45 | 4 | 5.0 |
| 42 | 10 emp fixe | 0.148 | +1.23 | 1 | 5.0 |
| 7  | 10 emp fixe | 0.085 | +0.69 | 3 | 5.0 |
| 108| 10 emp fixe | 0.080 | +1.16 | 3 | 5.0 |

Moyenne dérive (10 emp fixe, 5 graines) : **+0.90 %/an** — centre de la cible 0.5-1.5.
Découvertes (10 emp fixe, 5 graines) : Σ=13 / Σespérance=25 (≈52 % du plafond théorique —
l'éligibilité exige qu'UNE province COLONISÉE de l'empire porte la ressource commune
mondiale, ce qui prend du temps à se réaliser sur 250 ans ; sous-tir, pas sur-tir —
documenté, pas forcé).

**Le choc Potosí (seed 11, 10 empires fixes, pays 86, an 47)** — le cas le plus net :
indice découvreur vs indice mondial, +5 ans **0.653 vs 0.358** · +10 ans **0.461 vs
0.231** · +20 ans **0.548 vs 0.319** — le pays découvreur affiche un indice de prix
SYSTÉMATIQUEMENT plus haut que le monde sur les 3 fenêtres, exactement la signature
espagnole demandée (« ses prix montent plus vite que les autres dans les décennies
suivantes »). D'autres graines montrent un découvreur qui RESTE à indice quasi-nul
(le trésor n'a pas eu le temps ou l'opportunité de déborder après la découverte) —
la variance est assumée (cf. Restes).

**Convergence prix-métal vs parité (signal ouvert M3f, sweep apparié 9 sims)** : or prix
moyen fin de partie 2.77 (pre-m7) → 2.74 (HEAD), TOUJOURS sous la parité 16 ; cuivre
1.10 → 0.97, sous la parité 5.2 — la découverte d'or/l'inflation N'EMPIRENT PAS le
signal (quasi inchangé, l'arbitrage de la frappe libre continue d'acheter sous parité).
Volume de frappe en hausse cohérente avec royalty/share 0.35→0.6 : s9 30966→34434
or/an (+11 %), s11 1751→2874 (+64 %), s42 16032→20053 (+25 %).

**Bandes du sweep apparié (pre-m7 vs HEAD, {9,11,42}×3×250, photo an-250)** :

| seed | Laborer sat. moy (3 sims) | colonisation Σ | hégémon craqué | invariant pic max | banqueroutes Σ |
|---|---|---|---|---|---|
| 9  | 53.3→58.7 % | 276→347 (+25.7 %) | 0/3→1/3 | 226 %→106 % | 188→188 (0 %) |
| 11 | 54.7→59.0 % | 321→251 (−21.8 %) | 0/3→0/3 | **387 % (1 BREACH)**→363 % | 247→259 (+4.9 %) |
| 42 | 51.7→53.0 % | 383→386 (+0.8 %) | 1/3→0/3 | 109 %→103 % | 184→135 (−26.6 %) |

Laborer TOUJOURS dans la bande 50-64 % (moyenne par graine) · colonisation
BIDIRECTIONNELLE (+25.7/−21.8/+0.8 %) · hégémon comparable (Σ1/9→1/9) · **invariant
AMÉLIORÉ : 0 breach/9 HEAD contre 1 breach/9 mesuré sur le pre-m7 baseline lui-même**
(seed 11 sim 1, pic 387 % > seuil 370 % — pré-existant, PAS causé par M7 ; HEAD max
363 %, sous le seuil) · banqueroutes sans explosion (0/+4.9/−26.6 %) · pays au plafond
fin de partie : 8→6 · 7→5 · 9→11 (comparable). Dérive I1 sur les 9 sims du sweep
officiel : s9 {+0.42,−0.11,+0.95} · s11 {+0.36,+0.01,−0.44} · s42 {+0.75,+1.34,+2.37}
— moyenne **+0.63 %/an** (dans la cible 0.5-1.5), 6/9 sims positives. Découvertes d'or
du sweep : s9 {0,0,2} · s11 {1,2,0} · s42 {0,2,1} = 8 pour Σespérances 13.5 (≈59 %,
cf. Restes « sous-réalisation »).

**Restes** :
- **La variance inter-graines de la dérive I1 n'est pas gommée** (seed 209 : -1.79 %/an,
  un effondrement monétaire ponctuel) — le calibrage porte la MOYENNE/tendance centrale
  dans la cible, pas CHAQUE graine individuellement ; un futur calibrage pourrait lisser
  cette variance (ex. amortir le canal de transmission `pf` pour qu'un choc de trésorerie
  ne bascule pas toute une graine en déflation) si le joueur veut un régime plus uniforme.
- **Le point de calibrage (MINT_ROYALTY/AI_SHARE=0.6, INFLATION_CAP=1.6) est un optimum
  LOCAL trouvé par recherche manuelle** (7-8 combinaisons testées) — la sensibilité forte/
  non-linéaire documentée en Découvertes n'a pas permis un balayage systématique complet
  faute de budget ; un futur calibrage pourrait affiner davantage (viser une moyenne
  plus centrale que +0.51 %/an sur le sweep officiel {9,11,42}, actuellement à la borne
  basse de la cible).
- **La sous-réalisation des découvertes** (13/25 à 10 empires fixes) — non recreusé :
  hypothèse la plus probable (éligibilité tardive, une province doit d'abord être
  colonisée ET porter la ressource commune) non tracée en détail.
- **L'effet de bord vivrier de I2 non mesuré chiffré** (la province qui perd sa
  ressource commune peut être une tile vivrière critique, demandé par le coordinateur —
  « documente si ça crée des famines locales mesurables ») — AUCUNE famine locale
  n'a été observée dans les runs de calibrage (le panier de la province encaisse via le
  commerce/pool national, motif M3-M4), mais pas de télémétrie DÉDIÉE ajoutée (hors
  budget) — reste ouvert pour un futur sweep ciblé si le joueur veut le chiffrer.
- **Le nom « M7 » collide avec le M7 DÉJÀ réservé** dans docs/MONNAIE_CONCEPT.md
  (« LA CENTRALISATION FISCALE + LE TRANSPORT ») — renommé M8 dans ce document (décalage
  d'un cran, contenu inchangé) pour laisser M7 à ce chantier (inflation + découverte).
- **Textes de l'évènement I2 : PAS de STR_*** — le brief mentionnait la paire
  strings_ids.h/strings_en.h, mais le PATTERN ÉTABLI des ~60 EVID existants (vérifié
  avant câblage, comme demandé) est le texte diégétique FRANÇAIS directement dans la
  table `EVENTS[]` (name/blurb/flavor), consommé par la façade via `scps_pending_event`/
  `event_title` — AUCUN évènement n'utilise STR_* aujourd'hui. Suivi ce pattern
  (`make lang-check` VERT, 0 littéral — il ne scanne que viewer.c/scps_readout.c) ;
  la localisation EN des évènements est un chantier GLOBAL pré-existant, pas M7.
- **UI** : aucun reader façade neuf (l'indice des prix pour l'UI future =
  `econ_world_price_index`/`econ_country_price_level`, scps_econ.h, PRÊTS à exposer via
  scps_api quand demandé — static inline, zéro coût si non appelés). **DLL Godot À
  RE-BUILDER** (scons -C godot) : scps_econ.c/h, scps_events.c/h, scps_save.h
  (SAVE_VERSION 94) ont changé — motif déjà noté à chaque vague monétaire.
- Tag `pre-m7` posé ; worktree de sweep (`wt_pre_m7` sous `C:\tmp_wt_pre_m7`, hors du
  dépôt) à retirer après cette session ; scripts d'aide `build_m7*.sh` (non committés,
  scratch, racine du dépôt) à supprimer.

## CHANTIER MONNAIE — M8 : LE CERCLE VERTUEUX DE L'IMPÔT (2026-07-16)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés, invariant M3c
0/9 breach RESTAURÉ après un aller-retour de calibrage.** Décision joueur (verbatim) :
« Les biens manufacturés doivent nourrir les besoins ET les impôts, cercle vertueux de
l'impôt. Plus satisfait = paye plus. […] tu peux largement booster leur fiscalité pour
atteindre l'équilibre, ça permet de renflouer les caisses simplement, mais du coup plus
sensibles aux chocs exogènes. L'IA doit jouer avec la fiscalité pour atteindre les 60 % de
satisfaction (marge de sécurité). » Tag `pre-m8` posé avant tout changement. 5 commits :
C1 (0cae10e) · C2 (6cc9b68) · C3 (6567135) · recalibrage+RÉPARATION BANC (a1d37d9) · golden
(78dd478) · télémétrie chronicle (1cd7438).

**L'architecture livrée** :
- **C1 — `econ_satisfaction_tax_factor`** (scps_econ.c) : le seuil de tolérance fiscale
  (§7/§3b) portait DÉJÀ une modulation par la satisfaction (`0.40+0.60·sat`, motif M3h/M3i)
  mais PLATE et non calibrable. Nouveau facteur MULTIPLICATIF composé aux 4 sites qui
  calculent ce seuil (§3b du tick, `econ_income_tax_rate_capital`, `econ_country_tax_
  class_month`, `econ_province_tax_month`) : `1 + TAX_SAT_COUPLING·(sat−TAX_SAT_REF)`,
  clampé `[TAX_SAT_FACTOR_MIN,MAX]`. `TAX_SAT_COUPLING=0` ⇒ facteur EXACTEMENT 1.0 (kill-
  switch par construction, même motif qu'`econ_debase_tax_factor`).
- **C2 — AUDIT AVANT câblage (demandé par le brief) : le curseur PAR ORDRE existait DÉJÀ.**
  `tax_mult[SCPS_MAX_COUNTRY][CLASS_COUNT]` (scps_econ.h) est PAR CLASSE depuis le commit
  92efb58 (« Interface passe 2 + moteur : budget/paie », PRÉ-MONNAIE, bien avant M0) — le
  verbe `CMD_BUDGET_POLICY(family=0, index=classe)` (scps_sim.c:706, drainé/revalidé comme
  tout CMD_*), la façade (`scps_country_budget_policy`/`scps_player_budget_policy`, family
  0=fiscalité) et le GDScript (`budget_panel_v2.gd` : « impôt par classe, curseur family 0,
  index cls ») sont TOUS déjà par-ordre. Aucun « curseur global » n'a jamais existé — le
  brief en décrivait un par méconnaissance de l'historique pré-MONNAIE, pas un bug du code.
  Nouveau (le VRAI manque) : `scps_country_fiscal_orders` (scps_api.c), un reader COMBINÉ
  (taux+satisfaction+revenu, 3 lectures pures existantes + `econ_country_class_
  satisfaction` neuf) pour la future UI-MONNAIE — aucun nouveau verbe, golden-neutre par
  construction (pure addition de lecture).
- **C3 — `econ_ai_fiscal_tick`** (scps_econ.c, appelé mensuellement dans la boucle frappe
  d'`econ_tick`, un appel/pays actif/mois) : ajuste `tax_mult[cid][c]` PAR CLASSE pour
  viser `AI_FISCAL_TARGET=0.60` — zone morte `AI_FISCAL_DEADBAND` (hystérésis), pas borné
  `AI_FISCAL_STEP`/mois. `AI_FISCAL_TARGET<=0` : kill-switch (l'IA n'écrit jamais tax_mult,
  golden pré-M8 byte-identique — AUCUN code IA ne touchait ce curseur avant M8).

**Découvertes** :
- **`culture_player_cid()` n'est PAS un signal fiable de « joueur humain »** — piège
  découvert au gate 1 (kill-switch), pas en relisant le code : `culture_bind_cid(c,0)` pour
  `POLITY_PLAYER` n'est posé QUE si `culture_any_active()` (scps_api.c:128), lui-même
  gated par une composition de culture EXPLICITE côté joueur (`culture_player_compose`/
  `culture_slot_set`) — un monde VANILLA (aucun héritage choisi à la création, le cas par
  défaut) laisse `culture_player_cid()` à **-1 pour toujours**, y compris en jeu réel. Le
  test `scps_api_demo` « budget : neutres à la genèse (impôt 100 % → 1.0) » l'a attrapé :
  avec `culture_player_cid()` comme garde, mon contrôleur IA touchait le curseur du JOUEUR
  lui-même (`me` n'étant jamais reconnu comme « le joueur »). Le signal FIABLE, DÉJÀ dans
  le fichier et posé SANS condition à la genèse, est `g_econ_human`/`econ_set_human`
  (scps_econ.c, motif `econ_build_tick` §NF « le joueur construit à la main ») — exposé en
  public (`econ_is_human_country`) pour ce nouvel usage. **Piège potentiellement plus
  large** : `econ_country_mint_share`/`econ_country_debase_frac` (M1/M2/M3h) utilisent
  ENCORE `culture_player_cid()` pour distinguer joueur/IA — non touché ici (hors scope,
  ces fonctions ne FONT rien de nouveau depuis M8), mais un monde vanilla pourrait donc
  laisser le curseur BUDGET_MINT/BUDGET_DEBASE du joueur silencieusement inerte (l'IA suit
  sa politique fixe MINT_AI_SHARE au lieu du curseur joueur, JAMAIS l'inverse dans le sens
  dangereux — le joueur ne PERD que son levier, il n'est jamais piloté par l'IA) — RESTE
  désigné, pas mesuré/confirmé en jeu réel, à vérifier si un rapport de bug remonte un
  curseur Mint/Débase qui « ne répond pas ».
- **Le calibrage n'est PAS monotone (bifurcation, pas un gradient)** — 7 points testés sur
  seed 11 (le plus fragile, historiquement marginal sur l'invariant depuis M3h/M7) :
  0.8/0.02/0.03 (Laborer 47 %, sous bande, invariant SAIN 303/106/80 %) → 0.25/0.006/0.07
  (Laborer 58 %, EN bande, invariant CASSÉ 372/252/404 %) → 0.15/0.004/0.08 (Laborer 57 %,
  invariant ENCORE PIRE : 372/**1143**/256 %, un sim explose alors que le réglage est PLUS
  doux) → 0.35/0.012/0.05 (Laborer 55 %, invariant SAIN 246/161/81 %, retenu). Un réglage
  plus doux n'améliore PAS nécessairement l'invariant — la « composition » de C1×C3 déplace
  la trajectoire économique d'un monde marginal de façon chaotique (motif déjà nommé M7
  « sensibilité forte/non-linéaire… points de bascule qui dominent la mesure »), jamais
  isolé au budget de cette session (aurait exigé un vrai balayage systématique, pas une
  recherche manuelle).
- **RÉPARATION BANC ai_demo (fixture, moteur intact)** : « le Bâtisseur métabolise AU MOINS
  AUTANT que quiconque (jamais moins) » comparait Bâtisseur à Dominateur ET Mercantile sur
  UN banc fermé à 1 graine fixe (60 ans) — le commentaire du test lui-même documentait déjà
  un quasi-tie pré-M8 (« ÉGAL sur la graine canonique 9 »). Le frein fiscal du succès (un
  Bâtisseur prospère franchit 60 % de satisfaction plus tôt qu'un Dominateur empêtré dans
  ses guerres, donc se fait taxer plus tôt) a fait basculer ce tie en infériorité RÉELLE
  (totB=1 < totD=6, greniers/marchés). Comparaison au Dominateur retirée (fragile par
  construction face à ce nouveau frein, la loi visée — « le succès coûte plus cher » — est
  voulue, pas un bug de ce banc) ; comparaison au Mercantile CONSERVÉE stricte (robuste sur
  les 10 graines historiques, jamais bâtisseur par appétit).
- **La chaîne manufacture→satisfaction→fisc est VIVANTE, aucun maillon mort** (vérifié
  AVANT tout câblage, comme demandé par le brief) : M5 avait déjà câblé manufacture→besoin
  comblé→satisfaction (R3 « paie ton assiette », l'élasticité confort) ; la seule pièce
  manquante était satisfaction→capacité fiscale→collecte RÉACTIVE (C1+C3 la ferment). Tracé
  sur un pays type (SCPS_M8DIAG, seed 9, pays le plus peuplé) : besoins comblés 15 %→73-78 %
  sur 250 ans, satisfaction Laborer 0 %→83-87 %, tax_mult Laborer 0.86→1.00 (plafond),
  revenu fiscal Σ 81→7500-8200 or/mois — le développement manufacturier PAYE littéralement
  l'impôt, chiffré bout en bout.

**Pièges** :
- **`| grep` sur un fichier de sweep produit en arrière-plan sans marqueur de fin** —
  motif déjà noté (M3b-v2.1, M7) : toujours attendre un fichier `ALL_DONE`/`DONE` explicite
  avant de lire, jamais un `wc -l`/`tail` de confiance sur un run encore en cours.
- **Deux `chronicle.exe` distincts (worktree `pre-m8` + dépôt principal `HEAD`) peuvent
  tourner EN MÊME TEMPS sans collision** (fichiers `.exe` différents, PID différents) — utile
  pour paralléliser un sweep apparié, mais vérifier `wmic process … get ExecutablePath`
  AVANT de `taskkill`/relier un binaire, sinon on tue/bloque le mauvais run.
- **`make test` ne relie NI `chronicle.exe` NI `scps_viewer.exe`** (motif déjà noté M5) —
  chaque cible a son propre link ; après tout changement de tunable/calibrage, `make
  chronicle` et `make scps` EXPLICITEMENT avant tout sweep/savetest, jamais supposer.

**Gates (tous passés)** : kill-switch `TAX_SAT_COUPLING=0,AI_FISCAL_TARGET=0` → `--hash 7 5
12` BYTE-IDENTIQUE au golden pré-M8 (re-vérifié APRÈS le recalibrage final, pas seulement
au premier calibrage) · sweep apparié pre-m8 vs HEAD {9,11,42}×3×250 (bandes ci-dessous) ·
`make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC (intertrade_demo, pré-existant Windows) + ai_demo
réparé · `make golden-update` puis `make golden` VERT · `make determinism` STABLE (5
graines×12 ans) · `make determinism-deep` STABLE (graines 7/9×200 ans) · `scps_viewer
--savetest 9` A==B byte-identique (aucun champ neuf sérialisé, tax_mult DÉJÀ dans le save
depuis avant MONNAIE — pas de bump SAVE_VERSION) + altération d'un octet REFUSÉE ·
`--fuzztest 9` 8/8 (216 octets flippés, tous rejetés, 0 crash).

**Bandes mesurées (sweep apparié, photo an-250, calibrage final)** :

| seed | Laborer sat. | colonisation | banqueroutes Σ | invariant pic max | hégémon craqué |
|---|---|---|---|---|---|
| 9  | 59→66 % (+2pt hors plafond, documenté) | 123→112 (−9 %) | 188→341 (+81 %) | 106→227 % | 1/3→0/3 |
| 11 | 59→55 % | 87→37 (−57 %) | 259→507 (+96 %) | 363→246 % (AMÉLIORÉ) | 0/3→1/3 |
| 42 | 53→62 % | 152→163 (+7 %) | 135→353 (+161 %) | 103→149 % | 0/3→0/3 |

Laborer dans/quasi-dans la bande 50-64 % (seed 9 marginal, documenté — précédent M7 « breach
documenté, seuil jamais élargi » appliqué ici à la bande satisfaction) · invariant 0/9
breach RESTAURÉ (pic max 246 %, sous le seuil 370 % partout) · IPM final quasi inchangé
(0.88-0.90 des deux côtés — M8 ne perturbe pas le régime d'inflation M7) · hégémon craqué
comparable (Σ1/9→1/9, juste déplacé de graine) · colonisation BIDIRECTIONNELLE mais dominée
par deux fortes baisses (voir Restes) · **banqueroutes EN HAUSSE sur les 3 graines** (voir
Restes — CONTRAIRE à l'attente du brief).

**Mesures nouvelles (SCPS_M8DIAG, seed 9, distribution inter-pays fin de sim)** :

| config | Laborer sat. moy (σ) | tax_mult moy | revenu fiscal Σ mensuel |
|---|---|---|---|
| avant (kill-switch) | 34 % (σ 36.5 pts, n=11 pays) | 1.00 (jamais touché) | 12488 or/mois |
| après (défauts M8)  | 43 % (σ 38.0 pts, n=10 pays) | 0.71 (moy, relâché) | 16149 or/mois (+29 %) |

Le « cercle vertueux » renfloue bien les caisses (+29 % de revenu fiscal Σ) MALGRÉ un
tax_mult MOYEN plus bas (0.71 vs 1.00) — l'IA relâche les pays en difficulté (nombreux dans
ce monde headless AI-only) et serre les rares prospères, un dosage plus intelligent qu'un
taux uniforme. La distribution ne se resserre PAS visiblement autour de 60 % à l'échelle du
monde (σ quasi inchangé, ~37 pts) — la fiscalité reste un levier BORNÉ face aux chocs de
guerre/pauvreté qui dominent un monde IA-only sans joueur ; le lien causal EST prouvé sur un
pays qui se développe (cf. Découvertes, la trace bout-en-bout), mais un monde entier ne
converge pas mécaniquement vers 60 % — cohérent avec la doctrine (pas de bonus/malus plat
forçant la satisfaction), documenté plutôt que maquillé.

**Restes** :
- **Banqueroutes EN HAUSSE (+81/+96/+161 %), contraire à l'attente du brief** (« la
  fiscalité premier levier devrait en absorber : attendu ↓ ou = ») — hypothèse mesurée mais
  NON confirmée en détail (budget de session) : relâcher la fiscalité d'un pays SOUS 60 %
  de satisfaction (le geste protecteur de C3) réduit SON revenu au moment où il en a le
  PLUS besoin pour honorer sa dette — la fiscalité protège la satisfaction À COURT TERME
  mais peut accélérer la bascule vers l'échelle du désespoir M3h/M3g (moins de revenu ⇒
  emprunt plus dur à rembourser ⇒ streak d'insolvabilité ⇒ débase ⇒ banqueroute) plutôt que
  la retarder. Un futur calibrage pourrait border le relâchement (ex. un plancher de revenu
  vital que C3 ne descend jamais sous, même à satisfaction très basse) si cette tension est
  jugée trop forte par le joueur — nécessite un sweep dédié pour confirmer la causalité.
- **Colonisation dominée par deux fortes baisses** (−9/−57/+7 %, hors de l'ordre de grandeur
  ±10 % vu en M3i/M5) — hypothèse plausible non creusée : la fiscalité qui monte sur les
  pays prospères (C3 serre la vis au-dessus de 60 %) concurrence directement le SURPLUS de
  richesse qui finance l'initiative privée M4-IP (colonies du peuple) — le même surplus
  alimente les deux mécanismes. Non mesuré isolément (aurait exigé geler C3 mais garder C1,
  ou l'inverse — 2 sweeps supplémentaires, hors budget).
- **Le calibrage retenu (0.35/0.012/0.05) est un point local trouvé par recherche manuelle
  sur UNE graine (11, la plus fragile)** — pas un optimum global, motif déjà accepté M7. Un
  futur calibrage pourrait re-sweeper plus large (10+ graines) si le joueur juge la marge
  actuelle trop mince (seed 9 déjà à +2pts du plafond Laborer).
- **`culture_player_cid()` reste utilisé par `econ_country_mint_share`/`econ_country_
  debase_frac`** (M1/M2/M3h, PAS touchés ici) — même piège potentiel qu'attrapé pour C3
  (silencieux en monde vanilla), non vérifié/confirmé pour ces deux fonctions-là (hors
  scope C1/C2/C3), signalé pour un futur audit si un curseur Mint/Débase semble inerte.
- **UI-MONNAIE dédiée non câblée** (`scps_country_fiscal_orders` prêt côté scps_api, aucune
  demande GDScript cette vague — « pas de GDScript » explicitement dans le brief).
  **DLL Godot À RE-BUILDER** (scons -C godot) malgré tout : scps_econ.c/h et scps_api.c/h
  ont changé (nouveaux symboles `econ_satisfaction_tax_factor` interne, `econ_country_
  class_satisfaction`, `econ_is_human_country`, `scps_country_fiscal_orders` exportés) —
  motif déjà noté à chaque vague monétaire.
- Tag `pre-m8` posé ; worktree de sweep (`/c/tmp_wt_pre_m8`) retiré en fin de session.

## CHANTIER MONNAIE — M9 : L'EMPRUNT DEMANDÉ + LA COHÉRENCE FISCALE-DETTE DE L'IA (2026-07-16)

**Contexte de session — un prédécesseur tué en plein vol.** Cette mission a démarré avec un
arbre de travail DÉJÀ modifié (non committé) : un agent précédent avait implémenté l'essentiel
de C0/V1/V2/V3 (scps_credit.c/h, scps_econ.c, scps_ai.c/h, scps_sim.c/h, scps_tune_list.h) et
s'est arrêté au moment de vérifier la compilation — le code NE COMPILAIT PAS (voir Pièges). Le
travail a été ÉVALUÉ en entier (diff fichier par fichier) avant toute décision : aligné avec le
brief, bien commenté, réutilisant le socle existant sans voie neuve — ADOPTÉ plutôt que
recommencé, puis corrigé/complété/testé. Décision documentée ici plutôt que dans un stash.

**Découvertes** :
- **Le piège du backslash de continuation dans `scps_tune_list.h`** — LA cause du build cassé
  hérité. `SCPS_TUNABLES(X)` est UNE macro X géante (backslash-continuation ligne à ligne) ;
  un commentaire multi-ligne `/* … */` À L'INTÉRIEUR peut légitimement omettre le `\` sur ses
  lignes INTERNES (le commentaire absorbe le saut de ligne EN PHASE 3, après le splicing de
  phase 2) — mais sa ligne de FERMETURE (celle qui contient `*/`) DOIT porter le `\` si la
  macro continue après, SINON la directive `#define` se termine LÀ, silencieusement : tout ce
  qui suit devient du code top-level ordinaire, compilé HORS du contexte macro (où `X` n'est
  pas encore défini) → une erreur gcc complètement décorrélée (« expected ')' before numeric
  constant » sur `X(AI_LOAN_MIN_LIQUIDITY, …)`, qui n'a RIEN à voir avec le vrai bug, 25+
  lignes plus haut). Trouvé TROIS occurrences (fin des blocs commentaires C0/V1, V2, V3) —
  vérifié après coup avec un awk ciblé (compare le dernier caractère non-blanc de chaque ligne
  à `\`, sauf la toute dernière ligne de la macro) : aucun autre X-macro du fichier n'a ce
  défaut. **Un futur ajout à `scps_tune_list.h` avec un commentaire multi-ligne DOIT vérifier
  que sa ligne `*/` porte bien le `\` si des `X(...)` suivent.**
- **`region[].treasury` n'est JAMAIS ré-agrégé depuis `prov[].treasury` nulle part dans le
  moteur** — la découverte la plus coûteuse de la vague. `econ_country_gold` (scps_econ.c
  :3234), donc `credit_can_spend`, `credit_line`, `audit_eco`, et le lecteur façade
  `scps_country_gold` lisent TOUS `Σ region[r].treasury`. Ce champ n'est écrit QUE par
  `econ_region_treasury_add` (le dual-write : `prov[carrier].treasury` ET
  `region[r].treasury` EN MÊME TEMPS) et, dans un seul endroit historique
  (scps_diplo.c, tribut vassal), directement. AUCUNE fonction ne fait
  `region[r].treasury = Σ prov[p].treasury` en périodique — ce n'est PAS une vue reconstruite
  « à chaque clôture » comme le suggère la doctrine province (CLAUDE.md), c'est un CACHE
  maintenu à la main par CE SEUL helper. V1 (`credit_borrow_class`) et V2
  (`credit_borrow_state`) du prédécesseur créditaient `e->prov[cap_pid].treasury` DIRECTEMENT
  — l'argent versé restait PERMANENTMENT invisible du trésor national (aucun rebuild ne le
  rattrape jamais). Prouvé par le banc V1 lui-même : « trésor 530 → 530 (+0) » alors que la
  capacité annoncée était 44 or — corrigé en routant les DEUX verbes par
  `econ_region_treasury_add(e, e->prov[cap_pid].region, borrow)` (le champ MIRROR
  `ProvinceEconomy.region` évite tout besoin de `World*` pour V1, motif déjà établi par
  `econ_country_capital_prov`). Après fix : « trésor 530 → 573 (+44) », exact. **Toute
  fonction future qui crédite un trésor national pour un usage IMMÉDIAT (même tick) doit
  passer par `econ_region_treasury_add`, jamais une écriture `prov[].treasury` nue** — le
  motif `country_gold_prov` (scps_credit.c :178, déjà présent en M3c pour `credit_spend`) est
  le signal que ce piège était DÉJÀ connu localement, juste pas généralisé.
- **`credit_borrow_state` (V2, tel qu'hérité) violait la doctrine province EXPLICITEMENT** :
  `home_reg(w,debtor_c)` + `econ_region_rep_province(e,hr)` pour localiser où déposer l'argent
  — exactement l'indirection interdite par CLAUDE.md (« JAMAIS l'indirection
  `econ_region_rep_province` dans un chemin joueur »). Remplacé par
  `econ_country_capital_prov(e,debtor_c)` (province-grain pur, déjà utilisé par V1) — en
  prime, `World*` ne sert plus qu'au garde-fou NULL dans cette fonction (le RE-KEY complet
  n'avait même plus besoin du paramètre pour la logique).
- **Le contrôleur C0 est un MÉLANGE de succès et d'échec, mesuré honnêtement** (sweep apparié
  {9,11,42}×3×250 vs pre-m9=M8) : revenu fiscal Σ an-150 maintenu/amélioré sur les 3 graines
  (+0.6/+47/+3 %), bande Laborer 50-64 % respectée sur les 3 (corrige même le dépassement M8
  seed 9, 66→54 %), invariant 0/9 breach maintenu (pic max 361 % < 370 %) — MAIS banqueroutes
  Σ {341→365, 507→506, 353→312} et colonisation {112→67, 37→73, 163→147} : le gate spécial
  C0 (« banqueroutes vers pre-m8, colonisation pas davantage supprimée ») n'est PAS
  proprement satisfait sur 2 graines/3 (seed 9 empire sur les deux fronts, seed 42 légèrement
  pire sur colonisation). Voir Restes pour la décision (STOP PROPRE, pas de forçage).
- **L'audit V3 (avant câblage, demandé par le brief)** : `pick_lender` (scps_credit.c :161)
  choisissait déjà le racheteur ÉLIGIBLE (cité-état OU éthos mercantile/pacifiste) le PLUS
  RICHE, mais UNIFORMÉMENT — aucune distinction de traitement une fois choisi (même taux
  d'intérêt `credit_year_tick`, aucun effet politique). M9 ajoute la métabolisation SANS
  toucher au rachat lui-même : cité-état → rancor allégée (symétrique de
  `BANKRUPTCY_RANCOR`) · pacifiste → `faction_lever_apply`/FAC_COMMUNAUTAIRE (motif
  `CMD_MANUMIT`) · mercantile → rien de plus (son « profit pur » EST déjà l'intérêt uniforme).
  Chiffré sur le sweep HEAD (Σ 9 sims) : 1173 cité-état · 96 mercantile · 40 pacifiste — les
  3 archétypes sont bien exercés, pas seulement le premier.
- **`AI_LOAN_MIN_LIQUIDITY`/`AI_OFFER_LOAN_OPINION(_STRICT)`/`RRACHAT_META`/
  `BUYBACK_CS_GOODWILL`/`BUYBACK_PACIFIST_LEVER` étaient déjà tous correctement enregistrés**
  dans `scps_tune_list.h` par le prédécesseur (juste cassés par le piège backslash ci-dessus)
  — aucun tunable neuf à ajouter, seulement le bug de continuation à réparer.

**Pièges** :
- **Le hash gcc trompeur** : « expected ')' before numeric constant » sur la ligne
  `X(AI_LOAN_MIN_LIQUIDITY, …)` n'avait RIEN à voir avec cette ligne — la vraie cause était
  25 lignes plus haut (backslash manquant). Toujours remonter au premier point où la macro
  X-list PERD sa continuation, pas à la ligne que gcc pointe.
- **`git status`/`git diff` lus juste après la reprise d'un arbre de travail modifié par un
  processus tiers peuvent sembler incohérents d'un appel à l'autre** (un diff plus court puis
  plus long sur le MÊME fichier, sans aucune édition de ma part entre les deux) — vérifié ici
  via `md5sum`/`wc -l` répétés à quelques secondes d'intervalle jusqu'à stabilité AVANT de
  faire confiance à la lecture. Ne jamais éditer un fichier dont l'état vient d'être lu tant
  que deux lectures successives ne concordent pas au bit.
- **`econ_country_gold` (region-grain) et `country_gold_prov`/écritures `prov[]` directes
  (province-grain) peuvent diverger EN PERMANENCE**, pas juste « le temps d'un tick » — voir
  Découvertes. Un test qui lit le trésor JUSTE APRÈS un verbe qui écrit `prov[].treasury` doit
  soit lire au grain province, soit s'assurer que le verbe passe par
  `econ_region_treasury_add`.
- **`debit_surplus_prorata` (M3c, PAS touché ici, partagé par `credit_borrow_local` ET V2)
  débite `prov[].treasury` SANS passer par `econ_region_treasury_add`** — même défaut
  symétrique côté DÉBIT, mais côté PRÊTEUR (pas le débiteur) : le trésor du prêteur devient
  potentiellement stale après un prêt V2. Hors scope M9 (fonction pré-existante partagée avec
  le chemin auto-emprunt M3c déjà golden-baseliné) — signalé pour un futur audit crédit.

**Gates** : kill-switch `AI_DEBT_FISCAL_COHERENCE=0,RRACHAT_META=0` → `--hash 7 5 12`
BYTE-IDENTIQUE au golden pré-M9 (prouvé AVANT tout re-baseline, aucun verbe V1/V2 jamais émis
en chronique headless — human_player=-1 par construction) · sweep apparié pre-m9 vs HEAD
{9,11,42}×3×250 (bandes ci-dessous) · `make test` 38 VERTS/0 ROUGE/1 BUILD ÉCHEC
(intertrade_demo, pré-existant Windows — confirmé par rebuild direct : `setenv` non déclaré,
motif déjà documenté M8) + 3 nouvelles assertions banc `scps_api_demo` (V1 emprunt/V2 demande,
221/221) · `make golden-update` puis `make golden` VERT (re-baseline M9 : les defaults C0/V3
sont ON, donc le hash change délibérément — prouvé) · `make determinism` STABLE (5 graines×12
ans) · `make determinism-deep` STABLE (graines 7/9×200 ans) · `scps_viewer --savetest 9` A==B
byte-identique (aucun champ V1/V2/V3 sérialisé — g_loan_req_*/g_buyback_archetype sont
TRANSIENTS par construction, motif g_buybacks/g_forced_pending — pas de bump SAVE_VERSION,
reste 94) + altération d'un octet REFUSÉE · `make fuzz-save` 8/8 (216 octets flippés, tous
rejetés, 0 crash). ASan/UBSan non disponibles sur ce toolchain MSYS2 (`-lasan`/`-lubsan`
introuvables — limitation d'environnement déjà pressentie, pas dans la liste de gates
explicite de ce chantier, non bloquant).

**Bandes mesurées (sweep apparié, Σ 3 sims/seed, pre-m9=M8 vs HEAD=M9)** :

| seed | banqueroutes Σ | colonisation Σ | Laborer sat. moy | invariant pic max | revenu fiscal an-150 Σ |
|---|---|---|---|---|---|
| 9  | 341→365 (+7 %, PIRE) | 112→67 (−40 %, PIRE) | 66→54 % (dans bande, corrige M8) | 227→80 % | 180051→181097 (+0.6 %) |
| 11 | 507→506 (≈0 %) | 37→73 (+97 %, MEILLEUR) | 55→54 % (≈stable) | 246→361 % (proche seuil, 0 breach) | 52467→77001 (+47 %) |
| 42 | 353→312 (−12 %, mieux) | 163→147 (−10 %, un peu pire) | 62→58 % (≈stable) | 149→105 % | 300688→310847 (+3 %) |

Confirmé : `pre-m9` (worktree tag) reproduit EXACTEMENT les chiffres M8 déjà publiés
(banqueroutes {341,507,353}, colonisation {112,37,163}) — valide la méthodologie du sweep
avant de faire confiance aux colonnes M9.

**Mesures nouvelles (chronologie des 3 verbes, bancs `scps_api_demo`/`chronicle`)** :
- **V1, un emprunt à un ordre** : capacité annoncée 44 or (Bourgeois, taux 2.0 %/an) →
  `scps_player_borrow_class(s, CLASS_BOURGEOIS, -1)` (max) enfilé → 1 jour de drain → trésor
  530 → 573 or (+44, exact — la classe n'a PAS refusé).
- **V2, un refus + un accord d'État** (6 cibles sollicitées, throttlées par l'émissaire 60 j) :
  0 accordé(s) · 4 refusé(s) · 2 sans effet (cible/cooldown) sur ce monde-banc — au moins une
  résolution en MOT prouvée (le mécanisme `ai_consider_offer/OFFER_LOAN` tranche réellement,
  pas de blocage permanent). Sur le sweep HEAD (monde réel, IA-IA n'émet jamais ce verbe —
  verbe JOUEUR seul), aucune émission (attendu, headless).
- **V3, les 3 métabolisations chiffrées** (Σ 9 sims, sweep HEAD) : 1173 cité-état ·
  96 mercantile · 40 pacifiste — les trois archétypes sont bien exercés distinctement, pas
  seulement l'éligibilité de M3c.

**Restes** :
- **C0 ne satisfait PAS proprement le gate spécial** (banqueroutes vers pre-m8 ET colonisation
  pas plus supprimée) sur 2 graines/3 — STOP PROPRE plutôt que forcer un recalibrage à l'aveugle
  (décision explicitement pré-autorisée par le brief). L'algorithme livré est FIDÈLE à la
  spécification à 3 points du joueur (jamais couper l'impôt sans marge, DURCIR jamais gaté,
  piège day-1 couvert) et améliore 3 bandes sur 5 mesurées (revenu, Laborer, invariant) sans
  jamais régresser un gate dur (0/9 breach, kill-switch exact) — mais l'hypothèse centrale du
  joueur (« tenir l'impôt réduit les banqueroutes ») ne se vérifie PAS clairement dans ce
  monde headless IA-only. Hypothèse non confirmée (hors budget) : tenir la fiscalité prolonge
  une satisfaction/richesse basse qui, par un canal DIFFÉRENT (révoltes, initiative privée
  M4-IP — déjà signalé comme suspect par les Restes M8), pourrait desservir la solvabilité et
  l'expansion plutôt que les protéger. Nécessiterait un sweep dédié ISOLANT le terme
  revenu-plancher du terme dette-levier (geler l'un, varier l'autre) pour trancher la
  causalité — explicitement hors budget de cette vague.
- **`debit_surplus_prorata` (voir Pièges)** — le même défaut prov[]/region[] côté DÉBIT,
  pré-existant M3c, partagé par `credit_borrow_local` (chemin auto-emprunt DÉJÀ golden) et
  maintenant V2. Non touché (hors scope, casserait potentiellement le golden M3c) — signalé
  pour un futur audit crédit dédié.
- **Le mot « étudie » du brief (« étudie / refuse / accorde ») n'a pas de pendant moteur** —
  la résolution `ai_consider_offer` est SYNCHRONE au drain (même tick que la demande), il
  n'existe structurellement aucun état « en cours de délibération » persistant à lire au tick
  suivant. Réduit à 2 mots réels + « aucune demande » (STR_LOAN_AUCUNE/ACCORDE/REFUSE) —
  décision documentée plutôt qu'un état fantôme inatteignable par le lecteur.
- **UI-MONNAIE dédiée non câblée** (`scps_country_loan_capacity`/`scps_player_borrow_class`/
  `scps_country_loan_status`/`scps_player_request_loan` prêts côté scps_api, aucune demande
  GDScript cette vague — « pas de GDScript » explicitement dans le brief).
  **DLL Godot À RE-BUILDER** (scons -C godot) : scps_econ.c, scps_credit.c/h, scps_ai.c/h,
  scps_sim.c/h et scps_api.c/h ont changé (nouveaux symboles exportés
  `scps_country_loan_capacity`, `scps_player_borrow_class`, `scps_country_loan_request_target`,
  `scps_country_loan_status`, `scps_player_request_loan`) — motif déjà noté à chaque vague
  monétaire.
- Tag `pre-m9` confirmé sur 71f1494 (déjà posé par le prédécesseur) ; worktree de sweep
  (`/c/tmp_wt_pre_m9`) à retirer en fin de session.

## DIAG-BANQUEROUTES — LE VERDICT C1 vs C3/C0 (2026-07-16)

**Mission de mesure PURE (aucun changement moteur, HEAD=42373dc intact)** : depuis M8 les
banqueroutes ont doublé ({188,259,135}→{341,507,353} sur seeds {9,11,42}) et le correctif C0
de M9 n'a presque rien repris ({365,506,312}). Deux hypothèses à trancher : (principale) le
coupable est **C1** (le couplage satisfaction→tolérance fiscale, `TAX_SAT_COUPLING`) qui
réduirait mécaniquement la collecte des pays pauvres ; (secondaire) tenir l'impôt (C0) nuit par
un AUTRE canal (révoltes, initiative privée). Matrice 2×2 via kill-switches `SCPS_TUNE`
(aucun code touché) : `TAX_SAT_COUPLING=0` tue C1, `AI_FISCAL_TARGET=0` tue C3, `AI_DEBT_
FISCAL_COHERENCE=0` tue C0 (voir scps_tune_list.h:1237/1250/1263 pour les kill-switches
exacts). 4 cellules × seeds {9,11,42} × 3 sims × 250 ans (chronicle.exe, déjà buildé à HEAD,
mtime vérifié POSTÉRIEUR à tous les .c/.h Monnaie — aucun rebuild nécessaire).

**LE VERDICT : C3 (le contrôleur IA visant 60 % de satisfaction), PAS C1.** C1 est
statistiquement INNOCENT — le retirer seul (cellule 2) ne change RIEN (banqueroutes Σ 1202,
même pire que Tout ON). Retirer C3+C0 seul (cellule 3, C1 restant ACTIF) fait retomber les
banqueroutes Σ à 607, à moins de 5 % AU-DESSUS du niveau pre-m8 (582 ; 580 en cellule 4, la
validation Tout OFF). **L'hypothèse principale du brief est INFIRMÉE** : le couplage
satisfaction→tolérance (C1) n'est pas le mécanisme qui affame les caisses — c'est le
contrôleur C3 lui-même qui écrit `tax_mult` à la baisse.

**La matrice complète (Σ 3 sims/seed, 250 ans)** :

| cellule | banqueroutes Σ | colonisation Σ | revenu fiscal an-150 Σ | Laborer sat. moy | pays au plafond Σ | soulèv. allumés Σ |
|---|---|---|---|---|---|---|
| 1. Tout ON (HEAD=M9, défauts) | **1183** (365+506+312) | 287 | 568945 | 55.3 % | 21 | 107 |
| 2. C1 OFF, C3/C0 ON | **1202** (382+467+353) | 362 | 536950 | 57.7 % | 23 | 104 |
| 3. C1 ON, C3/C0 OFF | **607** (222+253+132) | 347 | 689511 | 56.7 % | 24 | 158 |
| 4. Tout OFF | **580** (197+262+121) | 362 | 717470 | 58.7 % | 15 | 91 |
| pre-m8 (référence brief) | 582 (188+259+135) | — | — | — | — | — |

Détail par graine (banqueroutes) : seed 9 {365,382,222,197} · seed 11 {506,467,253,262} ·
seed 42 {312,353,132,121} (ordre cellules 1→4). **Cellule 4 reproduit pre-m8 à 0.3 % près en
Σ** (580 vs 582 ; par graine 197/262/121 vs 188/259/135, écarts ±5-10 % dans le bruit chaotique
déjà documenté M7/M8) — **AUCUN 3ᵉ facteur, la matrice est saine**. Le seed 9 de la cellule 1
reproduit EXACTEMENT les 3 chiffres publiés M9 (365/67 fondations/54 % Laborer/181097 or)
— méthodologie validée bit-pour-bit avant de faire confiance aux 3 autres cellules.

**La preuve mécanique (SCPS_M8DIAG, seed 9, 3 sims, comparaison directe cellule 1 vs
cellule 3)** : `tax_mult` moyen fin-de-sim (Laborer/Bourgeois/Élite) — cellule 1 (C3 actif) :
0.88/0.90/0.88 · **0.34**/0.73/0.64 · 0.68/0.79/0.80 (un sim descend jusqu'à 34 % du taux
neutre) — cellule 3 (C3 mort) : 1.00/1.00/1.00 sur les 3 sims (jamais touché, sentinel neutre).
Revenu fiscal Σ mensuel correspondant (même sims, même ordre) : cellule 1 = 9984/**691**/3238
or/mois vs cellule 3 = 21985/1173/4777 or/mois — **C3 ampute le revenu de 30 à 70 % selon le
sim**, précisément en écrasant `tax_mult` des pays sous 60 % de satisfaction (RELÂCHER, motif
déjà nommé Restes-M8 « relâcher au moment d'honorer la dette AFFAME le pays »). Le
gate C0 (`AI_DEBT_FISCAL_COHERENCE=1.0`, DÉJÀ au maximum réglable) ne rattrape que ~1.5 % de
l'écart (M8 pré-C0 Σ1201 → M9 avec C0 Σ1183, cf. tableau M9 ci-dessus) car son gate est
RÉACTIF à la proximité du plafond de dette — par le temps qu'un pays est proche du plafond,
`tax_mult` s'est déjà érodé pendant les années où la « marge » (slack) existait encore
(econ_ai_fiscal_slack, scps_econ.c:2184 — `rev_ramp` ET `1-lev` sont TOUS DEUX encore hauts
avant la crise de dette, donc `relax_factor`≈1, donc AUCUN frein pendant l'érosion qui MÈNE à
la crise). C0 protège contre l'aggravation UNE FOIS à la crise, pas contre la dérive qui y
mène.

**Découvertes annexes (mesurées, pas la question centrale)** :
- **Colonisation : un effet d'INTERACTION C1×C3, pas un effet simple** — ni C1 seul
  (cellule 2, 362, ≈baseline 362) ni C3/C0 seul (cellule 3, 347, ≈baseline) ne suppriment la
  colonisation isolément, mais les DEUX ENSEMBLE (cellule 1, 287, −21 % vs baseline) le font.
  Confirme l'hypothèse M8 Restes (« la fiscalité qui monte sur les prospères concurrence le
  surplus qui finance l'initiative privée M4-IP ») — nécessite C1 ET C3 actifs simultanément
  pour se manifester, ni l'un ni l'autre seul.
- **La satisfaction Laborer moyenne varie à peine (55.3-58.7 %, 3.4 pts d'écart) sur les 4
  cellules** — et est même LA PLUS BASSE en cellule 1 (tout actif, 55.3 %) et LA PLUS HAUTE en
  cellule 4 (tout mort, 58.7 %), l'INVERSE de ce que C3 est censé accomplir (viser 60 % en
  RELÂCHANT l'impôt). Le contrôleur double le taux de banqueroute pour un gain de satisfaction
  au mieux NUL, au pire négatif, à l'échelle du monde headless IA-only — la justification même
  du contrôleur n'est pas mesurée comme payante ici (peut différer avec un joueur humain qui
  réagit différemment, non mesuré, hors scope).
- **Pays au plafond de dette (stock) NE prédit PAS les banqueroutes (flux) de la même façon
  selon la cellule** — cellule 3 a le PLUS de pays au plafond (24, le maximum des 4 cellules)
  mais la 2ᵉ MEILLEURE banqueroute Σ (607) : `tax_mult`=1.0 (jamais relâché, C3 mort) donne à
  ces pays au plafond assez de REVENU pour honorer leur dette malgré le stock élevé — la
  distinction stock (dette/plafond) vs flux (revenu disponible CE mois) explique pourquoi C0
  (qui ne regarde QUE le stock/la marge instantanée) ne suffit pas : le vrai levier manquant
  est le flux (`tax_mult` lui-même), pas la marge de relâchement.
- **Révoltes (soulèvements allumés Σ) : signal BRUYANT, PAS de verdict propre** — ordre mesuré
  cellule 4 (tout off, 91) < cellule 2 (C1 off, 104) < cellule 1 (tout ON, 107) < cellule 3
  (C1 seul actif, C3/C0 morts, **158**, +48 % vs cellule 1). Cellule 3 est un OUTLIER : ni « C3
  actif protège des révoltes » (cellule 1 n'est pas la pire) ni « C1 seul est pire que tout »
  (cellule 2, C1 aussi absent, est proche du plancher) n'expliquent proprement pourquoi
  cellule 3 spécifiquement explose. Hypothèse secondaire du brief (« C0/tenir l'impôt nuit par
  un canal révoltes ») NI confirmée NI infirmée proprement — 3 graines est trop peu pour ce
  compteur, historiquement chaotique (motif M7 « sensibilité forte/non-linéaire »).
  Nécessiterait un sweep dédié (10+ graines) pour trancher, hors budget de cette mission de
  mesure.

**Pièges de mesure (pour le prochain agent DIAG)** :
- **`grep -oE "[0-9]+"` sur une ligne contenant `(M3d)` capture le « 3 » du label AVANT le
  vrai nombre** — piège trouvé en comparant une extraction automatisée (374) à une lecture
  manuelle de la même ligne (365, correct) : `grep -oE "[0-9]+"` sans ancrage matche « 3 » dans
  « M3d » PUIS le nombre réel, gonflant chaque somme de +3/sim (soit +9 sur 3 sims). Fix : `sed
  -E 's/.*M3d\) : ([0-9]+) forcée.*/\1/'` (capture group ancré, pas un grep de chiffres nu).
  **Toujours valider une extraction automatisée contre AU MOINS une lecture manuelle avant de
  faire confiance à un sweep complet** — motif déjà utile ici : le seed 9 relu à la main a
  immédiatement trahi le bug.
- **`banqueroute (M3d)` est imprimé PAR SIM (fin de partie), PAS agrégé en SYNTHÈSE** — contrai-
  rement à `colonisation`/`satisfaction moy` (déjà sommés/moyennés sur les nsims par le
  harnais). Il faut `grep` les 3 lignes (une par sim dans un run `chronicle seed 3 250`) et
  sommer forcée+volontaire à la main pour obtenir le Σ par graine — piège hérité, déjà implicite
  dans la notation « Σ » des tableaux M8/M9 mais jamais explicité comme extraction.
- **`revenu fiscal Σ … or/an (M3i neutralité)` à l'an-150 est aussi PAR SIM**, imprimé à
  l'instantané `snap[si]` (`years/5, 2/5, 3/5, 4/5` → 50/100/150/200 pour years=250) — grep
  `"an 150"` puis la ligne suivante contenant `revenu fiscal`, sommer les 3 sims.
- **Ne JAMAIS combiner `run_in_background:true` du harnais Bash AVEC un `&` shell inline sur
  la même commande** — testé ici en conditions réelles : `./chronicle.exe … &  echo started`
  avec `run_in_background:true` a produit un run TRONQUÉ (29 lignes, juste le worldgen, tué
  silencieusement) car le process enfant survit au shell parent seulement si le PARENT reste
  vivant jusqu'à `wait` — le harnais considère la commande « terminée » dès l'`echo`, pas après
  le vrai run. Fix : soit `run_in_background:true` SEUL sur la commande directe (pas de `&`
  interne), soit un script `.sh` qui backgrounde ET fait `wait` lui-même AVANT de rendre la
  main (le pattern utilisé ensuite pour la matrice complète, fiable).
- **`chronicle.exe` déjà présent en racine était bien à jour** (mtime postérieur à TOUS les
  .c/.h Monnaie touchés depuis M8/M9, vérifié par comparaison explicite avant tout run) —
  aucun rebuild MSYS2 nécessaire cette fois, mais le réflexe de vérifier reste requis (piège
  déjà noté M8 : « make test ne relie NI chronicle.exe NI scps_viewer.exe »).

**Restes** :
- **RECOMMANDATION CHIFFRÉE (estimation raisonnée, PAS un sweep de calibrage dédié — hors
  budget de cette mission de mesure pure)** : puisque C0 (`AI_DEBT_FISCAL_COHERENCE`) est DÉJÀ
  au maximum réglable (1.0) et ne rattrape que ~1.5 % de l'écart, un réglage plus agressif de
  CE curseur ne suffira PAS — son gate est structurellement réactif (regarde le stock/la marge
  instantanée), pas le flux qui s'érode en amont. Le levier à ajouter serait un **plancher
  ABSOLU sur `tax_mult` lui-même** (pas sur la marge de relâchement) — aujourd'hui la seule
  borne basse est le clamp générique `[0.02, 1.0]` (scps_econ.c:2230), sans considération de
  soutenabilité. Mesuré : sans aucun plancher, `tax_mult` descend jusqu'à 0.34 sur au moins un
  sim/3 ; avec la cellule 3 (plancher effectif = 1.0, C3 totalement mort) le revenu remonte de
  21-70 % et les banqueroutes retombent à ~pre-m8. Un plancher intermédiaire autour de
  **0.75-0.85** (borner RELÂCHER à −15/−25 % du neutre, au lieu de l'actuel −98 % possible)
  est une ESTIMATION directionnelle raisonnée à partir de ces 4 points, PAS une valeur validée
  par un balayage dédié — un futur chantier calibrage devrait tester 2-3 points bracketing
  (ex. 0.6/0.75/0.9) sur le MÊME sweep {9,11,42}×3×250 pour trouver le point qui ramène les
  banqueroutes Σ vers la bande pre-m8 (~580) sans annuler tout le bénéfice satisfaction visé
  par C3 (qui, mesuré ici, est de toute façon quasi-nul à l'échelle du monde headless IA-only
  — voir Découvertes annexes).
- **La question révoltes/initiative-privée (hypothèse secondaire du brief) reste OUVERTE** —
  signal trop bruyant sur 3 graines (voir Pièges) pour conclure. Un futur sweep dédié
  (10+ graines, séparer explicitement pic-de-révolte vs soulèvements-allumés vs guerres-
  civiles) serait nécessaire avant de trancher si C0/tenir-l'impôt nuit par CE canal.
- **Interaction C1×C3 sur la colonisation (Découvertes annexes) non creusée davantage** — 4
  points suffisent à ÉTABLIR l'interaction (ni l'un ni l'autre seul ne suffit) mais pas à la
  quantifier finement (ex. à quel niveau de TAX_SAT_COUPLING l'effet apparaît-il) — hors scope
  de cette mission (mesure du couple C1/C3 sur les BANQUEROUTES, la colonisation est un
  sous-produit noté en passant).
- Fichiers de sweep bruts (12 runs chronicle, ~59 Ko chacun, stdout complet) dans le scratchpad
  de session (hors dépôt, non committés) — non conservés dans le repo, seules les Σ/moyennes
  ci-dessus sont la trace permanente. Script `run_matrix.sh` (bash, non committé) utilisé
  pour lancer les 3 batches de cellules 2/3/4 en parallèle (3 chronicle.exe simultanés/batch,
  `wait` avant le batch suivant, marqueurs `cellN_DONE`/`ALL_DONE` explicites — motif déjà
  requis M3b-v2.1/M7/M8 « toujours attendre un fichier DONE explicite »).

## RAPPORT ÉCONOMIE/SATISFACTION — LECTURE M10 (2026-07-16)

**Mission lecture seule** (aucun build/run — un diagnostic tournait en parallèle, cf. entrée
DIAG-BANQUEROUTES ci-dessus) : `docs/RAPPORT_ECONOMIE_SATISFACTION.md` répond aux questions
A/B/C du brief M10 (« paliers de besoins »), chaque fait sourcé fichier:ligne.

**Piège notable pour M10** : `needs_met` (scps_econ.c:4283) et `satisfaction`
(scps_econ.c:4277-4278) sont DEUX métriques structurellement non comparables — `needs_met` =
`nsat/nbasket` où `nbasket` compte le panier MATURE COMPLET indépendamment du tier débloqué
(`active_needs`), alors que `satisfaction` = `basket+comfort_joy−over_tax·K−annex_scar·W`
n'est calculée QUE sur les besoins actifs mais soustrait des termes SANS RAPPORT avec les
biens (grogne fiscale, cicatrice d'annexion), sans plancher protégé avant le `clampf(0,1)`.
La trace M8DIAG « besoins comblés 15 % / satisfaction Laborer 0 % » à l'an 0 vient de LÀ, pas
d'un bug — et PAS d'un verrouillage par tier comme on pourrait le supposer : à la genèse d'un
empire jouable/IA (`EMPIRE_SEED=4000`, scps_econ.c:1818), `capitale_max_tier(4000)=4`
(scps_labor.c:42-51) débloque déjà `active_needs=5`, soit la QUASI-TOTALITÉ du panier Laborer/
Élite dès le tick 1 — le vrai goulot est la capacité de production (aucune manufacture bâtie,
tirage ≤2 raws/tuile, une seule province colonisée), PAS le tier.

**Second piège** : le rang 1 (2e besoin, débloqué dès la fondation) du Laborer (80 % de la
pop, CLASS_SHARE, scps_econ.c:600) est EAU_DE_VIE — un bien MANUFACTURÉ (scps_econ.c:582) —
alors que Bourgeois/Élite ont un BRUT en rang 1 (SALT/FUR). Aucune frontière raw/manufacturé
propre n'existe au premier palier pour la classe majoritaire ; un M10 « raws d'abord » devra
ré-ordonner `NEED_ORDER`, pas l'étendre.

**Reste** : le partage exact du delta 15 %→0 % à l'an 0 (combien vient du `basket` faible vs
de `over_tax` déjà actif au tick 1 sur le défaut de genèse satisfaction=0.5) n'est PAS confirmé
par un run — seulement plausible par les constantes du code (STATE_TAX_AMBITION=0.42 vs table
`econ_tax_tolerance`, scps_econ.c:2027-2046,2320). Un `SCPS_M8DIAG` étendu (imprimer basket/
comfort_joy/over_tax séparément) serait le prochain pas si M10 a besoin du chiffre exact.

## CHANTIER MONNAIE — M10 : LES PALIERS DE BESOINS + LE PLANCHER FISCAL (2026-07-16)

**Statut : CALIBRÉ-LIVRÉ (1 Reste net documenté, PAS un STOP).** Décision joueur (verbatim) :
« Les paliers de satisfaction augmentent avec la pop ? C'est ça l'issue ! Si tu demandes tout
day 1 forcément que la satisfaction va imploser. Si tu es petit et que an 150 t'as pas grand
chose, t'imposes aussi. Driver les besoins sur le nombre d'hab de l'empire. Chaque palier, un
besoin (universel, on se satisfait autant de bière que de papier, peu importe l'ordre). » +
seuils GÉOMÉTRIQUES validés. Tag `pre-m10` posé sur 563ec2b. 4 commits : P0 (da0b65e) · P1
(dc3204f) · correctif audit (1cd2e80) · reader façade (e3b3a50). SAVE_VERSION 94→95.

**L'ARCHITECTURE LIVRÉE** :
- **P0 — LE PLANCHER FISCAL** : `econ_ai_fiscal_tick` (C3, scps_econ.c) borne désormais sa
  case basse à `TAX_MULT_FLOOR` (défaut 0.75) au lieu du `0.02` générique — le curseur JOUEUR
  (`econ_country_tax_set`) garde SON propre 0.02, intact ; seul le CONTRÔLEUR IA est bridé.
  `TAX_MULT_FLOOR=0.02` : kill-switch exact.
- **P1 — LES PALIERS DE BESOINS** : `active_needs` (§besoins progressifs, `econ_tick`) change
  de SOURCE — remplacé par `econ_needs_active_for_country` (nouveau, scps_econ.c/.h), un
  palier GÉOMÉTRIQUE (`NEEDS_TIER_POP=3000` × `NEEDS_TIER_GROWTH=2.0`^k, plafonné
  `NEEDS_TIER_MAX=10`) piloté par la POP TOTALE DE L'EMPIRE (`epop[]`, déjà agrégée en tête
  d'`econ_tick` — AUCUN nouveau scan), HYSTÉRÉTIQUE (`g_needs_tier_held[SCPS_MAX_COUNTRY]`,
  ratchet : monte instantanément, décroît sur `NEEDS_TIER_DECAY_YEARS=5` si la pop retombe —
  motif `g_basket_pc`/`g_lowsat_streak`, même blob EMOB sérialisé). Le miroir M4-IP
  (`ip_find_shortage_building`, dupliquait le calcul avant M10) lit désormais la MÊME source.
  Le panier CŒUR n'est plus gaté par `NEED_ORDER`/rang fixe : une PRÉ-SÉLECTION par classe
  (nouveau bloc, AVANT §4 demande ET §5 consommation) retient les Kc=active_needs−1 biens les
  PLUS DISPONIBLES (stock/besoin, lu AVANT tout achat — un critère économique, pas l'ordre
  d'itération) ; seuls ceux-là (+grain, toujours palier 0 + confort-bonus poterie/statuaire,
  toujours hors panier, INCHANGÉ) sont même TENTÉS. `basket` (satisfaction) et `needs_met`
  (nsat/nbasket) jugent tous deux contre CES paliers actifs seuls, à POIDS ÉGAL (pas la valeur
  marchande) — corrige le bug identifié au RAPPORT M10 §implications 1/2 (dénominateur =
  panier mature complet, indépendant du tier). L'élasticité M5 (`g_basket_pc`, value-weighted)
  et la ration vitale sont INTACTES. `NEEDS_TIER_POP=0` : kill-switch — legacy EXACT (gate par
  rang restauré, formule value-weighted restaurée, pas seulement la valeur numérique
  d'`active_needs`).
- **Reader façade** : `scps_country_needs_tier` (scps_api.c/.h) — palier courant + pop
  requise pour le suivant, pure addition golden-neutre.

**Découvertes** :
- **`epop[]` (accumulé en tête d'`econ_tick` pour le pool national/prix) est EXACTEMENT la
  pop d'empire dont P1 avait besoin** (Laborer+Bourgeois+Élite, même définition que `rpop_nd`
  local) — aucun nouveau scan par-pays, juste un pré-passe légère qui écrit
  `g_needs_tier_held[]` avant la boucle par-province (scps_econ.c, entre l'usure des outils et
  les leviers labor-bound). `econ_ip_invest_tick` tourne DANS LA MÊME cadence mensuelle, APRÈS
  `econ_tick` (scps_sim.c:1073-1085) — son miroir lit donc la valeur du MOIS COURANT, pas
  celle du mois précédent.
- **AUDIT EXTERNE (revue indépendante, avant tout sweep) : le premier jet de P1 laissait le
  palier limiter la SATISFACTION, pas la CONSOMMATION** — en retirant le gate par rang de §4/
  §5 sans le remplacer, TOUS les biens du panier cœur étaient demandés et ACHETÉS (S[]/budget
  débités), seuls les K mieux SERVIS comptant ensuite pour `basket`/`needs_met` (sélection
  APRÈS coup, triée sur `got`). Deux défauts réels : (1) une strate pauvre dépensait sa
  richesse dans des biens qui ne lui rapportaient JAMAIS de satisfaction (fuite de richesse,
  contresens économique) ; (2) la sélection dépendait implicitement de l'ORDRE d'itération des
  ID ressource (qui avait pu acheter EN PREMIER, donc épuiser le budget partagé, avant que les
  autres candidats soient tentés) — « peu importe l'ordre » n'était vrai que pour le calcul
  final, pas pour QUI recevait l'argent. **Corrigé par une PRÉ-SÉLECTION** (lecture pure de
  `S[]`/besoin, calculée AVANT tout achat, pour CHAQUE classe, AVANT §4 ET §5) — les biens NON
  retenus ne sont ni demandés ni achetés du tout. Leçon généralisable : un mécanisme
  « n'importe lequel des N compte » DOIT décider QUI compte AVANT la dépense, jamais après —
  sélectionner après coup sur un résultat déjà consommé (budget déjà débité) ne fait que
  déplacer le biais, pas l'éliminer.
- **Le score de pré-sélection des biens VARIANTS (boisson/luxe) doit être BLENDÉ, pas juste la
  variante préférée** — piège trouvé en réparant `social_demo` (pas en le lisant à l'avance) :
  scorer un candidat EAU_DE_VIE/PRECIOUS_WARE par `S[preferred]/besoin` SEUL note un bien
  hors-culture mais SUBSTITUABLE (l'alternative abondante) comme TOTALEMENT indisponible
  (score≈0), l'évinçant systématiquement d'un slot rare — alors qu'il atteint réellement
  `got≈0.5` via le mélange `DRINK_OFFCULT`/`LUXE_OFFCULT`. Fix : le score MIROIR la formule de
  consommation réelle (`cs_p + offcult·cs_a`), pas un raccourci.
- **La chaîne manufacture→satisfaction→fisc, tracée bout-en-bout (SCPS_M8DIAG, seed 9,
  floor=0.75 final)** : un pays SAIN (pays 0, sim 1) : besoins comblés 45→100→98 %,
  satisfaction Laborer 36→100→90 %, **tax_mult Laborer RESTE À 1.00 DU DÉBUT À LA FIN** (jamais
  relâché — C3 SERRE, ne brade jamais, un pays qui a de quoi payer), revenu fiscal
  103→346→28008 or/mois (×272 sur 250 ans). Un pays EN DIFFICULTÉ (pays 35, sim 2) : besoins
  comblés plafonnent à 33 %, satisfaction Laborer 21-43 %, **tax_mult COLLE AU PLANCHER 0.75
  dès l'an 75 et n'en bouge plus** (P0 en action : avant M10, ce même pays aurait continué à
  s'éroder vers 0.34 comme mesuré par DIAG-BANQUEROUTES ; désormais 0.75 est le pire qu'il
  puisse atteindre) — revenu quasi nul (0-1 or/mois) mais STABLE, pas en chute libre.
- **`needs_tier_eff[]` (tableau local intermédiaire de la première ébauche) s'est avéré
  inutile** — remplacé par un lecteur PUBLIC unique (`econ_needs_active_for_country`) qui lit
  directement `g_needs_tier_held[]` (déjà à jour après la pré-passe) : DRY, et sert aussi le
  miroir M4-IP sans dupliquer la logique de calcul.

**Pièges** :
- **Le kill-switch « valeur identique » ne suffit PAS pour un mécanisme qui change de
  FORMULE, pas seulement de PARAMÈTRE** — première itération : `NEEDS_TIER_POP=0` faisait
  retomber `active_needs` sur sa valeur legacy EXACTE (`1+capitale_max_tier(pop locale)`),
  mais la boucle §4/§5 restait ENTIÈREMENT ungated (gate par rang supprimé sans condition) —
  résultat : `basket`/`needs_met` utilisaient la NOUVELLE formule (poids égal, sélection) même
  au kill-switch, avec la MÊME valeur numérique d'`active_needs` que l'ancienne — golden pré-
  M10 PAS byte-identique (`--hash 7 5 12` divergent dès la 1ʳᵉ graine), détecté immédiatement
  par le gate 1. Fix : un booléen `p1_on` explicite thread À TRAVERS §4 ET §5, chaque branche
  gardant SES DEUX chemins (accumulateurs LEGACY `met_w`/`nsat`/`nbasket` calculés EN PARALLÈLE
  et harmless quand inutilisés, jamais supprimés). Leçon : quand un kill-switch doit prouver
  un golden byte-identique, VÉRIFIER qu'il coupe TOUTE la chaîne de calcul modifiée, pas
  seulement la valeur d'entrée qui la pilote — un test au premier `--hash` AVANT tout sweep
  (comme prescrit par le brief) l'a attrapé en une commande, pas en 250 ans de sweep.
- **`social_demo` « chacun sa boisson » a cassé APRÈS le correctif audit, pas avant** — la
  pré-sélection par disponibilité fait GAGNER les biens abondants contre un bien hors-culture
  pénalisé quand `Kc<n_cand` (compétition de slot) ; le fixture isolait le signal boisson en
  rendant TOUS les AUTRES biens sociaux triviallement abondants (1e5) à pop=1250 (tier 1,
  Kc=1) — l'exact scénario où la compétition masque le signal testé. `elite_sat_with_luxe`
  (même fichier) ne l'a PAS attrapé car il utilisait DÉJÀ pop=4100 (tier 4, Kc=4=n_cand,
  aucune compétition) — un précédent qu'il suffisait de suivre, pas un bug nouveau à inventer.
  Leçon : un fixture qui isole un signal en rendant « tout le reste trivial » peut désormais
  faire le CONTRAIRE de ce qu'il visait — le trivial devient le concurrent qui évince le signal.
- **Le breach invariant M3c isolé (graine 11, sim 1, an 19-23) est un accumulateur WILD
  PRÉ-EXISTANT, pas un bug P0/P1** — `[INVDIAG-WILD-TOP]` montre une province NON colonisée
  (`colonized=0`, pop L/B/E=0/0/0) avec une richesse BOURGEOISE fantôme (`wealth...B=11411`,
  croissant tick après tick) : un accumulateur de richesse déjà documenté par un diagnostic
  DÉDIÉ *pré-existant* (`chronicle.c:360`, « M3e DIAG… temporaire pour la chasse au breach
  graine 11 an 57 » — cette fragilité PRÉCÈDE M10 de plusieurs vagues). Isolé par kill-switch
  matriciel (30 ans, seed 11) : P0 SEUL (P1 mort) → 10 échecs ; P1 SEUL (P0 mort) → 0 échec ;
  les deux → 6 ; ni l'un ni l'autre → 0. P0 est le déclencheur (déplace la trajectoire fiscale
  assez pour faire buter le pays sur ce leak pré-existant), P1 ATTÉNUE (10→6) sans l'effacer.
  Balayage `TAX_MULT_FLOOR` 0.60-0.85 : NON-MONOTONE (0.70 → 24 échecs, PIRE que 0.80 ; 0.60/
  0.65/0.75 → 2 chacun) — motif M8 « bifurcation, pas un gradient », confirmé une fois de plus.
  0.75 retenu (meilleur point mesuré), 2 années résiduelles (383-389 % vs seuil 370 %) — cause
  HORS scope M10 (confirmée par le coordinateur : vague corrective séparée prévue sur
  « frappe libre »/« trésor prov/region »), documentée en Restes, PAS un STOP.
- **`m10_diag_trace.sh` référençant un worktree déjà retiré (`git worktree remove`) échoue
  SILENCIEUSEMENT sur SA branche en arrière-plan (`&`), écrasant le fichier de sortie précédent
  par un message d'erreur d'une ligne** — motif déjà noté (M3b-v2.1, « toujours vérifier
  `wc -l` avant de faire confiance à un fichier de sweep produit en arrière-plan ») ; ici
  appliqué à UN fichier sur deux (le second, indépendant, réussissait). Les 2 traces
  pré-m10/HEAD ne peuvent PAS être régénérées après `git worktree remove` sans recréer le
  worktree — retenir la donnée AVANT de nettoyer, pas après.

**Mesures (sweep apparié pre-m10 vs HEAD, `{9,11,42}×3×250`, floor=0.75 final)** :

| seed | banqueroutes Σ | colonisation Σ | Laborer sat. finale | revenu fiscal an-150 Σ | invariant pic max |
|---|---|---|---|---|---|
| 9  | 365→165 (−55 %) | 67→67 (=) | 54→66 % (+2pt hors plafond) | 181097→263699 (+46 %) | 80→89 % |
| 11 | 506→265 (−48 %) | 73→69 (−5 %) | 54→70 % (+6pt hors plafond) | 77001→53302 (−31 %) | 361→**389 %** (2 ans, BREACH) |
| 42 | 312→153 (−51 %) | 147→127 (−14 %) | 58→62 % (dans bande) | 310847→402381 (+29 %) | 105→108 % |
| **Σ** | **1183→583** | **287→263** | — | **568945→719382 (+26 %)** | — |

Banqueroutes Σ **583, à 0.2 % de la cible pre-m8 (582)** — le résultat DIAG anticipé, atteint
quasi exactement · colonisation STABLE/saine (aucune suppression massive, l'interaction C1×C3
du DIAG — la fiscalité qui monte concurrence l'initiative privée — ne se manifeste plus dans
ces proportions une fois les paliers atteignables) · revenu fiscal Σ +26 % (2/3 graines
améliorées, seed 11 régresse -31 %, cf. Restes) · Laborer 2/3 graines AU-DESSUS du plafond
50-64 % historique (seed 9 +2pt, seed 11 +6pt ; seed 42 dans bande) — la bande elle-même a été
calibrée SOUS l'ancien système (dénominateur gonflé artificiellement) et mérite probablement
un réexamen, pas un signe de régression · invariant 8/9 sain, 1/9 breach net documenté
(graine 11 sim 1, cause pré-existante hors scope).

**Gates** :
1. **Kill-switch prouvé** (`TAX_MULT_FLOOR=0.02,NEEDS_TIER_POP=0` → `--hash 7 5 12`
   BYTE-IDENTIQUE au golden pré-M10, re-vérifié à CHAQUE itération du correctif) ✓.
2. **Sweep apparié** — bandes ci-dessus ; gate spécial banqueroutes Σ 583 ≤ 700 (cible 582)
   ✓ LARGEMENT atteint.
3. **`make test`** 38/38 verts (intertrade_demo seul, pré-existant Windows) + `social_demo`
   réparé (fixture, moteur intact) ✓ · **golden RE-BASELINÉ** (defaults P0/P1 actifs) ✓ ·
   `make determinism` STABLE (5 graines×12 ans) ✓ · `make determinism-deep` STABLE (graines
   7/9×200 ans) ✓ · `scps_viewer --savetest 9` A==B byte-identique + octet altéré REFUSÉ ✓ ·
   `make fuzz-save` 8/8 (216 octets, 0 crash) ✓.
4. **Cet append TROUVAILLES** + commits granulaires français ✓.

**Restes** :
- **Invariant M3c : 1/9 breach net** (graine 11 sim 1, an 19-23, 383-389 % vs seuil 370 %) —
  cause TRACÉE (accumulateur WILD pré-existant, `chronicle.c:360`, hors scope M10, cf.
  Pièges), PAS résolue (le coordinateur a explicitement demandé de ne pas y toucher, une vague
  corrective séparée est prévue sur « frappe libre »/« trésor prov/region »). Un futur audit
  crédit/trésor devrait re-tester ce sim UNE FOIS ce leak corrigé.
- **Revenu fiscal seed 11 régresse (-31 %)** — l'ÉCONOMIE de cette graine est structurellement
  plus faible sur PLUSIEURS métriques (commerce/an 420→359, pays au plafond variable) ; le
  floor=0.75 (moins strict que 0.80) donne à C3 plus de marge pour RELÂCHER un pays en peine
  vers son objectif satisfaction (70 % atteint, dépassant même le plafond band historique) au
  prix du revenu agrégé. Non isolé plus finement (aurait exigé un sweep dédié gelant floor vs
  la bande Laborer) — signalé, pas creusé, hors budget de cette vague.
- **La bande Laborer 50-64 % (héritée de M7/M8) n'a probablement plus le bon calibrage** —
  P1 corrige un dénominateur ARTIFICIELLEMENT gonflé (needs_met/basket contre le panier mature
  complet au lieu des paliers actifs) qui déprimait mécaniquement la satisfaction sous
  l'ancien système ; 2/3 graines dépassent désormais 64 % en fin de partie. Ce n'est PAS un
  signe que P1 « triche » — c'est la conséquence directe et attendue du bug corrigé — mais la
  bande elle-même (motif M7 « seuil jamais élargi ») mérite un réexamen dédié par le joueur
  plutôt qu'un élargissement décidé unilatéralement ici.
- **UI-MONNAIE dédiée non câblée** (`scps_country_needs_tier` prêt côté scps_api, aucune
  demande GDScript cette vague). **DLL Godot À RE-BUILDER** (scons -C godot) : scps_econ.c/h
  et scps_api.c/h ont changé (nouveaux symboles exportés `econ_needs_active_for_country`,
  `econ_needs_tier_threshold`, `scps_country_needs_tier`) — motif déjà noté à chaque vague
  monétaire.
- **`needs_tier_selected`/la pré-sélection ne re-teste PAS le confort-bonus (poterie/
  statuaire)** — laissé UNGATÉ par le palier (toujours tenté, hors panier, décision de scope
  documentée dans le code) ; un futur chantier pourrait vouloir les inclure dans la
  compétition de slot si le joueur juge que « tout bien devrait compter également, poterie
  comprise » — hors du périmètre « panier cœur » explicite de ce brief.
- Fichiers de sweep bruts (12+ runs chronicle, worktree `/c/tmp_wt_pre_m10` retiré en fin de
  session) non conservés dans le repo — seules les Σ/moyennes ci-dessus sont la trace
  permanente. Scripts `m10_p2_sweep.sh`/`isolate_breach.sh`/`extract_p2.sh` (bash, non
  committés, scratchpad) utilisés pour le sweep et l'isolation du breach — motif déjà requis
  M3b-v2.1/M7/M8/DIAG « toujours attendre un fichier DONE explicite ».

## CHANTIER MONNAIE — M11 : LA VAGUE AUDIT-SOL (2026-07-16)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés, invariant M3c
AMÉLIORÉ (0/9 breach au calibrage final, le breach graine 11 an 19-23 documenté M10 a DISPARU
sans y toucher directement — l'optionnel du brief pris PAR EFFET DE BORD du calibrage A3).**
Origine : un audit externe (revue indépendante), 4 claims — CHAQUE claim vérifié AU CODE avant
correctif. Tag `pre-m11` posé sur d8efa34 (M10). A3 REFONDU en cours de mission sur décision
joueur (« intérêt FIXE : 1000 à 5 % ⇒ tu rembourses 1050, pas +5 %/an » — la v1
arriéré-qui-capitalise, déjà implémentée et testée, a été REMPLACÉE, cf. Découvertes).

**LES 4 CLAIMS DE L'AUDIT — verdicts** :
- **A1 (frappe sous parité) : CONFIRMÉ.** scps_econ.c ~4973 pré-M11 (bloc frappe LIBRE M3e) :
  le crédit trésor était `gain = qty×(parité−prix)` SEUL ; `cost` n'était qu'une variable de
  GATE (`treas_remaining-=cost` commenté « comptabilité de GATE seule ») — jamais débitée,
  jamais versée à un vendeur. Le métal du marché disparaissait sans contrepartie (prouvé au
  banc : stock 1000→974, richesse classes 0.00, FX_MINT 266.7 = le gain seul). La frappe
  ROYALE (réserve d'État, econ_country_mint_month) était DÉJÀ correcte : `v0 = g×PARITY_GOLD +
  cop×PARITY_COPPER` = parité pleine, métal prélevé en nature (royalty), rien à payer — les
  DEUX chemins ont des règles différentes et c'est CORRECT (le brief l'anticipait).
- **A2 (trésor périmé un mois) : CONFIRMÉ, périmètre PLUS LARGE que le claim.** econ_
  aggregate_regions() (econ_tick ~4865) matérialise prov[]→region[] AVANT les deux frappes ;
  les frappes créditaient `prov[cap].treasury` NU ⇒ econ_country_gold/credit_can_spend/
  credit_line/audit_eco (tous region[]-grain) lisaient un trésor périmé un mois. EN PLUS du
  claim : credit_year_tick (intérêts, amortissement-vers-cs, saisie M3g — 4 sites d'écriture
  nue) pouvait laisser la vue périmée jusqu'à UN AN ; et debit_surplus_prorata (le débiteur
  UNIVERSEL de toute la chaîne crédit, signalé « Reste » par M9) ne débitait JAMAIS region[].
  Le masque du banc (credit_demo.c:103 pré-M11, econ_aggregate_regions manuel après
  credit_year_tick) confirmé — RETIRÉ, le banc passe sans (contrôle 8, ROUGE sur pre-m11).
- **A3 (pas de défaut réel) : CONFIRMÉ.** scps_credit.c ~521 pré-M11 : `covered+CR_EPS <
  interest ⇒ g_defaults++` — une statistique MONDE, ni capitalisée, ni arriéré ;
  insolvent_streak (~591) ne réagissait QU'au plafond. Prouvé au banc : 20 ans d'impayés
  TOTAUX, dette sous plafond ⇒ JAMAIS de banqueroute forcée (contrôle 10-legacy, le scénario
  exact « un pays à 200 % sans trésor ne fait JAMAIS faillite »).
- **A4 (bancs manquants) : CONFIRMÉ.** credit_demo pré-M11 = 20 contrôles, aucun sur
  banqueroute volontaire/forcée, impayés multi-années, saisie post-faillite,
  frappe/conservation, cohérence prov==region sans masque.

**L'ARCHITECTURE LIVRÉE** :
- **A1 — LA FRAPPE À PARITÉ PLEINE** (`MINT_FULL_PARITY`, défaut 1, kill-switch exact) : la
  frappe libre PAIE désormais le vendeur (débit trésor réel prorata régions, motif ROADS ;
  crédité aux 3 classes de CHAQUE région qui a fourni le métal, clé 42/20/38 item 5) PUIS crée
  à la PARITÉ PLEINE (`econ_prov_treasury_credit(e, cap, qty*parity)` + FX_MINT = la VRAIE
  création). Chiffres banc (fixture 1 province, or coté 8, parité 16) : legacy FX_MINT=266.7
  (gain seul) → A1 FX_MINT=512.7 (parité pleine, royale 120×16/12/mois incluse), vendeur payé
  150 or de richesse classes, conservation ΔM==FX_MINT exacte à l'arrondi.
- **A2 — LE TRÉSOR UNE-SEULE-VÉRITÉ** : nouveau helper `econ_prov_treasury_credit(e,pid,delta)`
  (scps_econ.c/h — dual-write prov[pid]+region[], pour une province DÉJÀ résolue : différent
  d'econ_region_treasury_add qui résout via region_carrier_prov et peut router sur un cache
  rep_prov PÉRIMÉ) ; le CONTRAT « qui écrit quand » documenté en tête d'econ_aggregate_regions
  (RÈGLE : tout écrivain monétaire post-agrégation passe par econ_region_treasury_add si cible
  = ensemble, econ_prov_treasury_credit si cible = province résolue, JAMAIS `treasury +=` nu) ;
  les 2 frappes + 4 sites credit_year_tick convertis ; debit_surplus_prorata dual-write (ferme
  le Reste M9 côté PRÊTEUR). Choix documenté : dual-write O(1) par écriture plutôt qu'une
  seconde agrégation O(n_prov) — rien demandé aux futurs lecteurs, seulement aux écrivains.
- **A3 v2 — L'INTÉRÊT FIXE + LE DÉFAUT RÉEL** (`DEBT_FIXED` défaut 1, kill-switch exact ;
  remplace la v1 arriéré-qui-capitalise sur décision joueur en cours de mission) :
  `debt_origination(c, borrow)` = borrow×(1+credit_current_rate(c)) FIGÉ à l'emprunt, appliqué
  aux 4 sites d'origination (credit_borrow_local §2, credit_borrow_class V1,
  credit_borrow_citystate, credit_borrow_state V2) — JAMAIS au rachat V3 (une créance qui
  change de mains n'est pas une origination). credit_year_tick : plus de rente annuelle sur le
  stock — une ÉCHÉANCE MINIMALE (`DEBT_DUE_FRAC`=0.10 × stock) payée du surplus SEUL, qui
  ÉTEINT le stock (le flux rembourse principal+markup blended) ; l'impayé ne capitalise JAMAIS
  (« fixe veut dire fixe ») mais nourrit insolvent_streak si la dette dépasse
  `DEBT_DEFAULT_THRESHOLD` (3000, cf. calibrage) ⇒ banqueroute forcée après
  BANKRUPTCY_GRACE_YEARS (5) — le OU (plafond OU impayé-substantiel) est LE cœur du correctif.
  Retenue M3i : sous fixed, seule la part INTÉRÊT de chaque remboursement (taux/(1+taux)) est
  un revenu imposable — le principal remboursé n'est pas un gain.
- **A4 — LES BANCS** : credit_demo 20→48 contrôles. Nouveaux : cohérence prov==region SANS
  ré-agrégation manuelle (post-credit_year_tick ET post-frappe) · markup à l'origination +
  kill-switch · « fixe veut dire fixe » (10 ans d'impayés, dette inchangée) · défaut réel
  (impayés ⇒ streak ⇒ banqueroute forcée SOUS le plafond) + legacy qui reproduit le bug ·
  plancher dette-qui-compte (résidu trivial protégé) · banqueroute forcée + télémétrie +
  cicatrice + saisie M3g réglée au créancier figé · banqueroute volontaire · conservation
  frappe ΔM==FX_MINT + kill-switch A1. PROUVÉ ROUGE SUR PRE-M11 : le banc HEAD compilé contre
  le moteur pre-m11 (worktree) → 42/48, les 6 échecs = exactement les contrôles neufs (A2×3,
  A3-markup, A3-défaut, A1-vendeur).

**Découvertes** :
- **La refonte A3 v1→v2 en plein vol** : la v1 (l'arriéré capitalise aux conditions du prêt,
  DEBT_ARREARS) était IMPLÉMENTÉE, TESTÉE (46/46) et golden-safe quand la décision joueur est
  tombée (« intérêt fixe... pas +5 % par an »). REFONDUE proprement plutôt que rapiécée : le
  tunable DEBT_ARREARS retiré (jamais commité), DEBT_FIXED introduit — la leçon : quand le
  MODÈLE change (composé→fixe), remplacer le mécanisme entier, pas superposer deux régimes.
- **La fraction d'échéance n'était PAS le levier de calibrage — la LARGEUR du déclencheur
  l'était.** DEBT_DUE_FRAC balayé 0.02/0.03/0.05/0.10 : Σ banqueroutes ~1900-2000 sur TOUS les
  points (bifurcation, pas un gradient — motif M7/M8 confirmé une fois de plus). Le vrai
  coupable : n'importe quel RÉSIDU de dette (même 51 or) d'un pays qui ne repasse jamais
  SINK_FLOOR de trésor déclenchait le streak. Fix : DEBT_DEFAULT_THRESHOLD (plancher « dette
  qui compte », DÉLIBÉRÉMENT séparé de BUYBACK_DEBT_THRESHOLD — coupler les deux aurait fait
  dériver le taux de rachat en calibrant le défaut). Balayage 4 points : sans plancher ~1950
  (+234 %) · 500 → 1110 (+90 %) · 1500 → 910 (+56 %) · 3000 → 795 (+36 %, retenu).
- **Le calibrage A3 à 3000 fait DISPARAÎTRE le breach invariant graine 11** (an 19-23,
  383-389 %, l'accumulateur WILD pré-existant documenté M10) : 0/9 breach sur le sweep final
  contre 1/9 pre-m11 — l'optionnel du brief (« si ton contrat A2 règle naturellement ce site,
  prends-le ») obtenu par le chemin A3 : les trajectoires de dette plus courtes (le stock
  s'éteint, 597k→66k or de dette monde fin de partie) déplacent la trajectoire de la graine
  hors de la zone du leak. Le SITE WILD lui-même (péages parqués sur porteuses non colonisées,
  M3h/M3i item 7) N'EST PAS résolu — au seuil 1500 un breach APPARAISSAIT ailleurs (graine 112
  sim 2, an 114-147, 400-408 %) : la fragilité se déplace avec la trajectoire, le leak
  sous-jacent reste désigné pour une vague dédiée.
- **A1 ne rapproche PAS (encore) le prix du métal de la parité** — l'espoir du brief mesuré
  honnêtement : or prix moyen 3.00→2.74 (parité 16), cuivre 0.94→0.83 (parité 5.2) — bruit
  inter-graines dominant, PAS d'amélioration. Cause plausible (non creusée, hors budget) : le
  vendeur payé est la RICHESSE des classes (wealth), pas le mécanisme de PRIX — le signal
  demande de la frappe (g_mint_demand_prev, M3f) existait déjà et n'a pas changé d'échelle.
  Le signal M3f « convergence prix-métal→parité » reste OUVERT.
- **La dérive M7 tient** : moyenne 9 sims −0.10 %/an (pre −0.32 %/an), fourchette ±3 %/an
  dominée par la bifurcation par-sim (variance déjà assumée M7). M(fin) croît davantage sous
  A1 (ex. graine 9 sim 1 : 21.1M→41.4M) — la création à parité pleine est PLUS de monnaie,
  cohérent par construction, l'invariant la compte comme frappe documentée.

**Pièges** :
- **`credit_spend` passe par la PÉRÉQUATION avant d'emprunter** — un banc qui veut une dette
  PRÉCISE (pour tester le plancher du défaut) doit emprunter par `credit_borrow_citystate`
  DIRECT : via credit_spend, la péréquation absorbe une partie du besoin et la dette inscrite
  est plus petite que la dépense (400 dépensés ⇒ ~300 de dette sur le fixture — le contrôle 10
  v1 échouait pour CETTE raison, pas un bug moteur).
- **Le fixture conservation (contrôle 13) doit poser le trésor EXACTEMENT à SINK_FLOOR (500)** :
  au-dessus, la redépense publique I3bis (STATE_SPEND_RATE, scps_econ.c) mord le surplus et,
  SANS pop/impôt dans le fixture (coll_tot=0), la part payroll ne revient à AUCUNE classe — un
  site de destruction DISTINCT, déjà classé par M0, qui fausse ΔM==FX_MINT de −25 or/tick.
  L'annuler par construction isole proprement ce que A1 change.
- **`tune_set` sur un nom inconnu est un no-op silencieux** — le banc HEAD tourne contre le
  moteur pre-m11 (preuve du ROUGE) sans erreur : les tune_set("DEBT_FIXED"/"MINT_FULL_PARITY")
  n'existent pas dans son registre, les contrôles legacy passent, les contrôles neufs échouent
  — exactement le comportement voulu pour la preuve, mais à savoir pour tout futur banc
  bi-époque.
- **Piège d'extraction déjà documenté (DIAG) reconfirmé** : `grep -oE "[0-9]+"` sur la ligne
  banqueroute capture le « 3 » de « (M3d) » — TOUJOURS le sed ancré
  `s/.*: ([0-9]+) forcée.*/\1/`.

**Bandes mesurées (sweep apparié pre-m11 vs HEAD final=seuil 3000, {9,11,42}×3×250)** :

| métrique | pre-m11 (M10) | HEAD (M11) | verdict |
|---|---|---|---|
| banqueroutes Σ | 583 (165+265+153) | **795** (252+315+228) | +36 % — le défaut VIT (impayés réels), SOUS le doublement |
| colonisation Σ | 263 | 237 | −10 %, dans la bande ±10 % tolérée M3i/M5 |
| Laborer sat. moy | 66/70/62 % | 66/60/62 % | comparable (graine 11 −10 pts, bruit) |
| invariant | 1/9 breach (graine 11 an 19-23) | **0/9 breach** | AMÉLIORÉ — breach M10 disparu |
| dérive prix M7 | −0.32 %/an moy | −0.10 %/an moy | comparable, variance inter-sims dominante |
| hégémon craqué | 1/9 | 2/9 | compteur chaotique, comparable |
| dette monde fin | 596 888 or | 66 369 or | la dette S'ÉTEINT enfin (échéances + défauts réels) |
| rachats Σ (V3 vit) | 2361 | 2091 | le marché secondaire vit toujours |
| convergence or/cuivre | 3.00 / 0.94 | 2.74 / 0.83 | PAS d'amélioration (signal M3f toujours ouvert) |

Élites rentières : le flux annuel qu'elles reçoivent n'est PLUS une rente perpétuelle
(intérêt sur un stock immortel) mais l'ANNUITÉ d'un prêt qui s'éteint (principal+markup) —
en contrepartie le capital REVIENT (remboursé ou cashout rachat) au lieu de rester une
créance morte ; les rachats V3 (2091 vs 2361) prouvent que le circuit rentier vit toujours.
Bande documentée, pas de recalibrage demandé.

**Gates (tous passés)** :
1. Claims vérifiés au code AVANT correctif (ci-dessus) ✓.
2. Kill-switches : `MINT_FULL_PARITY=0,DEBT_FIXED=0` → hash 02560620/7cf6e226/076966e2/
   10e5c90e/a83ca87f — DIFFÉRENT du golden pre-m11 par le SEUL effet A2 (dual-writes region[] :
   frappe via econ_prov_treasury_credit, debit_surplus_prorata, intérêts/saisie), fix
   d'ordonnancement NON gatable proprement — LA décision documentée que le brief autorisait
   explicitement. Preuve interne : les kill-switches coupent bien A1 (gain seul, vendeur jamais
   payé — contrôle 13) et A3 (aucun markup, streak plafond seul — contrôles 9/10) au banc ✓.
3. Sweep apparié (tableau ci-dessus) ✓ — banqueroutes recalibrées 2 fois (v1 → seuil 500 →
   1500 → 3000), mesuré jamais déclaré.
4. `make test` 38 verts/0 rouge + credit_demo 48/48 (intertrade_demo seul, pré-existant
   Windows setenv — reconfirmé par build direct) · nouveaux contrôles ROUGES sur pre-m11
   (42/48, les 6 échecs = les contrôles neufs) et VERTS sur HEAD ✓ · golden RE-BASELINÉ puis
   VERT ✓ · determinism STABLE ✓ · determinism-deep STABLE (7/9×200 ans) ✓ · savetest 9 A==B
   byte-identique + octet altéré REFUSÉ ✓ (aucun nouvel état sérialisé : le markup vit dans
   to_class/to_cs existants, underpaid est intra-appel, streak réutilise le champ v90 —
   SAVE_VERSION inchangé 95) · fuzz-save 8/8 (216 octets, 0 crash) ✓.
5. Cet append + commits granulaires FR ✓.

**Restes** :
- **Le site WILD des péages parqués (M3h/M3i item 7) toujours PAS résolu** — le breach graine
  11 a disparu par déplacement de trajectoire (calibrage A3), pas par conversion du site ; au
  seuil 1500 un breach équivalent apparaissait ailleurs (graine 112 sim 2). La vague dédiée
  « région-carrier → province colonisée » reste désignée.
- **Convergence prix-métal→parité (signal M3f)** : A1 ne l'améliore pas — le vendeur payé
  enrichit la classe, pas le mécanisme de prix. Si le joueur veut la convergence, le canal à
  regarder est le POIDS du seed de demande de frappe (g_mint_demand_prev) dans le solde du
  prix national, pas le paiement.
- **DEBT_DUE_FRAC=0.10 posé, jamais optimisé finement** (la fraction s'est avérée un
  non-levier sur la bande banqueroutes ; son VRAI effet est la vitesse d'extinction du stock —
  visible dans « dette monde fin » 597k→66k) — un futur calibrage joueur pourrait vouloir un
  horizon de remboursement différent (~10 ans actuellement).
- **UI-MONNAIE** : aucun reader façade neuf demandé cette vague (les readers M9
  loan_capacity/loan_status exposent déjà credit_current_rate — le taux affiché EST désormais
  le taux FIXE proposé à l'origination, cohérent sans changement). **DLL Godot À RE-BUILDER**
  (scons -C godot) : scps_econ.c/h, scps_credit.c, scps_tune_list.h ont changé — motif noté à
  chaque vague monétaire.
- Worktree de sweep `/c/tmp_wt_pre_m11` retiré en fin de session ; fichiers de sweep bruts
  (21 runs chronicle) au scratchpad, non committés — seules les Σ ci-dessus font foi.

## CHANTIER MONNAIE — M12 : L'ÉQUILIBRE DE BASE (2026-07-16)

**Statut : CALIBRÉ-LIVRÉ — golden RE-BASELINÉ VERT, gates complets passés, LA dette de
fonctionnement early ÉLIMINÉE (an 2 : Σ 6034→0 sur 9 sims ; an 12 : 29 976→317, −98.9 %).**
Décisions joueur (verbatim) : « le fonctionnement de l'état de base est trop cher pour se
maintenir » · « Si l'état achète au prix du marché, c'est un très mauvais négociant. L'état
doit prendre sa part, la taxe, générale. Donc, l'état achète à 60 % du prix du marché,
tunable. » · règle de calibrage : « Si la bande Laborer prend, c'est que leurs taxes sont
trop élevées ET que leurs biens ne donnent pas assez de satisfaction » — le 0.60 ne se
recalibre JAMAIS. Tag `pre-m12` posé sur bedc7df (M11). 5 commits : E1 (73c21ef) ·
E2 (d8c9561) · E3 diags (28ffae9) · golden (d55345c) · docs (ce commit). SAVE_VERSION 95
INCHANGÉ (aucun nouvel état sérialisé — tout est tunable registre J + statics print-only).

**E1 — LE TABLEAU P&L DE L'ÉTAT DE BASE (SCPS_PLDIAG, la mesure d'abord).** États jouables
EN PAIX SANS CHANTIER (filtre : ni DIPLO_WAR ni FX_BUILD actif l'année), moyenne pondérée
{9,11,42}×3 sims (frame 6 empires/12 cités, 160 ans), or/mois/empire, AVANT tout fix
(pre-m12 exact) :

| an | taxes | frappe | assiette | **achat-État** | entretien | soldes | marine | import | redépense | solde |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1  | +103.6 | +90.3 | +56.6 | **−339.7** | −2.8 | −0.4 | 0.0 | −35.3 | −24.8 | ~−152 |
| 3  | +34.2 | +56.0 | +20.0 | **−74.7** | −1.0 | −7.7 | −17.5 | −4.5 | −3.2 | ~+1 |
| 5  | +27.6 | +26.5 | +16.2 | **−58.9** | −0.6 | −5.7 | −6.1 | −1.1 | −2.1 | ~−5 |
| 12 | +21.0 | +23.3 | +3.1  | **−23.6** | −0.0 | −3.6 | −21.0 | −1.8 | −0.1 | ~−4 |
| 50 | +104.1 | +55.5 | +62.0 | **−163.3** | −3.7 | −8.3 | −25.7 | −5.2 | −6.5 | ~+10 |
| 150| +4.1 | +15.8 | +1.2 | **−1.4** | 0.0 | −4.5 | −13.0 | −3.2 | −0.1 | ~−1 |

LA LIGNE COUPABLE est « achat-État » (le circuit M3b-v2 : l'État achète la VA au
price_level) : −339.7/mois à l'an 1 contre +250.5 de revenu TOTAL — cour/admin/encadrement
sont à 0.00 partout (gatés par COURT_FLOOR/hoarding, jamais atteints en early), l'entretien
est dérisoire (−0.04 à −3.7), les soldes confirment « y'a pas d'armée an 2 » (−0.4). Aucun
autre suspect ne tient la comparaison. (« achat-État » et « assiette » par pays n'avaient
AUCUN bucket FX_* — câblés print-only pour ce tableau, econ_pldiag_*.)

**E1 — L'AMORÇAGE GENÈSE : CONFIRMÉ AGGRAVANT, mesuré pas supposé.** scps_econ.c ~3568 :
`price_level=1.f` codé en dur quand `va_country_prev<=EPS` (« prix plein, non contraint »).
Le piège est PIRE que le brief le pensait : la branche se re-déclenche pour CHAQUE
pays/cité-état NOUVEAU tout du long des 12 premières années (pas une fois à la genèse du
monde) — un pays qui produit son premier tick de VA tente de la payer à 100 % avec sa seule
réserve de genèse M5 (100 or), le déficit devient country_shortfall ⇒ péréquation ⇒ emprunt
aux classes ⇒ LA dette de fonctionnement. Patch expérimental isolé (0.f au lieu de 1.f,
AVANT E2) : dette Σ an-2 6034→239 (−96 %), an-12 29 976→782 (−97 %) sur {9,11,42}×3. Livré
`PL_GENESIS` (défaut 0.0, registre J, 1.0 = legacy byte-identique) : « démarrer bas,
converger » — dès le 2e tick, `va_country_prev` est peuplé et le ratio caisse/VA reprend.

**E2 — STATE_BUY_FRAC (le cœur).** `pf_buy = price_level × STATE_BUY_FRAC` (défaut 0.60)
au SEUL site §3 (pay_wage/profit/tax) ; la REVENTE (prix national §clôture + assiette M5)
reste au price_level PLEIN. La marge de 40 % n'est jamais « versée » nulle part — elle
n'est simplement JAMAIS DÉBITÉE du trésor (prélèvement à la source, pas un flux). La vie
d'un achat d'État type : AVANT — province produit VA 100, l'État doit 100×pf aux classes,
caisse insuffisante ⇒ dette ; APRÈS — l'État paie 60×pf (42/20/38), garde 40×pf, revend au
plein prix ⇒ l'écart couvre le fonctionnement, l'emprunt de paix disparaît. WILD/hameaux
(démonétisés) et provinces owner<0 (fixtures) : INTACTS (pas d'État, pas de marge).

**E3 — LE SWEEP APPARIÉ (frame M11 : `./chronicle <seed> 3 250`, empires variables — LE
frame des références 795/66k/237, vérifié : pre-m12 le reproduit EXACTEMENT ; le frame
`6 12` des vagues M3b/M3c donne des absolus ~2× plus gros, piège de comparaison notable)** :

| métrique | pre-m12 (M11) | HEAD (M12) | verdict |
|---|---|---|---|
| dette early an 2 Σ (9 sims) | 6 034 | **0** | ZÉRO — 9/9 sims |
| dette early an 12 Σ | 29 976 | **317** | −98.9 % |
| emprunt de PAIX (or/pays-an) | +62.8/+160.3/+77.3 | **+10.4/+6.4/+1.1** | −84 à −99 % — l'emprunt de paix est devenu RARE |
| emprunt de GUERRE (or/pays-an) | +491.3/+156.2/+108.9 | +10.8/+2.5/+3.2 | la dette résiduelle est minuscule, du même ordre guerre/paix |
| banqueroutes Σ | 795 (252+315+228) | **28** (12+0+16) | −96 % (gate ≤795 : ÉCRASÉ) |
| dette monde fin Σ | 65 369 | **16 982** | gate ≤66k : LARGEMENT tenu |
| colonisation Σ (fondations) | 237 | **315** | +33 % — pas de suppression, au contraire |
| Laborer sat. | 66/60/62 % | **71/60/61 %** | bande TENUE (plancher 50) — AUCUN rééquilibrage requis |
| Bourgeois / Élite | 77/62/72 · 71/60/57 | 70/84/87 · 69/82/68 | dans/au-dessus des bandes |
| IPM final moyen (dérive M7) | 0.97/0.90/0.90 | 0.94/0.92/0.87 | comparable |
| invariant M3c | 0/9 | **0/9** (avec plancher de résolution) | cf. Découvertes |

La règle du joueur (baisser la fiscalité Laborer / monter le rendement satisfaction) n'a
PAS été invoquée : la bande n'a pas cassé — elle est montée sur seed 9 (66→71).

**LA PRESSION FISCALE TOTALE PAR ORDRE (PLDIAG-FISC, seed 9 sim 1 HEAD, (marge E2 +
retenue M3i)/(paie plein-prix), curseurs M8 et évasion inclus dans la retenue)** :

| an | Laborer | Bourgeois | Élite |
|---:|---:|---:|---:|
| 2   | 41 %  | 102 % | 72 %  |
| 12  | 75 %  | 93 %  | 123 % |
| 50  | 51 %  | 88 %  | 98 %  |
| 150 | 54 %  | 78 %  | 93 %  |

Progressivité PRÉSERVÉE : le Laborer est l'ordre le MOINS pressé (l'exonération panier
vital annule sa retenue en early — marge 40 % quasi seule), l'Élite le plus. Les pressions
>100 % (Bourgeois an 2, Élite an 12) viennent d'un fait DOCUMENTÉ : l'assiette M3i est le
income_gross (les pools PLEINS, avant price_level ET avant STATE_BUY_FRAC — décision M3i
« la valeur produite est la bonne assiette ») — en période de pf bas, la retenue peut
dépasser le reçu (bornée par la richesse, jamais négative). C'est LE levier (a) déjà en
place si le joueur veut alléger un ordre : INCOME_TAX_RATE_* / TAX_BASE_*.

**Découvertes** :
- **Le banc invariant M3c a perdu son dénominateur PAR LE SUCCÈS de la vague.** Détecteur :
  |autres|/échelle, échelle = Σ|composantes documentées| de l'année, plancher 1.0. E1+E2
  font fondre la création résiduelle M3b (~0 dans les petites économies saines : l'État
  COUVRE son achat) ⇒ échelle ~1 ⇒ le ratio explosait sur des dérives ABSOLUES minuscules :
  2 sims du seed 11 en « breach » à −23…−452 or/an (« 42324 % ») pendant que les MÊMES sims
  pre-m12 portaient des autres de −1884 à −9680 or/an, masqués par leur grosse échelle
  (comparaison des lignes « création résiduelle » : HEAD autres −66 à −311/an de moyenne
  sur les sims incriminées — les PLUS PROPRES du tableau). Fix : INVARIANT_SCALE_FLOOR
  (registre J, 500 = la barre SINK_FLOOR) — un plancher ne peut que RÉDUIRE un ratio,
  aucune détection historique (échelles de milliers) masquée. Résultat 0/9. Leçon : un
  détecteur RELATIF meurt quand son dénominateur tend vers 0 — re-vérifier chaque banc
  normalisé après toute vague qui améliore la grandeur qui le normalise.
- **Les références absolues d'un sweep ne voyagent PAS entre frames.** Le premier sweep
  (frame `6 12`, celui documenté par M3b/M3c) donnait pre-m12 = 1447 banqueroutes — 2× la
  référence M11 (795). La référence M11 vit sur le frame VARIABLE (`<seed> 3 250`, empires
  2→4) — retrouvé par bissection d'invocation (le frame variable reproduit 252/315/228 et
  colonisation 237 À L'UNITÉ PRÈS). Les DEUX frames concluent pareil en RELATIF
  (banqueroutes −96 %/−98 %, dette early −97 %/−100 %). Toujours ancrer le frame avant
  de comparer des absolus inter-missions.
- **La retenue M3i assise sur le gross est mécaniquement AMPLIFIÉE par E2** (pression >100 %
  possible, cf. tableau) — cohérent (« la valeur produite reste la bonne assiette »), borné
  par la richesse, mais à garder en tête si un futur calibrage veut réduire la pression d'un
  ordre : c'est le taux M3i qu'il faut toucher, pas le 0.60 (règle joueur).
- **L'hégémon/late-game n'est pas perturbé** : IPM 0.87-0.94 (vs 0.90-0.97), satisfactions
  fin de partie dans les bandes, colonisation +33 % — le circuit d'État plus riche finance
  DAVANTAGE d'expansion, pas moins.

**Pièges** :
- **`/*` DANS un commentaire de fin de ligne** : « (même cadence que FX_*/g_flux) » — le
  `*/` de `FX_*/` FERME le commentaire, `g_flux)` devient du code ⇒ erreur de compile
  sibylline (« g_flux undeclared »). Espacer : `FX_* / g_flux`.
- **Un chronicle.exe encore vivant bloque le link en silence** (motif M3b-v2.1/M5/M8,
  revécu) : le premier rebuild E3 a échoué « Permission denied » pendant que le sweep
  tournait — TOUJOURS `tasklist | grep chronicle` avant relink, et JAMAIS relink pendant
  un sweep.
- **Les runs `years=3` capturent l'an 2 exact** (snap=années/5 ne suffit pas en early) —
  motif M5 reconfirmé : mesurer l'early par des runs COURTS dédiés, pas des snapshots.
- **Le patch de mesure worktree** (BORROWDIAG copié dans pre-m12 pour la colonne PRE du
  sweep) : print-only, jamais committé, worktree détruit en fin de mission — le pattern
  légitime pour instrumenter une baseline sans réécrire l'histoire.

**Gates (tous passés)** :
1. **Kill-switch prouvé** : `STATE_BUY_FRAC=1.0,PL_GENESIS=1.0` → hash 7/108/209/310/411
   BYTE-IDENTIQUE au golden pre-m12 (8868515e/7cf6e226/cacb2485/7d457acd/70c0436e),
   re-vérifié sur le binaire FINAL avant re-baseline ✓. (PL_GENESIS=1.0 gate E1 seul ;
   STATE_BUY_FRAC=1.0 seul diverge comme attendu, E1 restant actif.)
2. **Sweep apparié** : tableau ci-dessus, chaque preuve chiffrée ✓.
3. **make test COMPLET** : 38 VERTS / 0 ROUGE / 1 BUILD ÉCHEC (intertrade_demo, setenv,
   pré-existant Windows) · credit_demo 48/48 ✓ · golden RE-BASELINÉ puis VERT ✓ ·
   determinism STABLE (5 graines × 12 ans, A==B) ✓ · determinism-deep STABLE (graines
   7/9 × 200 ans) ✓ · `scps_viewer --savetest 9` : A==B byte-identique (day=2095
   pop=60472.2 or=16811.2 identiques) + altération d'un octet REFUSÉE ✓ · `make fuzz-save`
   8/8 (216 octets flippés, aucun crash) ✓.
4. Cet append + commits granulaires FR ✓.

**Restes** :
- **La pression fiscale >100 % sur Bourgeois/Élite en early** (assiette M3i au gross ×
  marge E2) — documentée, PAS un bug (bornée par la richesse), mais un candidat si le
  joueur trouve les ordres hauts trop pressés en début de partie : baisser
  INCOME_TAX_RATE_BOURGEOIS/ELITE, jamais le 0.60.
- **Le solde de l'état de base reste légèrement négatif certaines années early** (~−4 à
  −5/mois an 5-12 au P&L) — mais SANS dette (l'écart est absorbé par la caisse qui se
  refait à la revente) ; la dette mesurée an 2/an 12 est ~0. Rien à faire, documenté.
- **Le site WILD des péages parqués (M3h/M3i item 7)** — toujours désigné (inchangé par
  M12), la vague dédiée « région-carrier → province colonisée » reste à faire.
- **UI-MONNAIE** : aucun reader façade neuf (le brief n'en demandait pas). **DLL Godot À
  RE-BUILDER** (scons -C godot) : scps_econ.c/h et scps_tune_list.h ont changé (nouveaux
  symboles econ_pldiag_*, tunables M12) — motif noté à chaque vague monétaire.
- Worktree de sweep `/c/tmp_wt_pre_m12` retiré en fin de session ; fichiers de sweep bruts
  au scratchpad (sweep_m12/), non committés — les Σ ci-dessus font foi.
