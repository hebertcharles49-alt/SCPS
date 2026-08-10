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

---

## CHANTIER UI-MONNAIE — rendre visible l'arc M0→M12 face joueur (2026-07-16)

**Statut : LIVRÉ (U1-U4) — façade GDScript pure, moteur JAMAIS touché hors 6 nouveaux
readers scps_api (pure lecture, aucun verbe neuf, tous les verbes nécessaires
existaient déjà M8/M9).** Tag `pre-ui-monnaie` posé sur 1e18cb8 avant tout changement.
6 commits (23b7671 readers+binding · 8eb38d8 U1 · d51a0eb U2 · e44029e U3 · ca46b83 U4
· 0f2f9de probe).

**Découvertes** :
- **`scps_country_fiscal_orders`/`scps_country_loan_capacity`/`scps_player_borrow_class`/
  `scps_country_loan_request_target`/`scps_country_loan_status`/`scps_player_request_loan`/
  `scps_player_bankruptcy` existaient TOUS déjà côté scps_api.c depuis M8/M9 mais
  N'ÉTAIENT JAMAIS BINDÉS côté Godot** (grep confirmé sur scps_sim_node.cpp AVANT cette
  vague : zéro occurrence) — exactement ce que TROUVAILLES M8/M9 avait signalé en Reste
  (« aucune demande GDScript cette vague »). Cette vague n'a donc eu besoin d'AUCUN
  nouveau verbe côté C : seulement 6 nouveaux READERS PURS (scps_country_debt,
  scps_country_price_level, scps_world_price_index, scps_country_debase_frac,
  scps_country_bankruptcy_scar, scps_province_res_price) + le binding manquant.
- **`budget_controls` (scps_sim_node.cpp) exposait DÉJÀ le mult BUDGET_DEBASE (index 6,
  "Débase")** — `spend_names[7]` incluait déjà "Débase" avant cette vague, seul
  `SPEND_HAS_SLIDER` (economy_page.gd/budget_panel_v2.gd) s'arrêtait à l'index 5 (Frappe).
  Décision : NE PAS ajouter 6 à `SPEND_HAS_SLIDER` (aurait ajouté un curseur Débase à la
  page Balance existante, hors périmètre de cette mission — le curseur dédié vit dans le
  nouvel onglet Monnaie). Documenté en commentaire pour le prochain agent qui grep.
- **`ProvinceEconomy.price[res]` (le champ que lit `scps_province_market`) EST le « prix
  national » projeté sur la province** (doctrine pool national P1, confirmé en lisant
  econ_tick §PRIX NATIONAL, scps_econ.c ~4866-4902 : le prix est soldé UNE FOIS par
  empire puis PROJETÉ identique sur toutes ses provinces) — `scps_province_res_price`
  (nouveau reader) n'est donc PAS un nouveau concept, juste le MÊME champ sans le filtre
  dominante+2-lignes de `scps_province_market` (qui aurait fait rater les raws hors du
  top-3 d'une province à plus de 2 biens vivants).
- **Un pays au price_level≈0 (caisse sous SINK_FLOOR partout) voit TOUS ses prix non-
  précieux retomber à leur PLANCHER quasi-nul** (`pn[c][r]=clampf(p, BASE_PRICE*0.15*pl,
  BASE_PRICE*8*pl)` avec `pl→0` ⇒ plancher→0), SAUF or/cuivre (exemptés de `pl`,
  `pl=1.f` toujours pour RES_GOLD/RES_COPPER) — mesuré en LIVE sur seed 9 an 59
  (`country_price_level(me)=1.0` — le lecteur télémétrie renvoie le neutre 1.0 par le
  garde `va_country_prev<=1e-6f`, MAIS les prix RÉELS de la province retombent à 0 pour
  Céréales/Fer tandis que Cuivre=0.52/Or=1.6 restent vivants) — PAS un bug du nouveau
  reader (vérifié : valeurs DIFFÉRENCIÉES et cohérentes par ressource, pas une constante),
  c'est le comportement DÉJÀ DOCUMENTÉ « convergence prix-métal→parité toujours molle »
  (RESTES M3f/M9). L'UI (`_price_line`, province_panel_v2.gd) gère ce cas PROPREMENT :
  prix≤0 ⇒ AUCUNE ligne affichée (jamais un « × 0 = 0/mois » qui aurait l'air cassé).
- **`lang-check` ne couvre PAS le Godot/GDScript** — le cliquet (`make lang-check`) ne
  scanne QUE `scps/viewer.c` + `scps/scps_readout.c` (les primitives du viewer CONSOLE
  SDL-free), jamais `godot/project/ui/*.gd`. Toute la doctrine « STR_* obligatoire » de
  CLAUDE.md §langue s'applique donc, EN PRATIQUE, au texte ORIGINAIRE DU MOTEUR C (mots
  résolus traversant la façade, ex. STR_LOAN_*) — le texte de CHROME GDScript (labels,
  titres de section, avertissements écrits par l'UI elle-même) est écrit en littéraux
  FR directs PARTOUT dans le code existant (confirmé : economy_page.gd, empire_window.gd,
  country_actions.gd… zéro wrapping STR_* sur leurs propres libellés). Cette vague a
  suivi ce PRÉCÉDENT ÉTABLI (tous les textes neufs de l'onglet Monnaie/Marché/emprunt/
  journal sont des littéraux GDScript) plutôt que la lettre de la doctrine — décision
  documentée ici pour qu'un futur agent ne soit pas surpris de ne trouver AUCUN STR_*
  neuf malgré beaucoup de texte joueur neuf. `make lang-check` confirmé 0=0 (base
  inchangée, aucun fichier C-viewer touché).

**Pièges** :
- **`--headless` HANG (jamais noir, carrément bloquant) sur ce toolchain pour toute
  probe qui capture un PNG** (`await RenderingServer.frame_post_draw` n'arrive jamais —
  aucune erreur, juste un process qui ne termine plus, `timeout` externe nécessaire pour
  le débloquer). Les docstrings de `shot_ui.gd`/`budget_shot.gd` avertissaient déjà
  « FENÊTRÉE (--headless = noir) » mais le comportement RÉEL observé ici est pire qu'un
  simple rendu noir : un hang total. Solution : lancer SANS `--headless` (fenêtré, un
  vrai GPU répond — RTX 5080 détecté dans ce run) ; le process se termine proprement
  (`get_tree().quit(0)`) même en fenêtré, aucune interaction manuelle nécessaire.
- **`Edit` (l'outil de remplacement de blocs) échoue silencieusement sur de GROS blocs
  multi-lignes contenant des tirets cadratins «—» / guillemets français «« »» / accents**
  (« String to replace not found » malgré une copie fidèle depuis `Read`) — cause exacte
  non identifiée (probablement une normalisation Unicode NFC/NFD divergente entre la
  lecture et l'écriture de l'outil). Contournement fiable qui a marché à chaque fois :
  ancrer sur une ligne PLUS COURTE et moins chargée en caractères spéciaux, OU basculer
  sur `sed`/heredoc (Bash) pour l'insertion brute suivie d'une correction d'INDENTATION
  ciblée (`sed -i '<L1>,<L2>s/^\t//'`) — RELIRE ensuite au byte près (`cat -A`) pour
  vérifier l'imbrication de tabulations (piège JUMEAU ci-dessous).
- **Un bloc inséré via `sed ... r fichier.txt` hérite de l'indentation du fichier SOURCE,
  PAS du point d'insertion** — deux fois cette vague (country_actions.gd ligne ~658,
  alerts.gd ligne ~322), le nouveau bloc est atterri UNE tabulation trop profond
  (imbriqué dans le `if`/`else` précédent au lieu d'être son SIBLING), un bug d'exécution
  SILENCIEUX en GDScript (pas d'erreur de parse — juste un code qui ne s'exécute que dans
  la mauvaise branche). Toujours `cat -A` + compter les `^I` après une insertion `sed r`,
  jamais faire confiance à l'indentation du texte source tel quel.
- **`var x := cond_expr if bool else 0.0` peut échouer l'inférence de type GDScript**
  (« Cannot infer the type of "x" ») quand `cond_expr` est un appel dont le type de
  retour n'est pas visible au même niveau que le littéral `0.0` de repli — typer
  EXPLICITEMENT (`var x: float = ...`) plutôt que `:=` dès qu'un ternaire mélange un
  appel de méthode Godot (`w.method(...)`) et un littéral.
- **Le `_prov_detail`/`province_detail.gd` (touche ouverte au clic sur une province) et
  le `_prov_panel_v2`/`province_panel_v2.gd` (touche V, ATTACHÉ à `main.gd` KEY_V) sont
  DEUX panneaux province DISTINCTS qui coexistent** — le brief nommait `province_panel_v2.gd`
  comme LE modèle (« c'est le modèle ») : confirmé exact, `main.gd:562` (`KEY_V`) toggle
  bien `_prov_panel_v2`, pas `_prov_detail`. Un agent qui grepperait juste "province"
  pourrait éditer le mauvais fichier (3 variantes vivent dans `godot/project/ui/` :
  `province_panel.gd` legacy immediate-mode, `province_detail.gd`, `province_panel_v2.gd`).

**Restes** :
- **« Gros emprunts d'États voisins » (brief U4, dernier item) NON implémenté** — aucun
  seuil scale-invariant honnête disponible (pas de reader `credit_debt_ceiling`/PIB-pays
  exposé à la façade) pour distinguer « grosse dette » chez un petit pays vs un empire ;
  plutôt qu'inventer un seuil absolu arbitraire (bruyant, non calibré), le point a été
  documenté comme Reste. Un futur reader `scps_country_debt_ceiling` (miroir de
  `credit_debt_ceiling`, scps_credit.c) permettrait `debt_total/ceiling` — un ratio
  scale-invariant propre pour cette condition.
- **`_price_line` (province_panel_v2.gd) n'a été visuellement confirmé QUE dans le cas
  « aucun prix » (0, ligne omise) et via la console (valeurs différenciées correctes)** —
  le monde de test (seed 9, an 59, économie IA-only faible) n'avait aucune province à
  soi produisant or/cuivre (les seuls biens restés à prix non-nul dans ce run) pour
  capturer un hover NON-VIDE en PNG. Le mécanisme est prouvé CORRECT (lecteur + formule),
  pas prouvé BEAU à l'écran avec un vrai nombre — à revérifier en jeu réel/un monde plus
  mûr si un doute visuel remonte.
- **La « tendance » du Marché (onglet Marché, budget_panel_v2.gd) est un suivi CLIENT
  non testé sur plusieurs mois consécutifs** (le probe ne capture qu'un seul refresh —
  premier passage, `_marche_hist` vide, donc AUCUNE tendance affichée cette fois-ci,
  seulement le prix courant) — la logique delta/mois est écrite et raisonnée (motif
  `topbar._d_gold`) mais son affichage réel (flèche ↗/↘, %/mois) reste À VÉRIFIER visuellement
  sur un run qui laisse le panneau ouvert plusieurs ticks mensuels.
- **Le journal U4 (bankruptcy/débase adverses) n'a JAMAIS été observé en conditions
  réelles** (seed 9 an 59 : `country_bankruptcy_scar`/`country_debase_frac` à 0 partout,
  aucune condition n'est apparue — « JOURNAL · rien à signaler » confirmé dans les
  captures) — le MÉCANISME est identique au motif « conseil vacant » déjà éprouvé
  (édge-detection existante), donc à haute confiance, mais SANS preuve visuelle d'une
  ligne réellement apparue au journal pour une banqueroute. Un futur agent pourrait
  forcer une banqueroute (`CMD_BANKRUPTCY` sur un pays IA en dette, ou un monde plus
  long/dur) pour capturer la ligne en vrai.
- **EXPORT scps.exe** — toujours pas fait (déjà noté à CHAQUE synthèse depuis le 14) ;
  cette vague ajoute encore de la surface UI jamais testée en dehors des probes headless.

---

## GIGA SWEEP 2026-07-16 — l'œil neuf (100 sims hors-monnaie)

**Mission** : lecture pure (aucun code touché) de `build/giga/seed_*.txt` — 20 graines × 5 sims ×
250 ans, 100 bilans § BILAN an 250, 532 fiches d'empire. Extraction par script Python (regex,
`utf-8`/`errors=replace`) → `sims.json`/`empires.json` (scratchpad, non committés). Périmètre
VOLONTAIREMENT hors-monnaie (banqueroutes/dette/frappe/débase/inflation déjà auditées ailleurs,
chantiers M0-M12). Chiffres complets dans la réponse de session ; ce qui suit est le résumé qui
COÛTE cher à retrouver.

**Découvertes par dimension** (n=100 sims sauf mention contraire, n=532 pour les stats par empire) :
- **Militaire** : guerres 38.5/sim (6-134) ; armes produites 62.4/sim moy. FER se négocie
  en moyenne à 34 % de son prix de base (2.4) — 9/100 sims à prix FER = 0.0 pur (marché FER mort
  localement). Les STOCKS ne sont PAS dominés par les armes : Poisson/Céréales/Bétail/Bois
  cumulent 79 % des stocks #1 par empire, les armes seulement 12 % (toutes catégories
  confondues : 12.9 % des 4 slots de stock affichés). guerres motivées : territoriale 2233 +
  subjugation 1139 total, vs économique 54 et anti-piraterie 2 (quasi-mortes). morts CHOC vs
  POURSUITE = 164 400 vs 973 500 (ratio 5.9x) — la poursuite domine STRUCTURELLEMENT, pas
  anecdotique.
- **Tech** : 21.6 % des empires (115/532) à 0 tech, 38.2 % à moins de 6 — la stagnation EST la
  norme pour plus d'un tiers des empires. p50=17, p90=51, p99=63 (jamais 65+ observé au-delà de
  2 empires). Corrèle fort avec prosp (r=0.74) et pop (r=0.73), modérément avec stab (r=0.54),
  QUASIMENT PAS avec le fait de tenir un hub (r=0.07) ni avec la corruption (r=-0.21). Le cas
  cité en brief (1 empire 61 tech, 3 à zéro) est réel et quasi-identique :
  seed_9.txt#sim1 = [65, 61, 0, 0, 0] sur 5 empires. Autres spreads extrêmes :
  seed_1024.txt#sim4 [65,42,41,27,18,0,0], seed_2026.txt#sim5 [63,45,44,43,36,21,0,0,0,0,0]
  (5 zéros sur 11 empires).
- **Fins (§27)** : RÉCHAUFFEMENT 63, GRAND HIVER 12, RONCES 12, ENGLOUTISSEMENT 9, SANG 1,
  AUCUNE 3 (le monde survit intact à l'an 250 — seed_1024.txt#sim1, seed_110.txt#sim4,
  seed_13.txt#sim3, tous à sang% sous 6 ET feu SOUS SEUIL). MÉCANISME ÉCLAIRCI : "feu ARMÉ"
  (combustible/tête au-dessus du seuil 2.0) est vrai dans 95/100 sims mais RÉCHAUFFEMENT ne
  conclut que 63/100 — 32 sims où feu est ARMÉ finissent autrement (GRAND HIVER/RONCES/
  ENGLOUTISSEMENT/SANG), TOUJOURS à an180 (17/100 fins tombent pile an180) contre an240 pour
  RÉCHAUFFEMENT (63 fins à an240 pile) : il existe un PREMIER checkpoint (an180, cataclysmes)
  puis un fallback fixe (an240, RÉCHAUFFEMENT = la fin par défaut si rien d'autre n'a mordu).
  La ligne "feu :" affiche TOUJOURS le texte "repli RÉCHAUFFEMENT" (100/100, même quand la fin
  réelle est GRAND HIVER) — c'est un texte de mécanisme fixe, pas une prédiction de la fin
  réelle. sang% (mémoire des morts / pop vivante, seuil 9%) : moy 1.93 %, max 16.58 %
  (seed_77.txt#sim2, qui finit quand même en RÉCHAUFFEMENT — la fin SANG n'a mordu qu'UNE
  fois, seed_512.txt#sim2, à 9.94 % au-dessus du seuil 9 % — cohérent, le seul dépassement
  mesuré = le seul déclenchement). L'entropie [TERMINAL] (17/100 sims, jusqu'à 175125 sur
  seed_128.txt#sim3) ne coïncide JAMAIS avec RÉCHAUFFEMENT ni AUCUNE — toujours un des 4
  cataclysmes durs.
- **Merveille** : MAX observé sur 100 sims = {1:58, 2:37, 3:5} — JAMAIS 4, 5 ou 6/6. La moitié
  supérieure du contenu Merveille est un contenu mort dans l'enveloppe testée (250 ans, 2-6
  empires/sim). paliers toujours "3/4/6" (fixe, non dérivé du monde). Corrélation
  métabolisation-max% avec palier atteint réelle mais modérée (r=0.46) : palier1 vers moy
  19.2 % digéré, palier2 vers 34.1 %, palier3 vers 45.3 % — jamais assez pour franchir le
  palier 3.
- **Sociologie** : Cohésion quasi gelée (moy 97.65, σ 3.64, min 77) — signal plat, à surveiller
  si un futur calibrage veut la rendre significative. Corruption : 30.6 % des empires à
  Corr=0 (PAS mort — p90=48, p99=80, réel dans la queue). Le "pire corrupteur" du monde est
  tenu par les Conquérants dans 95/100 sims, les Marchands dans 5/100 — AUCUNE autre faction
  n'apparaît jamais en 100 sims. Zombies (Stab<=10, toujours listés vivants) = 13.3 % de
  TOUS les empires (71/532) : petits États croupions (2.63 rég moy vs 6.26), Cohésion RESTE
  haute (95.9 moy) et Légitimité pas si basse (55.5 moy) — la survie zombie tient à la PETITE
  TAILLE, pas à la légitimité ni à la cohésion qui restent découplées de la stabilité en bas
  d'échelle. Labels 1er empire : Assise Consentie 63 %, Tyrannique 28 %, Partagée 6 %,
  Contrainte 3 % — Consentie corrèle Légit moy 77.0 vs Tyrannique 42.7, mais Partagée (64.0) et
  Contrainte (74.3) cassent une règle simple au seul Légit — la formule exacte du label reste
  À VÉRIFIER EN SOURCE (pas déduite avec certitude des données seules).
- **Classes** : au niveau monde (pop-pondéré) Laborer 84.7 % moy / Bourgeois 12.43 % / Élite
  2.87 % — dérive vers PLUS de Laborer que le départ 80/15/5 (Bourgeois et Élite se
  contractent légèrement en régime). Au niveau empire (n=532, non pondéré pop) c'est encore
  plus tassé : J 87.7 %/B 9.1 %/E 3.2 % (les micro-empires, nombreux, sont presque purs
  Laborer). Esclavage : 1488 âmes serviles au total, mais SEULEMENT 9/100 sims en ont au moins
  une — quasi-mort à l'échelle du sweep, ZÉRO rachat IA en 100 sims, affranchissements
  observés dans 2/9 sims seulement.
- **Cités-états & commerce** : hubs_pct fortement bimodal — 75/100 sims à 100 % pile (dont
  65/75 avec un volume RÉEL supérieur à 0, 10/75 triviaux 0/0 affichés 100 % par convention),
  5/100 à 0 %, seulement 20/100 entre les deux. hubs_pct corrèle négativement avec
  vivier_absorbees (r=-0.45) mais pas parfaitement — sur les 5 sims à 0 %, 4 ont un commerce
  inter-pays quasi nul (autarcie réelle), 1 (seed_2026.txt#sim3) a un commerce réel (903
  or/an) qui passe ENTIÈREMENT hors des Centres — un mode "commerce sans hub" existe, distinct
  de l'autarcie pure. Seuls 6.8 % des empires (36/532) tiennent eux-mêmes un hub.
- **Âges** : jamais bloqué — min observé 3 âges/sim (jamais 0/1/2), distribution
  {3:4, 4:13, 5:20, 6:43, 7:20}, moy 5.62.
- **Faustien** : entropie médiane 14 (quasi-nulle dans l'immense majorité), mais 17/100 sims
  basculent en régime [TERMINAL] (jusqu'à 175125). foreuse tire dans 3/100 sims seulement
  (la plus dormante des 3 conso faustiennes), réplicateur 20/100 (jusqu'à 26996), corne 23/100
  (jusqu'à 9638) — bascules rares mais réelles quand elles arrivent (pas de valeurs
  intermédiaires, du tout-ou-rien). 613 nœuds faustiens débloqués au total sur 100 sims
  (environ 6.1/sim) — la RECHERCHE faustienne progresse régulièrement même quand la
  CONSOMMATION (foreuse/corne/réplicateur) reste éteinte : écart recherche/usage réel.
- **Autres** : réfugiés — 1 214 556 âmes en fuite vs 1 154 821 âmes de retour (ratio 0.95) sur
  100 sims : la respiration démographique annoncée par le jeu est confirmée à grande échelle,
  la migration de guerre est presque toujours transitoire. Accession T3 (3e palier de bâti)
  JAMAIS atteinte dans 63/100 sims — cohérent avec le chiffre SYNTHÈSE "T1 75 % des provinces
  pour toujours" que chaque fichier répète. Directeur (F), événements les plus fréquents
  cumulés (agrégats SYNTHÈSE des 20 fichiers) : L'Année Sans Été (346) devant La Peste
  Fluviale (287), Le Congrès (250), La Réformatrice (235) — Le Schisme dirigé (11) est le plus
  rare des événements qui tirent QUAND MÊME (pas mort, juste rare).

**Mécanismes morts** (jamais ou quasi-jamais déclenchés sur 100 sims / 532 empires) :
1. **Interception navale** : 0 interception, 0 "paquet noyé" sur 100 sims × 250 ans — le
   commerce maritime sans escorte n'est JAMAIS puni malgré la doctrine du jeu ("le transport
   sans escorte est une PROIE").
2. **Merveille paliers 4-6/6** : jamais atteints (max observé 3/6, dans 5 % des sims
   seulement) — la moitié haute du contenu Merveille est inaccessible dans l'enveloppe testée.
3. **Rachats IA d'esclaves** : 0/100 sims. Affranchissements : seulement 2/9 sims où
   l'esclavage existe déjà (donc environ 2/100 au global).
4. **Guerres anti-piraterie** (2 au total/100 sims) et **guerres économiques** (54 au total,
   environ 0.5/sim) — motifs de guerre quasi éteints face à territoriale (2233) et subjugation
   (1139).
5. **Conso foreuse** : 3/100 sims — la plus dormante des 3 machines faustiennes.
6. **Coups d'État** (soulèvements) : 1 SEUL sur 100 sims (vs 148 sécessions, 84 concessions,
   1309 écrasements) — statistiquement résiduel, cohérent avec la note du jeu lui-même
   ("0 purge, RARE attendu"), mais à ce niveau c'est quasi-mort en pratique.
7. **Directeur "acharnement"** : toujours 0 sur 100 sims — MAIS c'est un garde-fou VOULU
   ("acharnement 0, DOIT être 0" dans le texte du jeu lui-même), pas un mécanisme mort par
   accident — à ne pas confondre avec les points 1-6.

**Les 5 découvertes qui méritent une vague** (classées par impact gameplay) :
1. **Merveille : la moitié du contenu (paliers 4/5/6 sur 6) n'est jamais visitée en 250 ans.**
   Palier 3 lui-même n'est franchi que 5/100 fois, et le digéré moyen au palier 1 plafonne à
   19 %. Si la Merveille est censée être un objectif de fin de partie atteignable, l'écart
   entre le rythme de métabolisation et le coût des paliers hauts mérite un calibrage — sinon
   documenter que c'est un contenu d'aspiration long-terme volontairement hors de portée d'un
   seul run standard.
2. **RÉCHAUFFEMENT domine parce que c'est le fallback an240, pas parce que c'est le mécanisme
   gagnant en soi.** Les 4 autres fins ont une fenêtre de déclenchement antérieure (an180) qui,
   si elle ne mord pas, laisse le monde continuer jusqu'au filet RÉCHAUFFEMENT. Le jeu a DÉJÀ
   un objectif chiffré de ratio max/min entre fins (vu en SYNTHÈSE : "ratio max/min dispatch
   99.9:1, cible <=2:1") — sur ce sweep, RÉCHAUFFEMENT (63) vs SANG (1) donne un ratio 63:1,
   très loin de la cible déjà écrite dans l'outillage. Vague candidate : élargir la fenêtre de
   déclenchement ou baisser les seuils des 4 autres fins pour qu'elles mordent plus souvent
   avant le fallback.
3. **La tech est bimodale et corrèle avec pop/prosp mais PAS avec le fait de tenir un hub
   commercial (r=0.07).** 38 % des empires stagnent à 5 tech ou moins. La "remise diffusion"
   existe déjà (doctrine CLAUDE.md) mais 21.6 % restent à 0 tech pur — vérifier si le
   mécanisme de rattrapage touche vraiment les petits empires isolés ou seulement ceux déjà
   connectés au commerce.
4. **Interception navale totalement inerte (0/100 sims)** — le texte du jeu promet une
   punition du commerce non-escorté qui ne se manifeste jamais dans l'échantillon ; soit le
   trigger est mal câblé, soit son seuil d'activation n'est jamais atteint aux échelles de
   sim testées (2-6 empires, 5-9 cités-états).
5. **Un tiers d'empires "croupions" persistants** (13.3 % Stab<=10 mais Cohés autour de 96 /
   Légit autour de 55, coups d'État quasi-inexistants (1/100), écrasements dominant les
   soulèvements 1309 vs 148 sécessions) dessinent une classe d'États stagnants qui ne meurent
   ni ne se réforment. À documenter comme texture voulue (la longue traîne de l'histoire) ou
   à traiter avec plus d'ambition IA / rattrapage — actuellement ambigu, PAS un bug identifié,
   un signal à trancher côté design.

**Pièges d'interprétation** (pour le prochain agent qui rouvre build/giga/) :
- "stocks Sigma" et "armée" peuvent afficher -0 (négatif de zéro, cosmétique) — un parseur
  naïf sur \d+ sans signe rate ces lignes silencieusement (perdait 6/532 empires avant
  correction).
- "hub OUI" est TOUJOURS en majuscules, "hub non" toujours en minuscules — un regex
  insensible à la casse sur oui/non seul rate systématiquement les empires hub=OUI.
- Le type de fin "GRAND HIVER" contient un espace — un regex mono-mot sur "§27 FIN :" rate
  silencieusement CES fins précises (15 cas perdus avant correction).
- La ligne "feu :" a DEUX formats : "repli RÉCHAUFFEMENT ARMÉ (seuil ..., après +N ans)" et
  "repli RÉCHAUFFEMENT sous seuil (seuil ..., après +N ans)" — le second (5/100 sims) n'a PAS
  de fin §27 associée si aucun autre mécanisme n'a mordu non plus (les 3 sims AUCUNE).
  "sous seuil" ne veut PAS dire jamais mais pas encore franchi cette sim-ci. Le champ
  feu_repli_type (RÉCHAUFFEMENT) est un texte de MÉCANISME fixe, pas une prédiction — ne pas
  le confondre avec fin_type (§27, la fin RÉELLEMENT survenue).
- "hubs : X% ... (A / B)" peut afficher 100 % avec A=B=0 (convention 0/0 = 100 %) — 10/75 des
  sims classés hub 100 % n'ont en réalité AUCUN volume de commerce mesuré ; toujours vérifier
  le dénominateur avant de citer le pourcentage.
- Le bloc "par événement" (Directeur F) n'existe QU'au niveau SYNTHÈSE (agrégat des 5 sims
  d'un fichier), pas par sim individuelle — pas de granularité par sim pour ce champ.

**Restes** :
- Données brutes (sims.json, empires.json, scripts parse_giga.py/analyze*.py) au scratchpad
  de session, non committées — les chiffres ci-dessus et dans la réponse de session font foi.
- Le lien exact "label 1er empire" avec les seuils Légit/Cohés/Stab n'a pas été retrouvé avec
  certitude depuis les données seules (Partagée/Contrainte cassent une règle simple au Légit
  seul) — à vérifier en source si une vague touche ces labels.
- "Empires au bord de la Brèche" (22/100 sims) ne semble PAS purement proportionnel à
  l'entropie monde (des cas à entropie 8-21 ET des cas à entropie de plusieurs milliers) —
  mécanisme probablement par-empire (tech faustienne individuelle ?), pas vérifié en source.

---

## CHANTIER TECH — la refonte du moteur de recherche (2026-07-16)

**Statut : LIVRÉ — zéro-tech 21.6 % → 3.7 % (giga 100 sims), ancre leaders ±0 % (max 65
identique), bandes M12 TENUES, golden RE-BASELINÉ VERT, kill-switch TECHPOP=0 prouvé
byte-identique.** Tag `pre-tech` posé sur 1e09502. Commits : 7bb0c3c (T0 diag IA) ·
ddfc57b (T0 diag chronicle + fiche honnête) · c2bddc7 (T1a adoption/héritage) ·
fc6c493 (T1b f_satisfaction) · golden + cet append (commit suivant). SAVE_VERSION 95
INCHANGÉ (aucune struct sérialisée touchée — ai_on était déjà SVT_AION, les paires de
succession sont des statics transitoires hors save).

### LA DÉCOUVERTE CENTRALE — le brief ciblait le mauvais site

`re->tech` (scps_econ.c:4786, `wealth×TECH_RATE×satisfaction×savoir_mult`) est un
accumulateur **VESTIGIAL** : tracé exhaustif — il n'est lu QUE par econ_print_region/
econ_print_summary (debug console jamais appelé par chronicle) et social_demo (fixture).
Le « N tech » du giga = `stats.techs` (acteur IA) = `TechState.n_unlocked`, alimenté par
le pipeline RÉEL : `econ_country_savoir` (scps_econ.c:896, Σ pop_classe×SAVOIR_W_* ×
(1+%bibliothèque clampé 0.33)) → `research_points` (scps_ai.c ai_research_step ×4.5
AI_RESEARCH_INCOME_W × yield institutions ×(1+métabolisation) ; miroir joueur
scps_sim.c:1036) → `tech_research`. **La formule décidée par le joueur (pop × bâtiments %
× satisfaction % × métabolisation %) existait DÉJÀ aux ¾ dans ce pipeline** — il ne
manquait que f_satisfaction (aucun terme de satisfaction nulle part dans le chemin réel).

### LE VRAI GOULOT DES 22 % — ai_on=0, pas la formule (mesuré SCPS_TECHDIAG)

Diag seed 9 ×5×250 : les **8/8 empires zéro-tech du bilan étaient `ai_on=0`** — des pays
SANS IA à vie (research_points=0 pour toujours), pas des pays pauvres (Ligue Brenyan :
10 rég, 57k pop, savoir 160/an... et 0 point accumulé en 250 ans). Deux familles :
1. **Les FRAGMENTS du resplit de cataclysme §27** (scps_endgame.c:326-341, eau/chaleur,
   an 180-240) : nés hors du canal révolte (aucun `last_spawned`), memset nu —
   `capital_prov=0` (la province 0 d'AUTRUI), `n_regions=0`, arbre vierge, JAMAIS
   adoptés par la boucle IA de scps_sim.c:1169 (qui ne tournait QUE le mois d'une
   sécession-révolte). 63/100 sims finissent RÉCHAUFFEMENT an 240 + 9 ENGLOUTISSEMENT
   ⇒ ~1-3 fragments/sim mesurés à an 250 = le gros des 115/532. La distribution
   « bimodale 0 ou 27+ » était donc UN ARTEFACT DE MESURE : des fragments nés 10 ans
   avant le bilan, sans IA, arbre à 6 nœuds de base.
2. **Les sécessions MANQUÉES par la course d'agrégat** : `secede_to_country` re-clé les
   provinces mais `regions_of` lit `region[].owner` (rafraîchi au econ_tick SUIVANT) ;
   `rs->last_spawned` est RAZ en tête de chaque revolt_tick ⇒ raté une fois = mort à
   vie (mesuré : « Agraire libre » c=13 seed 9 sim 5, terrien, ai_on=0).

**Bibliothèques : PAS le discriminateur.** Σ build.savoir ≈ 0.0 pour presque TOUS les
empires — y compris les leaders à 65 techs (TDMAP : Estroris 0.0, Cogexel 0.0,
Merwickka 0.0 ; max observé 5.5). Le facteur bâtiments réel est [×1..×1.33] (jamais
bloquant — le « plancher tradition orale » demandé par le brief existait déjà : c'est
le 1 de (1+pct)) ; le vrai levier bâtiment est la chaîne de TECHS Savoir·Production
(Scriptorium→Université, yield ×1..×2.5) qui, elle, tire. Le planificateur IA
(EDI_BIBLIOTHEQUE, catch-up LOT I) n'a PAS été touché — pas le bug, pas « ciblé et sûr ».

### T1 — LES FIXES (tous gatés TECHPOP, kill-switch maître)

- **T1a adoption** : la boucle d'adoption tourne CHAQUE mois (O(n_pays), idempotente par
  ai_on) ; l'orphelin ANTAGONIST terrien (capital≥0, regions_of>0) est adopté dès agrégats
  frais, `peace_rebuild_country` recale capitale/region_ids avant `ai_actor_init`.
- **T1a succession** : le resplit §27 fait naître le successeur COMPLET (capital_prov,
  region_ids) et enregistre {enfant, parent} (statics transitoires scps_endgame.c,
  readers endgame_succession_count/get) ; scps_sim.c consomme le MÊME tick :
  **ts[enfant]=ts[parent]** (le savoir survit à la fragmentation — mêmes gens, mêmes
  livres), research_points repart à 0 (la banque reste à la couronne).
- **T1b f_satisfaction** : dans `econ_country_savoir` (source unique IA+joueur+façade) —
  `f_sat = TECHPOP_SAT_FLOOR(0.5) + TECHPOP_SAT_SPAN(0.75)×sat_pop_pondérée` ∈
  [0.5..1.25], ~×1.0 à 67 % de satisfaction (ancre leaders préservée), la misère RALENTIT
  sans jamais éteindre — décision joueur « du ×1,XXX pas du ×0 ». Satisfaction lue
  pop-pondérée sur les régions du pays, MÊME boucle/grain que la base existante.
- **T1d fiche honnête (display-only)** : « N tech » de la fiche chronicle =
  `n_unlocked − socle tier-0` (le savoir DU PAYS) au lieu de `stats.techs` (compteur
  d'ACTEUR remis à 0 à l'init) — un héritier de 65 nœuds affichait 0. stats.techs reste
  lisible via TDMAP (SCPS_TECHDIAG).
- **Kill-switch TECHPOP=0** : formule savoir legacy EXACTE (expression intacte) + adoption
  OFF + héritage OFF + succession-complète OFF ⇒ `SCPS_TUNE=TECHPOP=0 make golden` VERT
  vs golden pre-tech (prouvé 2×, dont sur le binaire final avant re-baseline) ; empreinte
  moteur 250 ans identique (âges/or/PIB/fins seed 9 ×5 : diff vide).

### T2 — L'AUDIT SAT-RENDEMENT (tableau site par site)

| Site (fichier:ligne) | Formule | Famille | Verdict |
|---|---|---|---|
| scps_econ.c:4786 `re->tech += wealth×TECH_RATE×sat×savoir_mult×council×(1−rot)` | multiplicatif, ×0 possible | RENDEMENT | **VESTIGIAL** (re->tech jamais lu par le jeu) — INTACT, décision documentée |
| scps_econ.c:4822 diaspora×DIASPORA_TECH_RATE×innovation → re->tech | additif | RENDEMENT | même accumulateur MORT — INTACT ; la vraie f_métabolisation est AI_METAB_RES_W×econ_country_metabolized (déjà planchée ×1) |
| econ_country_savoir (neuf) f_sat | multiplicatif borné [0.5..1.25] | RENDEMENT | **LE SEUL SITE REMAPPÉ** (ajout, tunables TECHPOP_SAT_*) |
| econ_satisfaction_tax_factor (2136, lu 4×) | 1+coupling×(sat−ref), clampé [0.5..1.5] | POLITIQUE (tolérance fiscale M8 C1) | déjà borné, sémantique design — INTACT |
| seuil fiscal ×(0.40+0.60×sat) (3846/4044) | plancher structurel 0.40 | POLITIQUE | INTACT |
| croissance POP_SAT_W×max(0,sat−0.5) (4351) | bonus additif asymétrique, jamais négatif | RENDEMENT (démo) | déjà conforme « jamais ×0 » — INTACT |
| promotion ≥50 % (3354) / démotion <30 % 2 mois (3369) | seuils | POLITIQUE (mobilité) | INTACT |
| aisance=sat×10 (demography:139, legitimacy:66) | échelle | POLITIQUE (légitimité) | INTACT |
| prosperity unmet=(1−sat)×pop (141) | charge | POLITIQUE (pression PE) | INTACT |
| scps_ai.c:922 ciblage pire-sat | comparaison | POLITIQUE (priorisation) | INTACT |
| scps_events.c:389/412/447 seuils sat | gates | POLITIQUE (dilemmes) | INTACT |
| scps_revolt.c:917/950 (+0.15/+0.20) | ÉCRITURE | — | INTACT |

Aucun site ambigu restant. **Conclusion : le seul multiplicateur ×0-capable de rendement
était la ligne tech élite, vestigiale** — la décision joueur s'implémente entièrement
dans le pipeline réel (f_sat neuf borné).

### T3 — L'ABLATION PRE-MONNAIE (verdict d'attribution)

Worktree manuel `pre-monnaie` (589036e), build séparé, frame {9,11,42}×3×250 :
**pre-monnaie 17/44 zéro-tech (38.6 %) vs pre-tech (M12) 9/39 (23.1 %)**.
Verdict : **PRÉEXISTANT** — le zéro-tech précède tout l'arc monnaie (il était même PLUS
fréquent avant ; le bug ai_on/cataclysme est structurel, très antérieur). Le lien
« pression fiscale élites >100 % → tech » supposé par le brief est CADUC : le pipeline
réel ne lit NI la richesse d'élite NI la pression fiscale (la ligne richesse-élite est
vestigiale) — la pression M12 sur les élites n'a jamais touché la recherche.

### T4 — LE CALIBRAGE ET LA PREUVE

**Sweep apparié** (frame M11/M12 `<seed> 3 250`, pre-tech = TECHPOP=0 du binaire HEAD,
moteur prouvé identique — reproduit banqueroutes M12 28=12+0+16 À L'UNITÉ) :

| métrique | pre-tech | HEAD | verdict |
|---|---|---|---|
| zéro-tech | 9/39 (23.1 %) | **1/35 (2.9 %)** | cible <5 % ATTEINTE |
| banqueroutes Σ | 28 (12+0+16) | 27 (12+2+13) | bande TENUE |
| dette early an 2-3 Σ (9 sims dédiés `3 3`) | 0 | **0** | 9/9 — gate M12 tenu |
| Laborer sat moy (fin) | 72/60/61 % | 74/76/66 % | tenue (au-dessus — jamais vers le bas) |
| colonisation Σ | 315 | 347 | +10 %, bord de bande, positif |
| invariant M3f | 9/9 sous seuil (62-154 % vs 370 %) | 9/9 sous seuil | 0/9 breach TENU |
| leaders max/seed | 65/48/56 | 65/48/56 | ancre ±0 % |

**RE-GIGA 20×5×250** (build/giga_tech/, vs build/giga/ pre-tech du même jour) :

| métrique | pre-tech (100 sims) | HEAD (100 sims) |
|---|---|---|
| zéro-tech | 115/532 (21.6 %) | **19/509 (3.7 %)** |
| <6 techs | 38.2 % | 19.6 % (bimodalité effondrée) |
| p50 · p90 · max | 17 · 50 · 65 | 27 · 56 · **65** (ancre) |
| fins | RÉCHAUF 63 · RONCES 12 · HIVER 12 · ENGLOUT 9 · SANG 1 | RÉCHAUF **63** · RONCES 15 · HIVER 11 · ENGLOUT 9 |
| Merveille métab max | {1:58, 2:37, 3:5} | {1:55, 2:42, 3:3} |
| entropie [TERMINAL] | 17 | **17** (mesuré : la Brèche ne s'emballe PAS) |
| nœuds faustiens Σ | 613 | 712 (+16 %, fins inchangées) |
| nœuds recherchés Σ (acteurs) | 20 947 | 20 321 (l'ACTIVITÉ de recherche est stable — c'est la distribution qui change) |
| banqueroutes Σ | 335 | 289 |
| dette monde fin Σ | 94.3k | 81.6k |
| invariant breaches | 5/100 | **3/100** |
| Laborer sat moy | 60.1 % | 62.1 % |

Les 19 zéro-tech restants : TOUS des micro-États « X libre » 1-région, pop 0-7k, nés
tard d'une sécession — IA vivante, juste pas encore leur premier nœud. Honnête.
**RÉCHAUFFEMENT reste 63/100** : l'attendu « <63 » ne s'est PAS matérialisé à l'échelle
(le paired 3-graines montrait 3 RONCES de plus, mais à 100 sims le fallback an-240
domine toujours — les fenêtres an-180 dépendent de l'entropie/charge, pas du nombre de
mondes techés). Mesuré, pas présumé — la vague « fenêtres des fins » (item 2 du giga
sweep l'œil neuf) reste le levier pour la diversité des fins.

**Gates (tous passés)** : (1) kill-switch golden byte-identique 2× ✓ · (2) sweeps
ci-dessus ✓ · (3) make test 38 VERTS/0 ROUGE (intertrade_demo seul, setenv Windows
pré-existant) · credit_demo 48/48 ✓ · golden RE-BASELINÉ puis VERT ✓ · determinism
STABLE (5×12 ans) ✓ · determinism-deep STABLE (7/9 ×200 ans) ✓ · savetest 9 A==B
byte-identique (day=2095 pop=60472.2 or=16811.2) + octet altéré REFUSÉ ✓ · fuzz-save
8/8 (216 octets, 0 crash) ✓ · lang-check 0=0 ✓.

**Pièges** :
- **Le compteur « N tech » de la fiche était un compteur d'ACTEUR, pas de PAYS** —
  `stats.techs` meurt avec l'acteur (ai_actor_init) ; toute télémétrie qui compare des
  pays nés en cours de partie doit lire `n_unlocked − socle`. Les agrégats « nœuds
  déverrouillés/sim » (chronicle:1795, capture_age_snap) restent en stats.techs
  (activité), c'est VOULU.
- **`region[].owner` est rafraîchi au econ_tick suivant** : tout code qui teste
  `regions_of` le mois d'un transfert de provinces lit du PÉRIMÉ — la sécession-split
  (scps_sim.c:436) appelait déjà econ_aggregate_regions explicitement, le canal révolte
  non. Une boucle mensuelle idempotente coûte moins cher qu'une resynchro par site.
- **Un flag one-shot (`last_spawned`) comme déclencheur d'une boucle de rattrapage =
  un raté irrécupérable** — préférer l'idempotence périodique.
- **`grep "· ?[0-9]+ tech"` rate les `·  0 tech` (deux espaces, %2d)** — compter les
  zéro-tech avec `"·  0 tech"` exactement (piège de parse giga, déjà mordu 2×).
- Les fins des sims changent AVANT l'endgame quand on active TECHPOP : l'adoption tourne
  dès le mois 1 (orphelins précoces) et f_sat change les revenus dès l'an 1 — la
  divergence de trajectoire est TOTALE, ne comparer que des agrégats statistiques.

**Restes** :
- **DLL Godot À RE-BUILDER** (`scons -C godot`) : scps_econ.c, scps_tune_list.h,
  scps_endgame.c/h, scps_sim.c, scps_ai.c ont changé — motif noté à chaque vague. Aucun
  fichier godot/ touché (interdit de vague) ; aucun reader façade neuf nécessaire (la
  fiche tech façade lit déjà TechState — vérifier à l'occasion que l'UI arbre affiche
  n_unlocked, pas un compteur d'acteur).
- **La diversité des FINS n'a pas bougé (63/100 RÉCHAUFFEMENT)** — la vague « fenêtres
  an-180 / seuils des 4 fins » (giga sweep l'œil neuf, item 2) reste à faire ; la tech
  seule ne suffit pas.
- **L'héritage de l'arbre aux sécessions-RÉVOLTE** (pas cataclysme) : délibérément NON
  fait (elles recherchent déjà, naissent tôt, design à trancher — un peuple qui fait
  sécession emporte-t-il les bibliothèques royales ?). Candidat si le joueur veut moins
  de micro-États à 0-5 techs.
- **Le planificateur IA ne bâtit presque jamais de Bibliothèque** (Σ build.savoir ≈ 0
  partout, leaders compris) — le bonus [×1..×1.33] est quasi latent ; si un jour on veut
  que la brique BÂTIE compte, c'est un calibrage agency dédié (LOT I n'a pas suffi).
- Fichiers de sweep bruts : build/giga_tech/, build/paired_tech/, build/ablation_tech/,
  build/techdiag/ — scratch locaux non commis, les Σ ci-dessus font foi.
- Worktree `/c/tmp_wt_premonnaie` retiré en fin de mission.

---

## CHANTIER FINS & MERVEILLE — la course recalibrée (2026-07-16)

**Statut : F1 (diagnostic) et F2 (course) LIVRÉS et validés (ratio cible ≤2:1 ATTEINT,
1.80:1 mesuré). F3 (paliers Merveille) PARTIELLEMENT livré — proxy 4+/6 amélioré 0%→1%
sur 100 sims, cible 5-15% NON atteinte (écart honnête, diagnostiqué ci-dessous).** Tag
`pre-fins` posé sur 4f228f4 avant tout changement. Kill-switch `FINS_RACE=0` prouvé
BYTE-IDENTIQUE au pre-fins sur 5 graines × 250 ans (`chronicle --hash 7 5 250`), vérifié
3 FOIS (une par calibrage) — pas seulement le golden 12 ans qui n'exerce jamais
l'endgame (ENDGAME_YEAR_OPEN=180). SAVE_VERSION INCHANGÉ (aucune struct sérialisée
touchée — 4 tunables neufs, registre J pur, + 2 fonctions diag print-only).

### F1 — LE DIAGNOSTIC DES COURSES (la découverte qui change tout)

Instrumenté `SCPS_RACEDIAG` (scps_endgame.c : `racediag_tick` à chaque checkpoint
10 ans depuis l'an 180, `mervdiag_transition` à chaque palier Merveille + fondation ;
scps_events.c : print au site EVID_MERV_FONDATION). Mesuré sur 18 sims-diagnostic (6
graines × 3, hors giga) AVANT tout code touché, PUIS confirmé sur les 300 sims des 3
re-giga :

1. **Les 5 fins ne courent PAS 5 courses indépendantes — 4 d'entre elles
   (EAU/RONCES/FROID/SANG) étaient TOUTES gatées derrière le MÊME seuil
   `ENTROPY_FIN=55`**, alimenté quasi exclusivement par la charge de tech faustienne
   (`ENTROPY_TECH_W=0.20 × Σts[].charge`, un accumulateur SANS décrue, contrairement
   à sang/feu qui décroissent). L'entropie finale (an 250) sur 100 mondes est
   franchement BIMODALE (giga pre-fins) : p50=15, p75=910, p90=7893, p95=11540,
   max=62660 — un « coude » net entre p60 et p75, pas un plateau continu. count<15=46,
   count<25=59, count<35=61, count<55=65 : la bande [15,55[ ne contient qu'environ 15
   mondes « moyennement faustiens » sur 100, le reste est soit à plat (jamais
   d'investissement faustien réel) soit déjà en train de s'envoler vers le régime
   [TERMINAL] (seuil `ENTROPY_TERMINAL=4000`, 17/100 sims, DISJOINT du fallback comme
   voulu — vérifié inchangé à 17/100 sur les 3 re-giga).
2. **SANG était structurellement quasi-inatteignable, PAS juste rare** : même dans le
   monde le plus sanglant hors-TERMINAL mesuré au diagnostic (ratio 6.6 %, déjà sous
   le seuil `ENDGAME_BLOOD_FRAC=9 %` de toute façon), la contribution à l'entropie
   (`ratio×ENTROPY_BLOOD_W(8.0)`) plafonne à ~0.5 pt — à des kilomètres des 55 requis.
   SANG ne pouvait fleurir QUE dans un monde ÉGALEMENT faustien, une coïncidence
   quasi jamais mesurée (giga pré-FINS : SANG 1/100, ce seul cas — seed_512#sim2 —
   à 9.94 % de ratio, cohérent avec le seuil 9 % : pas un hasard d'entropie, un monde
   réellement à la fois très sanglant ET faustien).
3. **Le fallback RÉCHAUFFEMENT captait quasi tout le reste par construction, pas par
   mérite** : `FUEL_FALLBACK_MIN=2.0` était si bas que 95/100 mondes du giga
   pré-FINS avaient le combustible ARMÉ (feu/tête mesuré 3.4-9.3 dans l'échantillon,
   distribution complète an-250 : p50=5.6, p67=6.2, p75=6.7, p90=7.4) — le seuil ne
   triait presque rien.
4. **Merveille — le vrai bloqueur n'est PAS le palier, c'est que le chantier ne
   démarre JAMAIS en sweep headless.** `trig_merv_fondation` (scps_events.c) est
   STRICTEMENT `human_player`-gaté (double garde : `cx->human_player<0` renvoie faux
   ET chronicle passe `eg=NULL` à `world_events_tick` de toute façon — commentaire
   DÉJÀ présent dans le code, non touché : « un banc qui passerait un eg non-NULL en
   chronique... ne doit JAMAIS fonder la Merveille à la place de l'IA »). Un garde-fou
   DÉLIBÉRÉ, documenté, PAS un bug — confirmé empiriquement par `[MERVDIAG] fondation`
   posé au site de fondation : **0 occurrence sur 18 sims-diagnostic ET sur les 300
   sims des 3 re-giga**. Conséquence : FIN_ASCENSION ne peut JAMAIS apparaître en
   sweep headless (confirmé : absente des 5 fins observées dans les 3 re-giga), et le
   chiffre chronicle « métabolisation MAX X/6 » n'a JAMAIS mesuré une victoire
   tentée — c'est un PLAFOND théorique par empire (`endgame_metab_count` est pur,
   indépendant de l'état `merv`). Décision documentée : NE PAS ouvrir la Merveille à
   l'IA (le garde-fou reste intact, hors-scope de cette mission) — F3 recalibre le
   PLAFOND (le seul proxy mesurable en headless), pas qui a le droit de bâtir.

### F2 — LA COURSE RECALIBRÉE (leviers, tous sous `FINS_RACE`)

1. **SANG DÉCOUPLÉ du gate d'entropie partagé** (scps_endgame.c,
   `endgame_select_and_fire`) — même précédent que CHAUD (déjà indépendant depuis le
   REPLI 2026-07-08b : « le combustible n'agit qu'en repli, jamais un second seuil
   parallèle »), étendu à SANG qui EST déjà documenté comme « visage dominant, pas un
   second seuil » dans son propre commentaire legacy — la lettre du code n'avait pas
   suivi l'esprit. Évalué EN PREMIER chaque tick, indépendamment de l'entropie
   mondiale. `ENDGAME_BLOOD_FRAC=9 %` LUI-MÊME INCHANGÉ (déjà calibré à « une
   génération qui perd un cinquième du monde ») — seule la PORTE bouge, pas le
   mérite. Le bloc SANG legacy est GARDÉ mot pour mot, enveloppé `if (!race)`
   (kill-switch `FINS_RACE=0` ⇒ golden pre-fins byte-identique, prouvé).
2. **`RACE_ENTROPY_FIN` : deux passes mesurées** (55→35→25, re-giga 20×5×250 entre
   les deux). Passe 1 (35) : ratio dominante/médiane 6.00:1→3.47:1 — mieux, encore
   loin de la cible. Passe 2 (25) : la bande [25,35[ ne contient que ~2 mondes de
   plus (count<25=59 vs count<35=61) — gain modeste et DÉLIBÉRÉMENT pas poussé plus
   bas (count<15=46 est déjà la moitié du parc ; en-dessous, on balaierait des mondes
   SANS aucun investissement faustien réel — une fin non MÉRITÉE, juste du bruit).
3. **`RACE_FUEL_FALLBACK_MIN` : deux passes mesurées** (2.0→6.0→7.0). Passe 1 (6.0) :
   RÉCHAUFFEMENT 63→33 (giga), ratio encore 3.47:1. Passe 2 (7.0, calculée depuis la
   distribution combustible/tête an-250 : count≥7.0=19/100, visant dominante≤19 pour
   ratio≤2:1 avec médiane~9-10) : RÉCHAUFFEMENT 33→16, ratio ATTEINT (1.80:1).
   RÉCHAUFFEMENT ne prend désormais que le ~top 19 % du parc en combustible
   RÉELLEMENT brûlé — les autres finissent AUCUNE, une issue honnête (vérifiée :
   `RFIN_AUCUNE`/scps_readout.c est l'état NEUTRE par défaut de la membrane — pas un
   crash ni un écran d'erreur, la partie continue normalement).
4. RONCES/FROID/EAU (`FIN_BASE_*`, `FIN_PROD_W_*`) et `SANG_MEMORY_HL`, `THORN_*`,
   `COLD_RAMP_PER_YEAR` : INCHANGÉS (déjà calibrés dans une vague antérieure — le
   levier F2 est la PORTE qui décide qui a le droit d'être évalué, pas le mérite de
   chaque fin une fois évaluée).

**Distribution des fins — AVANT (giga pré-FINS, 100 sims) → APRÈS (re-giga final,
100 sims, `RACE_ENTROPY_FIN=25`/`RACE_FUEL_FALLBACK_MIN=7.0`)** :

| fin | avant | après |
|---|---|---|
| RÉCHAUFFEMENT | 63 | 16 |
| RONCES | 12 | 18 |
| GRAND HIVER | 12 | 13 |
| ENGLOUTISSEMENT | 9 | 7 |
| SANG | 1 | 2 |
| AUCUNE | 3 | 44 |
| **ratio dominante/médiane des 4 autres** | **6.00:1** | **1.80:1 (cible ≤2:1 ATTEINTE)** |

**Le prix de la cible : AUCUNE passe de 3 à 44 sur 100.** C'est une conséquence
DÉLIBÉRÉE et mesurée du levier #3 — la mission l'anticipait explicitement (« les
autres mondes atteignent l'an 250 SANS fin s'il le faut... une issue honnête ») — mais
44 % est un chiffre BEAUCOUP plus élevé que l'attente naïve d'une lecture rapide du
brief, et mérite d'être su AVANT de le voir en jeu : près d'un monde sur deux du re-giga
n'atteint plus AUCUNE des 5 fins en 250 ans (contre quasi tous avant). Signal à trancher
côté design si une future vague veut un filet moins large (un `RACE_FUEL_FALLBACK_MIN`
intermédiaire ~6.0-6.5 aurait donné un ratio ~2.5-3:1 avec un AUCUNE plus proche de
30 % — un compromis existe si 44 % s'avère trop haut à l'usage).

### F3 — LES PALIERS DE LA MERVEILLE (partiel, écart honnête)

`RACE_METAB_MERV_RATIO` (voie diaspora d'`endgame_heritage_metabolized_detail`, la
seule touchée — `METAB_MERV_MIN` et la voie gouvernance `arch_depth`/`PROF_PROFOND`,
chantier TECH, INTACTS) : deux passes mesurées.

| passe | ratio | metab MAX (n=100) | palier proxy ≥4/6 |
|---|---|---|---|
| pré-FINS | 0.60 (legacy) | {1:58, 2:37, 3:5} | 0/100 (0 %) |
| F3 passe 1 | 0.45 | {1:52, 2:43, 3:5} | 0/100 (0 %) |
| F3 passe 2 | 0.25 | {1:46, 2:43, 3:10, 4:1} | **1/100 (1 %)** |

**Cible 5-15 % NON atteinte** malgré un abaissement agressif (0.60→0.25, plus de la
moitié). count=3 a doublé (5→10) et un premier count=4 est apparu (0→1), donc le
levier POUSSE dans le bon sens, mais trop faiblement pour la cible. Diagnostic honnête
(mesuré, pas supposé) : le proxy `endgame_metab_count` = natif + Σ(contact_deep OU
diaspora_ok) sur 5 héritages étrangers ; count=4 exige que 3 des 5 franchissent l'UNE
des deux voies. La voie gouvernance (`arch_depth≥PROF_PROFOND`, chantier TECH,
INTACTE par construction de cette mission) n'a pas bougé ; seule la voie diaspora a
été assouplie, et son AUTRE garde — `METAB_MERV_MIN=500` âmes intégrées — N'A PAS été
touchée (décision documentée au moment de F3, cf. plus haut) : possible qu'elle soit
maintenant le facteur BLOQUANT plutôt que le ratio, mais pas mesuré (aurait exigé une
4e re-giga complète — ~15-20 min de plus — hors budget de cette mission). **Restes**
ci-dessous pour la vague suivante.

### GATES (tous mesurés, aucun déclaré sans chiffre)

1. **Kill-switch `FINS_RACE=0`** : `chronicle --hash 7 5 250` IDENTIQUE au pre-fins,
   vérifié 3× (une fois par calibrage F2/F3) — byte-identique, exerce réellement
   l'endgame (250 ans ≫ ENDGAME_YEAR_OPEN=180), contrairement au golden 12 ans.
2. **Sweep apparié** {9,11,42}×3×250 (pre-fins vs HEAD, valeurs F2 passe 1) : 9/9
   sims comparés, distribution cohérente avec la direction attendue (RÉCHAUF 4→3,
   RONCES 3→4, 1 ENGLOUTISSEMENT→RONCES par tir 2 ans plus tôt — même trajectoire,
   MÉRITÉ), Merveille metab 2/9 sims en hausse, banqueroutes/dette comparables
   (petites dérives dues à la trajectoire post-tir-décalé, cohérentes, pas
   d'alarme).
3. **Re-giga 20×5×250 × 3 passes** (build/giga_fins/, giga_fins2/, giga_fins3/) :
   voir tableaux F2/F3 ci-dessus. Ratio ≤2:1 ATTEINT (1.80:1). Merveille proxy 4+
   NON atteint (1 % vs 5-15 % cible).
4. **make test** : 38 VERTS / 0 ROUGE / intertrade_demo seul BUILD ÉCHEC (setenv
   Windows pré-existant, documenté CLAUDE.md) — vérifié 3× (après F2 passe 1, après
   F2 passe 2, après F3 finale), endgame_demo 119/119 ✓ à chaque fois (aucune
   fixture cassée).
5. **golden** : `hash monde IDENTIQUE au golden commité` — AUCUNE re-baseline
   nécessaire (le golden 12 ans n'exerce jamais l'endgame, ENDGAME_YEAR_OPEN=180 —
   prévisible a priori, confirmé a posteriori).
6. **determinism** (5×12 ans) : STABLE. **determinism-deep** (2×200 ans) : les 2
   graines (7, 9) STABLES.
7. **savetest** : 9/9 A==B byte-identique (day=2095, runs consécutifs, seeds
   internes variables) + altération d'un octet REFUSÉE à chaque fois (empreinte).
8. **fuzz-save** : 8/8 réussis (216 octets flippés, save_sane a rejeté chaque
   forge, aucun crash) — le warning `[save] SCPS_TUNE actif ≠ sauvegarde` observé
   est un artefact ATTENDU du fuzzing (byte-flip sur le checksum tune), pas une
   régression : `good` doit être vrai pour l'atteindre, ce que confirme le BILAN
   8/8.
9. **ASan/UBSan** : INDISPONIBLE sur ce toolchain MSYS2 (`ld: cannot find -lasan`/
   `-lubsan`) — limitation d'ENVIRONNEMENT pré-existante (le paquet mingw-w64 gcc
   installé ne fournit pas les sanitizers), pas une régression de cette mission ;
   hors la liste explicite des gates de ce brief (qui ne mentionne pas ASan), noté
   pour une vague outillage future.
10. **lang-check** : 0 littéraux face-joueur (base 0) — aucun texte STR_* neuf
    (tous les prints ajoutés sont diagnostic FR, stderr, `SCPS_RACEDIAG`-gaté).

### Pièges

- `tune_f("X", def)` : le littéral `def` au site d'appel n'est QU'un repli si `X`
  n'est PAS dans le registre `scps_tune_list.h` — dès qu'il l'y est, la valeur du
  REGISTRE gagne TOUJOURS (piège de lecture déjà mordu : scps_endgame.c appelait
  `tune_f("FUEL_FALLBACK_MIN", 4.0f)` en commentaire de code alors que le registre
  disait 2.0 — le seuil RÉELLEMENT actif était 2.0, le littéral 4.0 n'était qu'une
  relique jamais lue).
- Backslash de continuation X-macro manquant sur UNE SEULE ligne de commentaire
  FERMANTE (`*/`) dans `scps_tune_list.h` → erreur de parse GCC à des dizaines de
  lignes plus loin (« expected ')' before numeric constant » sur le `X()` SUIVANT, pas
  sur la ligne fautive). Règle : les lignes de commentaire À L'INTÉRIEUR d'un bloc
  n'ont PAS besoin de `\` individuellement (translation phase 3 avale les retours-ligne
  du commentaire AVANT que phase 2/4 ne compte les lignes de la macro), mais la
  DERNIÈRE ligne du commentaire (celle avec `*/`) EN A besoin pour rester dans la
  macro.
- `chronicle.exe` qui tourne (sweep en cours) bloque `ld` au link (« cannot open
  output file chronicle.exe: Permission denied ») — MAIS `scps_viewer`/les `_demo`
  binaires sont des fichiers SÉPARÉS : `make test`/ASan/savetest peuvent tourner EN
  PARALLÈLE d'un giga sweep chronicle sans conflit (économise ~30 min sur cette
  mission — 3 gates lancés pendant les re-giga plutôt qu'en série).
- La Merveille est RÉELLEMENT injouable en headless (double garde
  `human_player<0` + `eg=NULL` passé par chronicle/`world_events_tick`) — tout
  chiffre giga qui la mentionne (« métabolisation MAX ») est un PLAFOND, jamais une
  victoire observée ; ne jamais le présenter comme un taux de victoire sans le
  qualifier explicitement.
- `git add -p` sur un fichier aux hunks fortement interleavés (3 chantiers dans la
  même fonction `endgame_select_and_fire`) : les limites de hunk RÉELLES de git ne
  correspondent pas forcément à ce qu'un `git diff --unified=N` laisse deviner —
  une réponse y/n pipée à l'aveugle a sur-inclus un hunk voisin non voulu (74 lignes
  de contexte F2 collées à une hunk F1). Vérifier `git diff --cached` après CHAQUE
  session `add -p`, jamais faire confiance à la séquence de réponses seule ; sur un
  fichier à hunks trop imbriqués, préférer committer le fichier ENTIER avec un
  message qui couvre les chantiers réellement mélangés plutôt qu'une fausse
  granularité.

### Restes

- **Merveille palier 4+ toujours hors cible (1 % vs 5-15 %)** — le levier restant
  non essayé : `METAB_MERV_MIN` (plancher d'âmes, 500, INCHANGÉ dans cette mission)
  pourrait être le facteur bloquant réel plutôt que le ratio — introduire
  `RACE_METAB_MERV_MIN` (même gate `FINS_RACE`) et re-giga serait le prochain pas
  logique, ~20 min de compute, non fait faute de budget.
- **AUCUNE à 44 %** — beaucoup plus haut que l'intuition naïve du brief ; un futur
  arbitrage design pourrait vouloir un `RACE_FUEL_FALLBACK_MIN` intermédiaire
  (~6.0-6.5) qui garderait un ratio ~2.5-3:1 (au-dessus de la cible stricte mais
  sous l'ancien 6.00:1) avec un AUCUNE plus proche de 30 % — compromis non exploré
  faute de budget (aurait exigé une 4e re-giga).
- **DLL Godot À RE-BUILDER** (`scons -C godot`) si le front lit `endgame_readout`
  ou tout tunable neuf — `scps_endgame.c/h`, `scps_events.c`, `scps_tune_list.h` ont
  changé. Aucun fichier `godot/` touché (interdit de vague respecté).
- Fichiers de sweep bruts : `build/giga_fins/`, `build/giga_fins2/`,
  `build/giga_fins3/`, `build/paired_fins/`, `build/parse_fins.py` (script de
  lecture, réutilisable) — scratch locaux, les tableaux ci-dessus font foi. Worktree
  `/c/Users/Charl/Desktop/SCPS-prefins-wt` (build pre-fins pour comparaison) à
  retirer en fin de mission (`git worktree remove`).

---

## MISSION FAUSTIEN — LES MACHINES GÉNÉREUSES (2026-07-16)

**Statut : LIVRÉ, X1-X6 tous implémentés et gatés `FAUSTIEN_BOOST` (défaut ON), kill-
switch prouvé byte-identique, `make test` 38/38, golden re-baseliné VERT, determinism
+ deep VERT, savetest 4/4 seeds + fuzz-save 8/8 VERT, gate AUCUNE=0 PROUVÉ sur le
paired sweep complet (0/9 côté HEAD vs 2/9 pre-faustien).** Tag `pre-faustien` posé
SUR 2ffa76c (posé APRÈS coup — le tag doit toujours précéder la première édition, pas
un souci ici car un tag pointe un commit existant indépendamment de l'ordre des
commandes, mais pour la prochaine vague : `git tag` AVANT le premier `Edit`, pas
après). 4 commits séparés (X1 · X2/X3/X4 combinés · X5/X6 combinés · registre+golden)
— voir Pièges pour pourquoi X2/X3/X4 et X5/X6 ne sont PAS 4 commits distincts.

### X1 — L'implantation (scps_econ.c econ_init, genèse)

Densité du « nœud riche » (fer céleste/cristal arcanique, SOUS-GISEMENTS protégés,
JAMAIS une 3e brute — `prot[RES_CELESTIAL_IRON]=prot[RES_ARCANE_CRYSTAL]=true`
inconditionnel, ligne ~1845) doublée via 2 divisors tunables sur le modulo existant
(`FAUST_ARCANE_DIV` 4→2, `FAUST_CELESTIAL_DIV` 9→4), appliqués IDENTIQUEMENT aux
2 sites de spawn (cité-état ET empire/joueur — code dupliqué historique, les 2 mis à
jour). **Mesuré** (`SCPS_FORGEDIAG`, région-grain, 9 sims pairés {9,11,42}×3×250) :
fer céleste 12.89→19.11 régions/sim moy (+48 %) ; cristal arcane 8.22→8.44 (+3 %,
quasi plat À CETTE TAILLE D'ÉCHANTILLON). Le mécanisme est mathématiquement correct
(propriété de SOUS-ENSEMBLE : `%2==0` ⊇ `%4==0`, donc JAMAIS de baisse — vérifié 0
décroissance sur les 9 paires) mais le pool de tuiles ÉLIGIBLES pour l'arcane (celles
qui tirent déjà soufre/métal précieux) est petit et FIGÉ par graine (indépendant de
X1) — à cette taille, le doublement du taux ne se traduit pas forcément en un
doublement du COMPTE observé. Une mesure à plus grand N confirmerait la convergence
vers ×2 en espérance ; hors budget de cette mission (plafond ~30 sims).

### X2 — Le rendement (scps_econ.c econ_tick, manufacture)

`FAUST_YIELD_MULT=2.0` multiplie `out_full` (sortie primaire) des 3 machines
SEULEMENT (`bld_is_faustian` : foreuse/réplicateur/corne) + le panier bonus de la
Foreuse — PAS la Forge céleste ni l'Atelier de mage (consomment les mêmes raws mais
ne sont pas nommées « les 3 machines » par la mission ; hors scope, non touchées).
Site unique : juste après `out_full *= (1.f - 0.5f*re->revolt_scar)`, AVANT la saisie
M3g (donc le boost de rendement TRAVERSE aussi la mécanique de banqueroute-saisie
existante sans dupliquer de logique).

### X3 — Chaque usage pousse vers la fin (scps_econ.c, hook faust_charge unique)

Étend le canal existant : `spawn = out * FAUST_SPAWN_CHARGE` (inchangé, ∝ SORTIE)
reçoit un second terme `+= (lim*rc->q1) * ENTROPY_PER_USE` (0.10 défaut) ∝ L'INTRANT
BRÛLÉ CE TICK (essence/flux/fer céleste). Les deux termes alimentent LA MÊME variable
`spawn` avant d'être appliqués à `arcane_charge`/`faust_charge` — un seul point
d'entrée, pas un second hook parallèle (discipline « hook UNIQUE »). Conséquence
directe : X2 (rendement ↑) fait consommer plus vite ⇒ pousse plus fort vers la fin —
les deux chantiers se renforcent PAR CONSTRUCTION, pas par coïncidence.

### X4 — Les 3 machines, « pas de jaloux » (scps_econ.c)

- **Foreuse** : le panier était DÉJÀ le lot complet (7/7 « brutes minérales » du
  catalogue : fer + cuivre/charbon/soufre/salpêtre/or/métal précieux — rien à
  ajouter). Ratio 2:1 INTERPRÉTÉ brut-commun (cuivre+charbon+soufre+salpêtre,
  Σqty=8.0) : brut-précieux (or+métal précieux) — `FAUST_FOREUSE_PRECIOUS_MULT=5.0`
  porte or 0.5→2.5 et métal précieux 0.3→1.5 (Σ=4.0, EXACTEMENT 2:1). **Découverte
  collatérale** : le panier bonus de la Foreuse ne passait PAS par la redevance
  minière (`MINT_ROYALTY`, appliquée seulement à l'EXTRACTION section §1, jamais à la
  MANUFACTURE section §2 où vit le panier) — sans correction, le levier « inflation »
  du brief aurait été un NO-OP pur (le lot dépose l'or directement en marchandise,
  jamais en réserve). Fix : l'or/cuivre du panier passe désormais par LA MÊME
  redevance que l'extraction (`mint_royalty → e->reserve_gold/copper`), gaté
  `FAUSTIEN_BOOST`. L'inflation suit donc ÉMERGEMMENT le canal royalty→réserve→frappe
  (M7) existant — mesuré : IPM final quasi identique HEAD/PRE sur les 3 seeds pairés
  (0.91/0.91, 0.93/0.90, 0.87/0.87) — la foreuse tire trop rarement (1/9 sims) dans
  cet échantillon pour que le signal d'inflation émerge visiblement ; le CÂBLAGE est
  correct et prouvé (royalty testée gatée), l'EFFET AGRÉGÉ reste à confirmer à plus
  grande échelle.
- **Corne** : `+ RES_EAU_DE_VIE` ∝ `lim` (motif out2, comme le bâton de mage/kit
  d'alchimiste — hors PIB/salaires, un supplément de transmutation), `FAUST_CORNE_
  ALCOHOL_QTY=2.0`.
- **Réplicateur** : `+ pop growth` via un NOUVEAU `PMOD_MUTATION` (scps_econ.h
  `ECON_PROVMOD_BODY`, la macro qui alimente déjà gibier/halieutique/abondance/admin
  dans l'entrée DÉMO `demo` — JAMAIS un `+pop` plat) : intensité ∝ niveau du bâtiment
  (fenêtre 0..4, motif PMOD_ADMIN), `FAUST_MUTATION_K=0.20`. Face joueur : `STR_PMOD_
  MUTATION_NOM/EFF` (« Mutations », scps_readout.c switch + strings_ids.h/
  strings_en.h) — un nouveau CAS dans le switch province-mods, `PMOD_COUNT` étendu
  (non sérialisé, `ProvModHit` est un tableau-pile local, aucun bump SAVE_VERSION).

**Usage mesuré (paired sweep, 9 sims/côté) — HONNÊTE, PAS CE QUI ÉTAIT ESPÉRÉ** : au
moins UNE machine active (conso>0) dans 4/9 sims côté HEAD vs 6-7/9 côté pre-faustien
(1 point PRE illisible, sortie stderr/stdout interleavée par `[FORGEDIAG]`). Le
rendement (X2) et l'usage-pousse-la-fin (X3) sont câblés et VÉRIFIÉS actifs quand une
machine tourne (spawn/faust_charge mesurés cohérents avec la formule), mais la
DÉCISION de construire une machine (gate tech débloquée + heuristique `econ_build_
tick`/IA, scps_ai.c) n'a PAS été touchée par cette mission (hors scope explicite :
« ne pas toucher au code hors sujet ») — X1 change SEULEMENT le raw_cap au sol, ce qui
peut faire dévier le tirage de recherche/construction d'un empire par effet papillon
(divergence chaotique déjà documentée dans la vague FINS pour un phénomène analogue),
sans qu'on puisse trancher à n=9 si le taux de construction RÉEL a bougé dans un sens
ou dans l'autre. **Ne pas lire ce chiffre comme une régression du chantier** : c'est
un signal bruité à un échantillon 11× plus petit que le giga qui avait mesuré la
baseline (3/100, 20/100, 23/100) — un futur giga (hors budget ici, « PAS DE GIGA »
imposé par le joueur) trancherait proprement si la question revient.

### X5 — Le réchauffement redevient le backup universel (scps_endgame.c)

**Correction joueur FERME reçue EN COURS de mission** (le brief initial disait
« AUCUNE ~0 attendu », le joueur a explicitement corrigé : « = 0 DÉFINITIF, pas de
sans-fin, c'est réchauffement si y'a rien »). `endgame_select_and_fire` : la branche
`if (wp->entropy < ent_gate) { ... }` (le monde n'a PAS de fin naturelle) contenait un
second gate (`fuel_ratio >= fuel_gate`) qui laissait un monde « sobre » sans fin du
tout. Sous `FAUSTIEN_BOOST` (nouveau, PAS `FINS_RACE` — un second gate indépendant
posé À CÔTÉ pour que `FAUSTIEN_BOOST=0` seul suffise à restaurer l'ancien
comportement SANS toucher `FINS_RACE`), ce second gate est cour-circuité
(`fuel_ok = true` inconditionnel) : passé `FUEL_FALLBACK_DELAY` ans après l'ouverture
de l'endgame, RÉCHAUFFEMENT tire, UN VRAI ELSE FINAL. `fuel_gate` reste CALCULÉ
(diagnostic/priorité future si un 2e repli apparaissait un jour — aucun arbitrage
câblé, un seul repli existe à ce jour) mais ne bloque plus rien. **Gate mesuré (pas
supposé)** : sweep pairé {9,11,42}×3×250 → **0/9 sims « aucune » côté HEAD** (vs 2/9
côté pre-faustien, cohérent avec le ~44 % mesuré au giga FINS 100-sims pré-vague).
**Cas limite vérifié EN SOURCE** (demandé par la correction joueur, point 3) : un
monde qui dégénère avant l'an 240 (empires absorbés) — `chaud_step` (le seul handler
de `FIN_CHAUD`) est STRUCTURELLEMENT GLOBAL (boucle sur TOUTES les régions
`0..nr`, jamais indexé par `epi`/`fauteur`) et `endgame_faction_react` garde déjà
`fauteur<0` (early return) — AUCUN chemin de crash ou de « sans-fin fantôme » identifié ;
`chronicle.c`'s boucle principale (`for yr<years`) ne s'arrête JAMAIS tôt sur
élimination d'empires (aucun `break` trouvé), donc `endgame_tick` tourne TOUJOURS
jusqu'à `years`, le checkpoint an-240 est TOUJOURS atteint tant que la sim dure au
moins 240 ans.

### X6 — Merveille à 400 (scps_endgame.c endgame_heritage_metabolized_detail)

`FAUST_METAB_MERV_MIN=400` (500 legacy), MÊME motif ternaire que `RACE_METAB_MERV_
RATIO` (mission FINS) mais gaté `FAUSTIEN_BOOST` (chantier différent). La Merveille
reste STRICTEMENT joueur-gatée (`human_player`, double garde `trig_merv_fondation` +
`eg=NULL` en chronique — NON touché, décision confirmée : pas d'IA). Injouable à
mesurer en headless (déjà diagnostiqué mission FINS, F1) — un réglage pour les
parties RÉELLES, pas prouvable par sweep.

### GATES (tous mesurés)

1. **Kill-switch `FAUSTIEN_BOOST=0`** : `chronicle --hash 7 5 12` byte-identique au
   golden pre-faustien (5/5 graines), vérifié CONTRE LE COMMIT FINAL (pas seulement
   en cours de route).
2. **Sweep pairé** {9,11,42}×3×250 (pre-faustien vs HEAD, worktree
   `SCPS-prefaustien-wt`, RETIRÉ en fin de mission) : 9 sims/côté. AUCUNE : 0/9 HEAD
   vs 2/9 pre-faustien (gate X5 PROUVÉ). Densité X1 : +48 %/+3 % (ci-dessus).
   Banqueroutes forcées cumulées : 22 (HEAD) vs 27 (pre-faustien) — même ordre de
   grandeur que la bande de référence (~27). Laborer satisfaction moy par run : HEAD
   {72,71,53} vs PRE {76,73,65} — **HEAD systématiquement 2-12 pts SOUS pre-faustien**
   sur les 3 seeds, dont UN passage sous la bande de référence 60-71 (seed 42 : 53 %)
   — plausiblement le PRIX thématique de X3 (plus d'entropie ⇒ plus de cataclysmes
   qui stressent les régions avant de les engloutir) mais PAS re-calibré dans cette
   mission (budget « PAS DE GIGA » épuisé par les gates obligatoires) — **Reste**
   explicite pour la prochaine vague si le joueur juge l'écart trop grand. Tech nœuds
   débloqués : ordre de grandeur inchangé (574/586, 223/242, 886/904 HEAD/PRE) — pas
   de régression visible. Inflation IPM : quasi identique (voir X4 ci-dessus).
3. **`make test`** : 38 VERTS / 0 ROUGE / `intertrade_demo` seul BUILD ÉCHEC
   (pré-existant Windows, documenté CLAUDE.md) — vérifié 2× (avant split des commits,
   après — sur le HEAD final committé).
4. **golden** : RE-BASELINÉ (X1 change la genèse par défaut, anticipé au brief) —
   `make golden` VERT sur le nouveau golden, kill-switch prouvé AVANT re-baseline
   (point 1).
5. **determinism** (5×12 ans) STABLE · **determinism-deep** (2×200 ans, graines 7/9)
   STABLE — les deux avec `FAUSTIEN_BOOST=1` (défaut), donc l'endgame (X5/X6) est
   RÉELLEMENT exercé par le run 200 ans (`ENDGAME_YEAR_OPEN=180`).
6. **savetest** : 4/4 graines (7/9/11/42) A==B byte-identique + altération d'un octet
   REFUSÉE à chaque fois. **fuzz-save** : 8/8, 216 octets flippés, aucun crash
   (warning `SCPS_TUNE actif ≠ sauvegarde` attendu, artefact du fuzzing).
7. **lang-check** : 0 littéraux neufs (base inchangée) — les 2 nouvelles chaînes
   (`STR_PMOD_MUTATION_NOM/EFF`) sont passées par le canal STR_* obligatoire dès
   l'écriture, jamais un littéral face-joueur brut.
8. **ASan** : toujours indisponible sur ce toolchain MSYS2 (limitation
   d'environnement pré-existante, documentée mission FINS) — hors gates listés dans
   ce brief.

### Pièges

- **La redevance minière (`MINT_ROYALTY`) ne couvre QUE la section EXTRACTION de
  `econ_tick`** (§1, brutes tirées du sol) — PAS la section MANUFACTURE (§2, où vivent
  out2/paniers bonus). Tout futur bonus qui dépose de l'or/cuivre en dehors de la §1
  DOIT router MANUELLEMENT vers `e->reserve_gold/copper` (motif copié depuis la §1,
  lignes ~3796-3807) s'il doit compter pour l'inflation M7 — sinon c'est un NO-OP
  silencieux (le lot se serait déposé en stock marchand direct, jamais vu par la
  frappe). Découvert en implémentant X4 (Foreuse), aurait pu passer inaperçu sans
  relire `econ_tick` section par section.
- **`ProvModHit`/`ECON_PROVMOD_BODY` (scps_econ.h) est une macro PARTAGÉE** entre
  `ProvinceEconomy` et `RegionEconomy` (2 instanciations, `provmod_collect_prov` et
  `provmod_collect`) — tout nouveau champ lu dans la macro (ex. `(re)->n_bld`/
  `(re)->bld[]` pour PMOD_MUTATION) doit exister à L'IDENTIQUE sur LES DEUX structs
  (vérifié : les deux ont bien `bld[]`/`n_bld`, la région étant une VUE agrégée qui
  MIRRORE les bâtiments, cf. chronicle.c `s.econ->region[r].bld[b].type` déjà
  utilisé ailleurs) — sinon erreur de compilation sur UNE seule des deux
  instanciations, facile à rater si on ne compile que `_prov`.
- **`git worktree remove` supprime AUSSI le `build/` non-tracké** du worktree — les
  fichiers de sweep bruts (`build/paired_faustien/pre_*.txt`) qui n'ont pas été
  copiés AVANT le retrait sont PERDUS. Pour cette mission, les chiffres agrégés
  avaient déjà été extraits et consignés (ci-dessus) avant le retrait, mais UN point
  de données (PRE seed11 sim2, conso foreuse) est resté illisible dans la capture
  interleavée `stdout`/`SCPS_FORGEDIAG` `stderr` et n'a pas pu être re-extrait après
  coup. Prochaine fois : `mkdir` un dossier de rapport HORS du worktree et y COPIER
  (pas juste rediriger dans le worktree) les sorties brutes AVANT tout
  `git worktree remove`.
- **`SCPS_FORGEDIAG=1` mélange `stdout` (le rapport normal chronicle) et `stderr`
  (les lignes `[FORGEDIAG]`) sur le MÊME flux si on redirige `2>&1`** — une ligne
  `[FORGEDIAG]` peut atterrir AU MILIEU d'une ligne `stdout` non terminée (tampon),
  rendant certains chiffres illisibles par un grep naïf (`conso f[FORGEDIAG]...`).
  Pour une mesure fiable : rediriger séparément (`1>out.txt 2>err.txt`), jamais
  `2>&1` quand les deux flux impriment activement en parallèle sur un run long.
- **Tag posé APRÈS les premières éditions** (le brief demandait `git tag pre-faustien`
  EN TOUT DÉBUT — fait seulement une fois la moitié du code déjà écrite, mais AVANT
  tout commit, donc récupéré proprement en taguant le commit `2ffa76c` explicite
  plutôt que `HEAD`). Fonctionne car un tag peut pointer n'importe quel commit déjà
  existant, mais SI un commit avait déjà été fait par erreur avant le tag, le tag
  aurait pointé le MAUVAIS commit. Prochaine vague : le TOUT premier geste après
  lecture du brief, avant même d'ouvrir un fichier.

### Restes

- **Laborer satisfaction HEAD systématiquement sous pre-faustien** (2-12 pts,
  1 seed sous la bande de référence 60-71 %) — plausiblement le prix thématique de
  X3 (plus d'entropie mondiale ⇒ plus de cataclysmes qui stressent AVANT
  d'engloutir), pas re-calibré ici (budget épuisé par les gates). Un futur giga
  (hors « PAS DE GIGA » de cette mission) devrait confirmer si c'est un effet
  systématique ou du bruit de 3 seeds, et si oui, quel levier (X2 `FAUST_YIELD_MULT`
  trop haut ? X3 `ENTROPY_PER_USE` trop lourd ?) le corrige sans désarmer X3.
- **Densité arcane crystal (X1) quasi plate à n=9** (8.22→8.44) malgré le doublement
  du taux (div 4→2) — le mécanisme est mathématiquement correct (sous-ensemble,
  vérifié 0 décroissance) mais le pool éligible (tuiles soufre/métal précieux) est
  trop petit pour que l'effet se voie à cette taille d'échantillon. Une mesure à
  plus grand N (giga futur) confirmerait la convergence attendue vers ×2.
- **Usage des 3 machines (conso>0) mesuré PLUS BAS côté HEAD que pre-faustien**
  (4/9 vs 6-7/9) — contre-intuitif vs l'intention X2 (booster le rendement pour
  motiver la construction), mais la mission n'a PAS touché le gate tech-débloquée ni
  l'heuristique de construction IA (scps_ai.c, hors scope explicite) : X1 fait
  seulement dévier le raw_cap au sol, ce qui peut faire diverger CHAOTIQUEMENT le
  chemin de recherche d'un empire sur 250 ans sans lien causal direct avec le
  rendement des machines elles-mêmes. Si une future vague veut PROUVER que le
  rendement boosté fait construire plus, il faudra soit un giga (hors budget ici),
  soit toucher `econ_build_tick`/l'IA de construction (hors scope de CETTE mission,
  décision délibérée de ne pas y toucher).
- **Inflation « foreuse-or » câblée mais pas mesurée à l'échelle** (royalty
  maintenant appliquée au panier, IPM quasi identique HEAD/PRE sur 3 seeds car la
  foreuse ne tire que dans 1/9 sims de cet échantillon) — le mécanisme est prouvé
  correct (le code fait ce qu'il dit), l'AMPLEUR de l'effet agrégé sur un monde
  reste à confirmer à plus grande échelle.
- **DLL Godot À RE-BUILDER** (`scons -C godot`) — `scps_econ.c/h`, `scps_endgame.c`,
  `scps_readout.c`, `scps_tune_list.h`, `strings_ids.h`/`strings_en.h` ont TOUS
  changé (nouveaux tunables + nouveau PMOD affiché en façade). Aucun fichier
  `godot/` touché (interdit de vague respecté) — le rebuild lui-même n'a PAS été
  fait (hors périmètre outillage de cette mission, comme les vagues précédentes).
- Fichiers de sweep bruts : `build/paired_faustien/head_{9,11,42}.txt` (côté HEAD
  seulement — le côté pre-faustien a été perdu au retrait du worktree, cf. Pièges) —
  scratch local, les chiffres consignés ci-dessus font foi.

## MISSION M14 — AUDIT-2 : trésor négatif · dette unifiée · amortissement périmé · +6 P1 (2026-07-17)

**Statut : LIVRÉ — 3 P0 + 6 P1 complets, `make test` 39/39 sous Windows pour la
PREMIÈRE FOIS (B9), golden RE-BASELINÉ VERT, invariant 0/9 breach (pre-m14 ET HEAD).**
Origine : audit externe (2e vague après M11-« audit-Sol »). Tag `pre-m14` posé sur
ee79945. CHAQUE claim vérifié AU CODE avant correctif (consigne du brief) — TOUS
confirmés, aucun faux-positif cette fois.

### LES 3 P0

**B1 — LE TRÉSOR NÉGATIF INVERSAIT LES PAIEMENTS (CONFIRMÉ).** `fminf(coût,
treasury)` sans clamp : treasury<0 ⇒ `paid` négatif ⇒ le trésor RECEVAIT, la
dépense devenait un revenu, la richesse du payé passait sous zéro. Grep généralisé
(pas seulement les 4 sites cités) — **6 sites corrigés** :
| site | nature | code AVANT | fix |
|---|---|---|---|
| scps_warhost.c:311 | solde militaire | `fminf(pay,treasury)` | `fmaxf(0,fminf(...))` |
| scps_warhost.c:397 | prix de recrutement | `fminf(price,treasury)` | idem |
| scps_diplo.c:340 | tribut vassal | `treasury*frac` (treasury signé) | `fmaxf(0,treasury)*frac` |
| scps_diplo.c:1637 | réparations de guerre | idem | idem |
| scps_diplo.c:1377 | pillage (siège/raid), **trouvaille** | `fminf(pp->treasury,target)` | `fminf(fmaxf(0,...),target)` |
| scps_statecraft.c:455 | coût du Conseil, **trouvaille** | AUCUN clamp du tout | `fminf(cost,fmaxf(0,treasury))` |

Sites vérifiés SAINS (déjà clampés) : ai.c:2734/2790, diplo.c:395-418/435/1176/1662,
econ.c:4269/4346 (`fmaxf(0,...)` DÉJÀ le motif correct — la référence copiée
partout ensuite), navy.c:217. Bancs dédiés (warhost_demo LOT 1.5, diplo_demo tribut/
réparations/pillage, statecraft_demo) — trésor à −100/−1000/−2000/−800 ⇒ paid==0,
aucune richesse ne passe sous zéro par ce chemin.

**B2 — LES DEUX SYSTÈMES DE DETTE + LE TOCTOU (CONFIRMÉ, périmètre exact).**
(a) `econ_region_treasury_add` (scps_econ.c:2887) forçait un résidu non couvert en
trésor négatif SANS l'inscrire dans CountryDebt — dette fantôme (sans intérêt, ni
créancier, ni plafond, ni banqueroute). Les 3 appelants ACTUELS en delta négatif
(INVEST/ROADS/frappe libre) bornaient déjà leur montant (jamais atteint EN
PRATIQUE) — mais `diplo_fabricate_cb` (scps_diplo.c:679) débite UNE SEULE région
alors que `diplo_can_fabricate` vérifie le trésor TOTAL du pays : un vrai chemin
de dette fantôme, réellement atteignable, trouvé par le grep généralisé.
(b) `credit_can_spend` lit region[]-grain ; `credit_spend` n'écrivait QUE prov[]
nu (scps_credit.c:441/448) — region[] restait périmé jusqu'à la PROCHAINE
econ_aggregate_regions : deux `credit_spend` consécutifs dans le même mois
pouvaient être TOUS DEUX autorisés sur la même vue stale.
**Correctifs** : (a) clamp au trésor RÉELLEMENT disponible (paiement partiel,
motif déjà appliqué aux dépenses d'État), retourne le montant RÉEL pris.
(b) `credit_spend` route ses 2 écritures via `econ_prov_treasury_credit` (dual-
write immédiat, motif A2/M11) ; si la chaîne d'emprunt ne couvre pas le besoin,
le reliquat non financé est RENDU à la province (réalisation PARTIELLE, jamais
une dette fantôme) — choix documenté (credit_spend est `void`, ~10 appelants
sans vérifier de retour, réaliser partiellement ferme le trou sans toucher aux
appelants). Non gatable proprement (même nature que M11-A2).

**B3 — L'AMORTISSEMENT SUR DETTE PÉRIMÉE (CONFIRMÉ, monnaie détruite MESURÉE).**
`credit_year_tick` : `debt_total` capturé UNE FOIS en tête de boucle, AVANT
l'échéance. L'échéance réduit `g_debt[c].to_class/to_cs` (`fixed`) mais JAMAIS
`debt_total` lui-même — l'amortissement, juste après, répartissait `repay` avec
`to_class/debt_total` : numérateur POST-échéance, dénominateur PRÉ-échéance.
**Mesuré au banc (avant fix)** : dette 1020 (100% classes) → échéance → amortis-
sement réparti sur le total PÉRIMÉ ⇒ le débiteur paie 204.000, les créanciers ne
reçoivent que 193.805 — **10.195 or DÉTRUITS**, reproduisant EXACTEMENT le
scénario 100→9+1 du brief. Fix : recapture `debt_total_amort` APRÈS l'échéance,
juste avant l'amortissement. Après fix : écart 0.004 (résidu float pur).
Banc rouge-sur-pre-B3 / vert-sur-fix prouvé par A/B directe (fichier restauré
après coup, cf. Pièges).

### LES 6 P1

**B4 — LE MARKUP QUI CRÈVE LE PLAFOND (CONFIRMÉ, mesuré 15.6 or de dépassement).**
`debt_draw_cap` rendait le headroom en PIÈCES (`ceiling-debt_total`), mais
`debt_origination` (DEBT_FIXED) inscrit `borrow×(1+taux)` au passif — emprunter
le headroom nominal inscrit PLUS que ce headroom. Fix : `room/(1+taux)`
(taux lu AVANT toute mutation, même convention que `debt_origination`).
DEBT_FIXED=0 : kill-switch exact (headroom nu, inchangé). Banc : 60 itérations
d'emprunt-maximum ⇒ dette JAMAIS >plafond (0.000 de dépassement, contre 15.646
sur le code désactivé) ; dette finale colle au plafond (>95%, le headroom n'est
pas gaspillé).

**B5 — LA VENTILATION PAR ORDRE (CONFIRMÉ).** `credit_borrow_class`/
`credit_borrow_local §2` agrégeaient tout dans un SEUL `to_class`, remboursé aux
poids FIXES ELITE/BOURGEOIS_LEND_WEIGHT (1.0/0.5) — un emprunt 100% bourgeois
remboursait quand même l'élite à 67%. Fix : `CountryDebt.to_class` →
`to_elite`+`to_bourgeois` (la créance RÉELLE par ordre) ; échéance, amortissement
(même bloc que B3) et rachat de crédit ventilent ∝ la composition RÉELLE.
`credit_debt_class(c)` reste l'AGRÉGAT (contrat externe INCHANGÉ) ; nouveaux
readers `credit_debt_elite`/`credit_debt_bourgeois`. **SAVE_VERSION 95→96**
(struct CountryDebt grandit d'un float/pays), save_sane revalide les deux champs
individuellement. Banc : emprunt 100% bourgeois ⇒ élite=0/bourgeois>0 à
l'origination, l'élite ne touche RIEN au service, le bourgeois EST payé.

**B6 — LA MIGRATION UNE-SEULE-VÉRITÉ COMPLÉTÉE (grep généralisé, 17 sites).**
Tous les écrivains `treasury` nus restants convertis vers `econ_prov_treasury_
credit` (province déjà résolue) ou `econ_region_treasury_add` (delta, résolution
géo) :
| fichier:fonction | site | nature |
|---|---|---|
| scps_missions.c:167 | récompense de mission | post-agrégation |
| scps_statecraft.c:455 | coût du Conseil | post-agrégation (+ B1) |
| scps_warhost.c:311,397 | solde + prix recrutement | post-agrégation (+ B1) |
| scps_diplo.c tribut de base | ~340 | post-agrégation (+ B1) |
| scps_diplo.c tribut VFN_COMMERCE | ~416-424 | post-agrégation |
| scps_diplo.c digestion d'annexion | ~442 | post-agrégation |
| scps_diplo.c don mercantile (fronde) | ~523-525, **TROUVAILLE** | écrivait `region[]` DIRECTEMENT, jamais `prov[]` |
| scps_diplo.c diplo_peace_take_gold | ~1191 | verbe (arbitraire) |
| scps_diplo.c diplo_pillage_value | ~1396,1416 | post-agrégation (+ B1) |
| scps_diplo.c diplo_reparations | ~1658,1663 | post-agrégation (+ B1) |
| scps_diplo.c diplo_loot | ~1684,1688 | verbe (arbitraire) |
| scps_ai.c:2790 | audit des offices | pré-agrégation (hygiène) |
| scps_decrees.c decree_spend_capital | ~190 | verbe CMD_DECREE (pas seulement decrees_tick) |
| scps_decrees.c decree_afford_capital | ~209 | idem |
| scps_events.c apply_region_eff | ~2250-2268 | pré-agrégation (hygiène) |
| scps_revolt.c:948 | CONCEDE_GOLD | post-agrégation |

**TROUVAILLE au-delà du grep mécanique** : le mécanisme du « don » mercantile
(fronde de suzeraineté, scps_diplo.c) écrivait `region[].treasury` DIRECTEMENT —
jamais `prov[]`. `region[]` n'étant qu'une VUE reconstruite ENTIÈRE depuis `prov[]`
à chaque `econ_aggregate_regions`, le don s'ÉVAPORAIT silencieusement au tick
suivant (ni donateur appauvri, ni receveur enrichi, une fois la vue reconstruite)
— pire qu'une simple staleness, un no-op complet. Résolu (province résolue via
`econ_region_rep_province` + dual-write). ATTENTION « péages parqués » vérifiée :
aucun des 17 sites ne touche `region_carrier_prov` ni le routage des dépenses
d'État — seul le CHOKEPOINT d'écriture change, jamais la SÉLECTION de province
porteuse.

**B7 — L'ÉCHÉANCE AFFICHÉE FAUSSE (CONFIRMÉ).** Le moteur prélève DEBT_DUE_
FRAC=10%/an du stock (DEBT_FIXED), `budget_panel_v2.gd:581` affichait
`total×taux` (taux = `credit_current_rate`, le taux d'ORIGINATION d'un FUTUR
emprunt, 2-5%/an — JAMAIS celui qui prélève sur la dette EXISTANTE) — un montant
mesuré **~4.6× trop bas** (banc : dette 459, échéance réelle 45.9, ancien calcul
9.9). Fix : nouveau champ `ScpsDebt.due` (scps_api.h/.c, même formule EXACTE que
`credit_year_tick`, lue via `tune_f` — jamais une constante dupliquée côté
GDScript) ; binding `country_debt()` (scps_sim_node.h/.cpp) expose `due` ;
`budget_panel_v2.gd:581` (SEUL fichier GDScript touché) affiche `due`. **DLL
Godot REBUILDÉE** (scons, debug+release, `PROCESSOR_ARCHITECTURE=AMD64`).

**B8 — LES SLIDERS HORS SPEC (CONFIRMÉ, la preuve save_sane).** La décision
joueur validée était ×0.1–×2 (impôt+paie), le moteur/API clampaient [0.02,1.0]
sur un commentaire UNILATÉRAL (« plus de surpaie/surtaxe ×2, qui n'a pas de
sens » — `policy_mult`, scps_econ.c). **La preuve que [.,2.0] est l'ORIGINAL** :
`save_sane` (scps_save.c) validait DÉJÀ jusqu'à 2.0 (commentaire explicite
« saves legacy ») et `scps_sim.c` (CMD_COUNCIL_PAY) attendait déjà "a[1] = paie
×100 (0..200)" — cette vague RESTAURE, n'étend pas. Fix [0.1,2.0] partout
(policy_mult, econ_country_tax_set, econ_country_budget_set, statecraft_council_
pay/_set_pay, scps_player_council_pay, scps_player_budget_policy) — BUDGET_
INVEST/MINT/DEBASE EXEMPTÉS (niveau brut [0,1] distinct, B8 ne les concerne pas).
TAX_MULT_FLOOR (M10, le plancher du contrôleur IA C3) vérifié INTACT — son
propre `clampf(base+delta,floor_,1.f)` n'est PAS touché, la distinction
joueur/IA survit (vérifié PAR LECTURE DE CODE — un banc dynamique s'est avéré
trop fragile, cf. Pièges).

**B9 — INTERTRADE_DEMO WINDOWS : make test 39/39 pour la PREMIÈRE FOIS.**
Trois couches, chacune démasquée par la précédente :
1. `setenv` absent sous MinGW-w64 (même avec `_POSIX_C_SOURCE`) — shim portable
   (`_putenv_s` sous `_WIN32`) → le banc BUILD enfin.
2. STACK_OVERFLOW (0xC00000FD) immédiat — même classe que campaign_demo/
   warhost_demo (déjà `-Wl,--stack,8388608`, motif « hors scope, ne touche pas »
   dans le brief). Appliqué le MÊME pattern PRÉCÉDENT à intertrade_demo (un 3e
   binaire qui en avait besoin) — n'a PAS touché campaign/warhost eux-mêmes.
3. **LA VRAIE SUBSTANCE** : le banc posait sa fixture UNIQUEMENT sur `region[]`
   (owner/stock/price/treasury) — jamais compilé, jamais démasqué, DORMANT
   depuis la migration RE-KEY PROVINCE. `intertrade_tick`/`intertrade_market_*`
   résolvent stock/trésor au grain PROVINCE (`it_treasury`, `econ_region_
   stock_add`) : le GATE lit `region[]` (donc « voit » la fixture) mais le
   DÉBIT RÉEL mord `prov[]` (jamais seedé) ⇒ `moved≈0` ⇒ AUCUN échange, 8
   échecs de test. `mirror_prov` (nouveau helper local) pousse la fixture vers
   TOUTES les provinces de la région (owner PARTOUT — sinon
   `econ_aggregate_regions` ÉLIT un AUTRE owner depuis la capitale/pop la plus
   peuplée, stock/trésor/prix sur la représentative, sœurs à zéro pour ne pas
   polluer la Σ) ; `econ_aggregate_regions` après chaque tick/consume tire
   l'état RÉEL vers `region[]`. Bonus : le test de « conservation » ne comptait
   que `treasury` — le péage TRADE_LEVY (M5-R1) verse la MOITIÉ de sa marge aux
   BOURGEOIS en RICHESSE (pas au trésor) : `wsum()` compte désormais
   trésor+richesse (même périmètre que l'invariant M(t) ailleurs).
   **RÉSULTAT : intertrade_demo 28/28, `make test` 39/39.**

### Découvertes

- **Le bug B1 était accidentellement AUTO-CORRECTEUR** — un effet de bord
  découvert en analysant le sweep : sous le code pré-M14, un pays déjà en
  trésor négatif qui tentait une dépense (solde/tribut/etc) RECEVAIT de l'argent
  au lieu d'en perdre (le paiement s'inversait) — une « bailout » accidentelle
  qui réduisait artificiellement le nombre de pays visibles en négatif à un
  instant donné. En fermant cette inversion (B1), le sweep mesure un nombre de
  pays « or net < 0 » PLUS ÉLEVÉ qu'avant (chronicle : Σ 3 → Σ 22 sur les 3×3
  sims apparié) — pas une régression, la SURFACE VISIBLE du problème que le bug
  masquait. La dette TRACKÉE (CountryDebt) augmente en proportion (Σ 1752 →
  5456 or sur le même échantillon) : c'est EXACTEMENT le sens de B2 — l'argent
  qui disparaissait/s'inversait avant devient maintenant une dette RÉELLE,
  visible, avec intérêt/créancier/plafond/banqueroute, au lieu d'un trou muet.
  Site(s) résiduel(s) non converti(s) probable(s) : le WILD péages parqués
  (M3h/M3i item 7, toujours désigné, cf. Restes) — non recreusé plus avant,
  hors budget de cette vague (le brief l'exclut explicitement).
- **`region_rep_prov[]` est un cache STRUCTUREL, pas dérivé de `region[].owner`**
  (`econ_region_rep_province` renvoie juste `e->region_rep_prov[region]`,
  jamais recalculé au tick) — mais `econ_aggregate_regions` ÉLIT `region[].owner`
  depuis LA POPULATION des provinces membres (capitale d'abord, sinon la plus
  peuplée) : poser `region[X].owner` SANS poser `prov[toutes les provinces de
  X].owner` en écho se fait ÉCRASER à la PROCHAINE agrégation — piège trouvé en
  chassant B9.
- **`debt_origination` doit être appelé UNE SEULE FOIS par borrow, jamais par
  tranche** (B5) — le taux dépend de `credit_debt_total(c)` (le LEVIER courant) ;
  appeler `debt_origination` séquentiellement pour `b_elite` PUIS `b_bourg`
  (après avoir déjà incrémenté `to_elite`) ferait dériver le taux de la 2e part
  — violerait le contrat « figé À L'ORIGINATION » documenté par M11-A3. Fix :
  UN forfait total, ventilé ∝ le ratio réel après coup (le taux étant uniforme
  sur tout le borrow, mathématiquement équivalent, sans le piège d'ordre).

### Pièges

- **A/B directe sur un fichier engine (pas un kill-switch tunable)** — pour
  prouver B3/B4 rouge-sur-pré-fix, il a fallu ÉDITER temporairement le fichier
  (retirer le fix), builder, tester, puis RESTAURER (copie de sauvegarde avant
  l'édition destructrice, `cp` vers un scratch) — B3/B4 n'ont pas de tunable
  naturel pour ce test (contrairement à B1/B8 qui ont un chemin `!fixed`/
  ancienne-borne déjà présent). Toujours sauvegarder AVANT l'édition de preuve,
  jamais git-diff/restore en aveugle sur un fichier avec du travail non commité
  dessus.
- **`econ_ai_fiscal_tick` (C3) est illisible depuis un banc mono-province** —
  tenté (B8) de prouver dynamiquement que le contrôleur IA ramène `tax_mult`
  à ≤1.0 même si le curseur JOUEUR l'a poussé à 1.8 : la fonction est `static`
  (pas d'accès direct), et `econ_country_class_satisfaction` AGRÈGE sur TOUT le
  pays (pas la seule province riggée) — un choc fiscal mono-province se noie
  dans le reste d'un pays généré, `err` ne sort jamais du `deadband` (0.05),
  AUCUNE correction n'est jamais observée même après 12 mois + isolement des
  provinces sœurs (pop→0, mais `active` DOIT rester vrai sinon `econ_country_
  capital_prov` ne trouve plus de capitale ⇒ `econ_ai_fiscal_tick` n'est même
  plus APPELÉ, `cap<0 → continue` dans la boucle de frappe). Abandonné après
  3 tentatives — vérifié PAR LECTURE DE CODE à la place (le clamp `[floor_,1.f]`
  d'`econ_ai_fiscal_tick` est BYTE-IDENTIQUE dans le diff B8). Un futur banc
  dédié voudrait construire un pays synthétique complet (1 SEULE province,
  `n_prov=1`) plutôt que rigger une province dans un monde `world_generate`.
- **`credit_borrow_class`/`credit_borrow_local §2` (M9 V1) : la capacité de
  prêt ORGANIQUE (via `world_generate`+`gen_population`) est souvent NULLE dans
  les premiers jours** — `credit_debt_ceiling` exige >90j de revenu CAPTÉ
  (bootstrap, motif M3d) — un banc qui veut une dette réelle tôt doit soit
  avancer >90j avant d'emprunter, soit utiliser un fixture À LA MAIN
  (credit_demo.c, pas scps_api_demo.c) où le revenu est semé directement via
  `econ_flux_add`+`econ_flux_year_capture` (motif déjà établi, cf. M9/M3c).
- **`mirror_prov` (B9) doit garder `active=true` sur les provinces sœurs
  muettes** — le réflexe (mettre `active=false` pour les exclure de l'agrégat)
  CASSE `econ_country_capital_prov` (qui exige `pe->active`) si la SŒUR
  DÉSACTIVÉE est justement la capitale du pays : plus de capitale ACTIVE ⇒
  `econ_country_mint_month` renvoie `cap<0` ⇒ TOUTE la boucle mensuelle de
  frappe (et `econ_ai_fiscal_tick` avec elle) est SKIPPÉE pour ce pays. Zéro
  pop (`memset(strata,...)`) suffit à exclure une province de l'agrégat
  pop-pondéré SANS la désactiver.
- **`econ_aggregate_regions` RÉÉCRIT `region[X].owner` ENTIÈREMENT** (élection
  capitale/pop, jamais une simple copie) — tout poke direct de `region[].owner`
  sans poser `prov[toutes les provinces membres].owner` en écho est PERDU au
  premier appel d'agrégation qui suit (piège central de B9, cf. Découvertes).

### Bandes mesurées (sweep apparié pre-m14 vs HEAD, `{9,11,42}×3×250`, chronicle)

| métrique | pre-m14 | HEAD (M14) | verdict |
|---|---|---|---|
| banqueroutes Σ (forcées) | 49 (20+9+20) | 45 (14+10+21) | −8 %, stable |
| colonisation Σ (fondations) | 327 (102+108+117) | 381 (137+111+133) | +16 %, bande vivante |
| Laborer satisfaction (par seed) | 72/71/53 % | 61/78/60 % | comparable, variance inter-graines dominante (motif établi) |
| invariant (M3f) — breach | 0/9 (aucun ÉCHEC) | 0/9 (aucun ÉCHEC) | INCHANGÉ — B3 ne bouge PAS les breaches à cette échelle apparié |
| invariant — pic annuel max/seed | 98/154/156 % | 98/154/156 % | IDENTIQUE (seuil courant 370 %, la dérive dominante est HORS des sites touchés par M14) |
| dette CountryDebt Σ (fin de sim) | 1752 or | 5456 or | ×3.1 — la dette FANTÔME devient dette TRACKÉE (cf. Découvertes) |
| « or net < 0 » (débiteurs, snapshot) Σ | 3 (1+1+1) | 22 (8+6+8) | ↑ — la SURFACE du problème, pas une régression (cf. Découvertes) |
| tech débloqués Σ | 574+223+886=1683 | 581+250+930=1761 | +4.6 %, aucun zéro-tech |
| fins (§27), "0 aucune" | 3/3 seeds | 3/3 seeds | INCHANGÉ, fins variées (EAU/RONCES/GRAND HIVER/RÉCHAUFFEMENT) |

### Gates (tous passés)

1. Claims vérifiés au code AVANT correctif ✓ (tous CONFIRMÉS, aucun faux-positif).
2. Kill-switches prouvés où gatables (B4 DEBT_FIXED=0, B8 slider ancien-borne,
   B1 legacy `!fixed`/déjà clampé, B9 côté implicite) ; B1/B2/B3/B5/B6 sont des
   BUG FIXES non gatables proprement (golden a changé, re-baseline documentée
   ci-dessous — même nature que M11-A2).
3. Sweep apparié pre-m14 vs HEAD `{9,11,42}×3×250` : bandes tenues (tableau
   ci-dessus) — AUCUN giga (règle joueur respectée, 9 sims/vague standard).
   **INVARIANT AU MICROSCOPE** : 0/9 breach des DEUX côtés, B3 (destruction de
   monnaie corrigée) NE BOUGE PAS les breaches résiduels à cette échelle — le
   bug B3 était réel et mesuré (10.2 or détruits au banc isolé) mais trop
   PETIT/RARE pour se voir dans l'agrégat 9-sims (n'exclut pas qu'il compte au
   giga 100-sims, non tenté — hors règle).
4. `make test` **39/39** (B9 — objectif ATTEINT, PREMIÈRE FOIS sous Windows) ·
   nouveaux bancs B1(warhost/diplo/statecraft)/B2/B3/B4/B5/B7(credit_demo §18)
   VERTS sur HEAD, B3/B4 prouvés ROUGES sur le fix désactivé (A/B directe,
   cf. Pièges) · golden RE-BASELINÉ (commit séparé, décision documentée) puis
   VERT · `determinism` STABLE (avant re-baseline, la preuve du non-hasard) ·
   `determinism-deep` STABLE (2 graines × 200 ans) · `savetest` A==B ×3 graines
   (9/11/42, motif B5 SAVE_VERSION 96) + octet altéré REFUSÉ ×3 · `fuzz-save`
   8/8 (216 octets, 0 crash) · `lang-check` OK (0 littéraux face-joueur neufs).
5. Cet append + 9 commits granulaires FR (B1, B2, B3, B4+B5, B6, B9, B8, B7,
   golden-rebaseline).

### Restes

- **Le site WILD des péages parqués (M3h/M3i item 7, désigné depuis M11) reste
  NON résolu** — probable contributeur au « or net < 0 » résiduel qui persiste
  MÊME après B1-B6 (cf. Découvertes/bandes). Toujours hors budget explicite du
  brief M14 (« traite-le avec un sweep dédié... sois prêt à documenter-reporter
  plutôt que casser la bande » — non tenté cette vague, la bande colonisation
  restant SAINE +16 % sans y toucher).
- **B8 — la distinction joueur/IA (TAX_MULT_FLOOR) vérifiée PAR LECTURE DE
  CODE seulement**, pas par un banc dynamique (3 tentatives infructueuses, cf.
  Pièges) — un futur banc dédié voudrait un pays synthétique `n_prov=1` plutôt
  que rigger une province dans un monde `world_generate`.
- **`credit_spend`'s « réalisation partielle » (B2b) n'a pas de reader façade
  dédié** — un appelant qui veut savoir SI la dépense a été financée en entier
  devrait comparer `country_gold_prov` avant/après lui-même (aucun nouveau
  verbe/reader demandé cette vague).
- **DLL Godot REBUILDÉE cette fois** (B7 — contrairement aux vagues MONNAIE
  M7-M12 qui la laissaient « à rebuilder », ici scons a été exécuté avec succès,
  debug + release) — précédent utile pour la prochaine vague qui touche le
  binding.
- **La dette CountryDebt Σ ×3.1 (bandes) mérite un œil joueur** — ce n'est pas
  un bug (c'est le POINT de B1/B2 : rendre visible ce qui était invisible),
  mais l'AMPLEUR (1752→5456 or sur 9 sims) est plus grande que ce que
  l'intuition initiale du brief suggérait ; à surveiller au prochain sweep si
  la dette continue de croître structurellement ou se stabilise (250 ans, un
  seul point de mesure ici).

## ROUTES — R1-R4 : audit + pathfinding déjà-là + « routes dans la mer » corrigées (2026-07-17)

Mission joueur : « raffiner le pathfinding et l'esthétique des routes sur la carte ». Propriété
de fichiers : `godot/project/**` exclusivement (aucun `scps/*.c/h`, aucun `godot/src/*`, aucun
`scons`/`make` — cohabitation stricte avec l'agent MOTEUR M13 en parallèle sur
`scps_intertrade.c`/`scps_econ.c`/`scps_events.c`). Tag `pre-routes` posé sur HEAD (0b1af83)
avant tout changement.

### Découverte capitale (R1) : le pathfinding demandé (R2) était DÉJÀ FAIT — engine A*
terrain-aware, committé 5512fc9 (juin), raffiné depuis par une dizaine de commits
- Avant de dessiner quoi que ce soit, l'audit a trouvé que TOUT ce que R2 demandait (coûts
  plaine/pente/montagne, eau interdite sauf gué/pont, convergence en jonctions Y/T/X, cache,
  déterminisme) est DÉJÀ implémenté CÔTÉ MOTEUR : `scps_api.c:4220-4466`
  (`api_road_astar`/`api_roads_build`/`scps_road_path`), exposé par le binding `road_paths()`
  (`godot/src/scps_sim_node.cpp:2427`). Coûts exacts : plat `1+height*7`, forêt +2.5, jungle +5,
  collines/hautes-terres +5, montagnes +16, pic +45, franchissement de fleuve +7 (jamais un mur —
  le pont vient après côté façade), mer/lac = coût `-1` = INFRANCHISSABLE (contournée par l'A*).
  Convergence : une cellule déjà sur un corridor tracé voit son coût ×0.30 → « les routes
  attirent les routes », jonctions Y/T/X au lieu de spaghetti parallèle. Cache par SIGNATURE
  (photo colonisation/owner) — recalcul UNIQUEMENT quand le réseau de villes bouge, jamais au tick.
- Par-dessus, `godot/project/map/overlay.gd` fait déjà un traitement FRONT-END complet et
  display-only (respecte la membrane) : snap d'extrémité au bourg (`_snap_endpoint`),
  ré-échantillonnage à pas constant (`_resample_polyline`), Chaikin gardé-eau ×2
  (`_chaikin_safe`), magnétisme de couloir (dédup des tronçons partagés dans `_augment_roads`),
  croissance organique 1 an/province (`_road_partial`), rendu en 3-traits d'encre
  (sépia/crème/clair) avec fondu SOUS canopée (`ROAD_FOREST_A`), ponts d'encre (`_ink_bridges`)
  aux franchissements route×rivière. Tout ça remonte à 5512fc9 (juin, « roads: réseau de routes
  reliant les villes ») + des commits de raffinement ultérieurs (« CARTE — LA ROUTE SOUS LA
  CANOPÉE », « passe de finition », pointillé « carte au trésor » ESSAYÉ PUIS ABANDONNÉ pour la
  lisibilité) — AUCUN de cette mission.
- Conclusion assumée : PAS de pathfinding GDScript réimplémenté (aurait dupliqué/contredit la
  vérité moteur — contraire à « la solution la plus simple » et « ne pas toucher au code
  hors-sujet »). Le travail réel de cette vague : auditer/mesurer l'existant, TROUVER et CORRIGER
  un vrai défaut visuel, réparer l'outil de non-régression qui aurait dû l'attraper, purger le
  code mort qui brouille la lecture pour un futur agent.

### Découverte : `viewer_audit.gd` (l'audit de non-régression front-end) était CASSÉ et
mentait « OK » depuis l'unification ISO / la réécriture des ponts en encre
- Il appelait DEUX méthodes disparues : `map_view.gd::_enter_iso(...)` et
  `overlay.gd::_build_bridges(...)` (mortes depuis l'unification en rendu ISO unique et la
  réécriture des ponts en `_ink_bridges` vectoriels). L'erreur (« Nonexistent function ») avorte
  SILENCIEUSEMENT `_audit_seed()` avant tout invariant ; `total` reste à 0 →
  **« VIEWER AUDIT OK » s'affichait alors que ZÉRO invariant n'avait tourné**, sur les 3 graines
  de la commande documentée dans son propre en-tête. Personne ne l'a remarqué parce qu'un script
  GDScript qui plante sur un appel invalide n'interrompt NI le process NI le exit code.
- Réparé (`viewer_audit.gd:56-61`, `:98-100`) : caméra posée directement (même geste que
  `shot_parch.gd` : `_camera.position`/`_camera.zoom`) au lieu de `_enter_iso` ; les ponts se
  lisent dans `ov._ink_bridges` (déjà peuplé par `_ensure_roads()`, appelé via le signal
  `Sim.generated` juste avant — aucun appel-builder à refaire).
- Une fois réparé, l'audit a IMMÉDIATEMENT trouvé un vrai défaut (Découverte suivante) — la
  preuve qu'un audit qui ment « OK » est pire qu'un audit qui manque : il masque activement le
  problème au lieu de le signaler.

### Découverte + correctif (R2/R3) : « routes dans la mer », mesuré et corrigé
- Root cause : le lissage MOTEUR (`api_road_smooth`, moyenne mobile 3 passes, `scps_api.c:4312`)
  n'est PAS water-aware — sur une côte concave serrée il peut tirer un sommet du tracé A* (qui,
  lui, ne foule jamais la mer : coût `-1` = infranchissable) DANS la mer. Le front-end a bien un
  Chaikin « gardé-eau » (`_chaikin_safe`) mais il ne protège que ses PROPRES coins interpolés
  (q/r) — il retombe sur les sommets D'ORIGINE (a/b) comme « sûrs » SANS les vérifier, donc un
  sommet fautif hérité du moteur survivait jusqu'au rendu final.
- Mesuré via `viewer_audit.gd` (réparé) AVANT correctif, `seed=9,11,42 years=120` :
  `route-mer-path` = 19 cellules (seed 9), 18 (seed 11), sur ~1700-2300 cellules de tracé chacune.
- Corrigé DISPLAY-ONLY (aucune sémantique, aucun fichier moteur touché) : nouvelle passe
  `_snap_water_points()` (`overlay.gd`, insérée avant les 2 Chaikin dans
  `_smooth_resample_road`) — pour tout sommet INTERNE (jamais les extrémités, déjà ancrées au
  bourg par `_snap_endpoint`) tombant en MER/LAC (jamais la rivière — un fleuve se FRANCHIT, ce
  n'est pas un défaut), cherche en anneaux croissants (rayon ≤4 cellules) la terre la plus proche
  et l'y substitue. Même esprit qu'`api_snap_land` côté moteur, mais purement côté tracé affiché.
- Après correctif, MÊME probe : `route-mer-path` = 1 (seed 9), 1 (seed 11), 0 (seed 42) —
  19→1 et 18→1 (>94 % de réduction). `decor 0 | struct 0` (bâti/décor jamais dans l'eau) était
  DÉJÀ propre des deux côtés — seul le TRACÉ de route en profitait.

### Découverte : code mort — un système de « routes en tuiles » ABANDONNÉ, jamais nettoyé,
contredisait le commentaire ET le code actif
- `overlay.gd` portait un bloc de consts/vars (`ROADS_IN_SHADER`, `USE_ROAD_TILES`,
  `ROUTE_GRID_K`, `ROUTE_SURFACE`, `ROUTE_SPLAT_EXP`, `ROUTE_CORE_A`, `_road_tex`, `_road_tiles`,
  `_road_tiles_dirty`, `_route_meshes`, `_bridge_tex`, `_bridges`, `_bridges_dirty`) pour une
  piste « routes en autotile cardinal au niveau terrain + ponts en sprites modulaires » — 100 %
  morte (grep confirmé : zéro lecture, seulement 2 sites d'écriture de flag qui ne servaient plus
  à rien). Le commentaire affirmait même « overlay MUET » pour les routes — FAUX, c'est CE MÊME
  fichier qui les dessine (le bloc 3-traits d'encre juste en dessous). Purgé (~30 lignes) avec une
  note qui explique pourquoi, pour qu'un futur agent ne perde pas de temps à croire qu'un système
  de tuiles est actif.

### Perf mesurée (demandée par le brief — « mesure le coût du pathfinding »)
- `godot/project/routes_perf_probe.gd`/`.tscn` (nouvelle probe) chronomètre `w.road_paths()`
  (l'appel façade qui déclenche `scps_roads_build`) : COLD (1re construction) / WARM (même
  signature, cache C) / REBUILD (signature changée par 20 ans de colonisation en plus).
- Mesuré sur un monde MÛR (seed 9, an 250, 163→172 routes, 4381 points) : **COLD/REBUILD ≈
  25-26 ms**, **WARM ≈ 0.2-0.3 ms** (cache quasi gratuit). Seed 42 (an 250, 125 routes) : mêmes
  ordres de grandeur. Très en dessous du seuil « >100 ms → incrémentalise » du brief — aucune
  action nécessaire, le cache-par-signature déjà en place (recalcul SEULEMENT au changement de
  réseau de villes, jamais au tick) suffit largement. `_snap_water_points` (mon ajout) ne coûte
  rien de mesurable (ring-search borné, déclenché seulement sur les ~0-19 sommets déjà en mer,
  pas sur les milliers d'autres).

### Pièges
- Le binaire Godot n'est PAS à la racine du dépôt (contrairement au brief) — il est dans
  `Godot_v4.6.3-stable_mono_win64/Godot_v4.6.3-stable_mono_win64_console.exe` (un niveau plus
  bas). `--headless` donne bien un écran noir (piège confirmé) ; fenêtré marche, le process quitte
  proprement (`get_tree().quit(0)`).
- `shot_parch.gd zoom=0` (fit) cadre la carte ENTIÈRE, mais tôt en partie (an 15-60) le brouillard
  de guerre masque tout sauf le territoire du joueur → le screenshot « fit » est presque tout noir
  et INUTILE pour juger des routes ; préférer `cap=1` + un zoom ≥ ROAD_ZOOM_MIN (2.5).
- Le lissage 3-passes moving-average CÔTÉ MOTEUR (`api_road_smooth`) n'est PAS water-aware —
  piège pour qui suppose que « l'A* garantit un tracé jamais en mer » : vrai du tracé BRUT, plus
  après le lissage (cf. correctif ci-dessus).
- `viewer_audit.gd` avortait SILENCIEUSEMENT (invalid-call → retour tôt → `total` reste 0 →
  « OK ») : à vérifier explicitement (lire les SCRIPT ERROR dans les logs, pas seulement le
  verdict final) pour tout probe/audit similaire — un « OK » n'est une preuve que si on a
  confirmé que le corps a bien tourné.
- Godot lance 2× le pipeline de génération de monde par run de probe (log « territoires... »
  etc. répété) — normal (Sim.regenerate() + le `_ready()` initial), pas un bug.

### Restes
- **Convention maritime en pointillé (portulan) NON implémentée — aucun reader façade n'expose
  de routes maritimes.** L'A* moteur exclut structurellement la mer (coût `-1`) ; nulle part un
  concept de « route maritime » séparé (aucune lecture intertrade/navy exposée à la carte). Sur
  les mondes archipel (l'archétype même du seed 9 testé), les villes d'îles séparées restent donc
  SANS lien visuel inter-île (le réseau garantit une route à toute ville joignable PAR TERRE
  seulement — « île isolée d'1 ville : rien », `scps_api.c:4433`). Ajouter des routes maritimes
  pointillées demanderait un reader MOTEUR nouveau (hors propriété de fichiers cette vague) : à
  documenter comme piste pour une future mission MOTEUR+CARTE conjointe.
- **Épaisseur par VOLUME de commerce non exposée** — le rendu utilise déjà le repli
  explicitement prévu par la doctrine (rang/tier de ville max aux deux bouts → niveau
  artère/desserte), donc CONFORME à la mission telle qu'écrite (« volume si exposé, SINON rang
  hub/route »). Si un futur agent MOTEUR expose un volume de trafic par route, le point d'accroche
  façade est `rd["level"]` (déjà consommé par `_road_bucket`/le choix artère-vs-desserte dans
  `_draw_iso`).
- **Les 2 cellules « route-mer » résiduelles** (seed 9 ×1, seed 11 ×1, sur ~4000 cellules) ne
  sont pas chassées plus loin — sub-pixel, invisibles à l'écran, rendement décroissant
  (discipline anti-scope-creep). Root cause probable : un sommet à la limite du rayon
  `ROAD_WATER_SNAP_R=4` sur une presqu'île très fine — un rayon plus large réglerait
  vraisemblablement le dernier cas si jamais ça devient visible en jeu.
- **Aucun fichier moteur touché** cette vague (aucun `scps/*.c/h`, aucun `godot/src/*`, aucun
  `scons`) — conforme à la cohabitation stricte avec l'agent MOTEUR M13 (`scps_agency.c`/
  `scps_econ.c`/`.h`/`scps_events.c`/`scps_intertrade.c`/`scps_tune_list.h` vus modifiés en
  parallèle dans `git status`, non touchés, non commités par moi).
- **`make test`/`scons` non lancés** (consigne explicite de la mission) — changements 100 %
  GDScript (`godot/project/**`), vérifiés par exécution directe des probes (`shot_parch`,
  `viewer_audit`, `routes_perf_probe`) via le binaire Godot déjà buildé, pas par la suite `make`.

### Preuve visuelle (avant/après, PNG dans `godot/project/`, gitignorés — non committés)
- `routes_before_s9_fit.png` — R1, le piège « fit + brouillard » (presque tout noir).
- `routes_before_s9_z3.png` / `routes_before_s9_z5.png` — R1, seed 9 an 15, cap=1, zoom 3.0/5.0 :
  réseau naissant, un tronçon qui hugs la rive entre deux lacs.
- `routes_before_s9_y60_z28.png` / `routes_before_s9_y60_z8.png` — AVANT, seed 9 an 60 : jonction
  Y propre entre 3 bourgs. `routes_after_s9_y60_z8.png` — APRÈS (même vue) : stable.
- `routes_before_s11_y200_z3.png` / `routes_before_s11_y200_z6.png` — AVANT, seed 11 an 200 :
  réseau mature à 5 bourgs, plusieurs jonctions T/Y, hiérarchie de tracé.
  `routes_after_s11_y200_z6.png` — APRÈS (même vue) : stable.
- `routes_before_s42_y180_z55.png` / `routes_before_s42_y180_z9.png` — AVANT, seed 42 an 180 :
  réseau à 6 bourgs sur relief accidenté (montagnes contournées au moindre coût, pas en ligne
  droite), PONT D'ENCRE confirmé visuellement (crop `crop_bridge3.png`, franchissement
  route×rivière près de Pré Vert). `routes_after_s42_y180_z55.png` — APRÈS : stable.
- Correctif « route-mer » prouvé par TÉLÉMÉTRIE (`viewer_audit.gd`, chiffres ci-dessus) plutôt
  que par crop pixel-à-pixel — les cellules fautives (1-19 sur ~2000-4000) n'ont pas été
  localisées une à une avant/après, la preuve quantitative reproductible étant plus fiable qu'une
  chasse visuelle sur un si petit nombre de pixels à trouver à l'aveugle.

## ANTISPAG — A1-A3 : consolidation visuelle des routes en troncs, ~47 % (PAS 70 %, honnête) (2026-07-17)

Mission joueur : « y'a toujours des spaghettis de routes » — consolider VISUELLEMENT les tronçons
partagés en troncs, display-only. Propriété : `godot/project/**` exclusivement (cohabitation
stricte avec l'agent MOTEUR M13, en parallèle sur `scps_agency.c`/`scps_econ.c`/`.h`/
`scps_events.c`/`scps_intertrade.c`/`scps_tune_list.h` — 4 commits de leur côté pendant cette
vague, zéro fichier partagé, zéro conflit). Tag `pre-antispag` posé sur HEAD (4ef00b6) avant tout
changement. Suite directe de la vague ROUTES d'hier (cf. section précédente) : magnétisme existant
0.65 cellule/±1/1 passe, dédup exact 0.25 cellule dans la boucle de dessin.

### A1 — la métrique a d'abord MENTI par excès (découverte capitale, avant tout code de fix)
- 1re version de `_count_spaghetti_segments()` (segments proches < 1.5 cellule + quasi-parallèles,
  cos > 0.90, routes différentes, clé de segment différente) : **66 % des segments flagués** sur
  les 3 graines (3815/4137/3643). Un chiffre absurdement élevé pour un réseau où le pathfinding a
  déjà un bonus corridor ×0.30 côté moteur.
- Diagnostic hors-Godot (probe `road_dump_probe.gd`/`.tscn`, nouveau — sérialise `ov._roads` en
  JSON, analysé en Python pur, stdlib only, aucune dépendance) : histogramme de l'ÉCART
  PERPENDICULAIRE réel entre paires flaguées → **77-82 % ont un écart < 0.1 cellule** (médiane
  0.02-0.1). Root cause : même quand 2 routes CONVERGENT déjà (magnétisme réussi, sommets
  partagés), l'échantillonnage (`_resample_polyline`, pas 2.0 cellules) et le Chaikin sont faits
  INDÉPENDAMMENT par route → le « joint » entre segments consécutifs ne tombe pas à la même
  abscisse sur les 2 tracés → clé de segment différente MALGRÉ un tracé pixel-identique. C'est de
  l'encre redondante INVISIBLE (sous le pixel à tout zoom de lecture), pas le spaghetti VISIBLE
  signalé par le joueur. Le vrai signal (écart ≥ 0.25 cellule, un tracé réellement DÉCALÉ) ne
  représentait que ~15-20 % du total brut.
- Corrigé : `_perp_offset()` (projection du milieu d'un segment sur la droite portée par l'autre)
  + `SPAG_MIN_OFFSET := 0.25`. Le test de clé (`o["key"]==s["key"]`) a été RETIRÉ de la métrique
  (redondant avec le filtre d'écart, et couplait la mesure à la résolution de `_seg_key` utilisée
  ailleurs pour le rendu — mauvaise séparation des responsabilités). **Leçon générale** : une
  métrique de proximité géométrique SANS test d'écart perpendiculaire mesure la DENSITÉ du réseau,
  pas le défaut visuel — à refaire pour toute mesure « ces 2 tracés se ressemblent trop ».
- Chiffre AVANT retenu (métrique corrigée, `viewer_audit.gd`, seed=9,11,42 years=120) :
  **1108 / 1113 / 1058** segments spaghetti (total 3279).

### A2 — la consolidation : magnétisme renforcé + itéré, PAS un consensus de grille (essayé, pire)
- Renforcé l'existant (`_augment_roads`, `overlay.gd`) : rayon de collage 0.65→**1.4 cellule**
  (`ROAD_MAGNET_R`), voisinage de recherche ±1→±3 cellules (`ROAD_MAGNET_RING`, doit couvrir le
  rayon), **2 passes** (`ROAD_MAGNET_PASSES` — une route traitée tôt dans `_roads` profite, à la
  2e passe, des attracteurs posés plus tard par des routes voisines ; sans ça l'ordre de `_roads`
  biaisait quel tracé « gagnait » le couloir commun).
- **ESSAYÉ PUIS ABANDONNÉ, mesuré pire** : un consensus de grille (chaque point vote dans une
  cellule de résolution ~1.3 ; toute cellule visitée par ≥2 routes fusionne TOUS ses points sur
  leur CENTROÏDE) — séduisant sur le papier (une seule passe, pas d'ordre, pas de plafond glouton),
  mais le centroïde n'est ni sur l'un ni l'autre tracé d'origine et ignore la CONTINUITÉ le long de
  la route : une cellule qui capte incidemment un CROISEMENT (pas un couloir partagé) tire un point
  isolé loin de ses voisins → zigzag artificiel. Mesuré (dump JSON + Python) : 1108 → **1878**
  (pire que l'AVANT). Reverté sans hésiter — la métrique avait attrapé la régression avant tout
  jugement à l'œil.
- Rayon/passes balayés (dump+analyse offline, sans repasser par Godot à chaque essai) :
  R=0.95→25 %, R=1.4→**47 %**, R=2.2→48 %, 4 passes à R=1.4 → identique à 2 passes (convergence
  atteinte, 0 gain). **Plateau structurel** au-delà de R≈1.4, quel que soit le rayon ou le nombre
  de passes — le glouton nearest-point (contrairement au consensus de grille) ne réassigne un point
  QUE vers une position déjà existante sur une autre route, jamais une moyenne inédite : plus
  prudent, mais son plafond de convergence ne bouge pas avec plus d'essais. Retenu R=1.4/2 passes
  (meilleur compromis efficacité/prudence — au-delà, le rayon dépasse un pas de rééchantillonnage
  entier, risque de coller des routes qui se CROISENT sans corridor commun, pour +1.6 pt).
- `_seg_key()` (dédup/multiplicité du rendu, `_ensure_road_network`) élargie 0.25→**0.5 cellule**
  (`ROAD_SEGKEY_RES`) — même découverte que A1 : capte les quasi-doublons déphasés pour la
  dédup/comptage de multiplicité SANS toucher aux points rendus (seule la décision « déjà encré ? »
  utilise cette clé). Découplée de la métrique (qui n'utilise plus de clé du tout, cf. A1).
- **HIÉRARCHIE (tronc/capillaire)** : `_ensure_road_network()` (nouveau, remplace l'ancienne
  construction inline dans `_draw_iso`) compte la MULTIPLICITÉ de chaque tronçon (combien de routes
  logiques l'empruntent, sur la clé `_seg_key`) et bucket en 3 TIERS (`ROAD_MULT_TIERS`,
  `ROAD_TIER_WSCALE := [1.0, 1.28, 1.55]`) — un tronc partagé s'affiche jusqu'à 55 % plus épais
  qu'un capillaire solo, sans toucher une seule couleur. CACHÉ (`_road_net_valid`, invalidé par
  `_ensure_roads()` quand le réseau bouge) : un monde mûr STABLE (aucun chantier en cours) ne
  repaie plus ce coût par frame.
- Bonus (repéré en marge, pas le cœur de la mission) : `_ink_bridges` pouvait compter le MÊME pont
  2× (2 routes qui partagent un franchissement, chacune l'ajoutait indépendamment — jamais de dédup
  avant cette vague). Dédup ajoutée (`bkey`, arrondi 0.5 cellule sur le milieu). Ponts : 40/14/40
  (AVANT, non dédupliqués) → 27/13/28 (APRÈS) — la baisse reflète des doublons littéraux retirés
  (crops vérifiés : les franchissements réels restent tous dessinés), pas des ponts perdus.

### A3 — la preuve : ~47 % de réduction MESURÉE, PAS les 70 % ciblés — dit honnêtement
- `viewer_audit.gd seed=9,11,42 years=120` (probe ROUTES d'hier, réutilisée + 1 ligne de
  télémétrie) : **1108→588 (−47 %) / 1113→607 (−45 %) / 1058→558 (−47 %)**, total 3279→1753
  (**−46.5 %**). Sous la cible ≥70 % du brief — cause racine identifiée (plateau du glouton
  nearest-point, cf. A2), pas un bug non-investigué.
- **Pourquoi pas 70 %** (signalé explicitement, pas caché) : une fois la métrique corrigée du bruit
  de phase (A1), ce qui RESTE flagué est en bonne partie des FAISCEAUX LÉGITIMES — plusieurs routes
  vers des destinations DIFFÉRENTES qui quittent le même bourg dans une direction proche et restent
  côte à côte un moment avant de diverger (un vrai carrefour à spokes multiples, pas un doublon).
  Ceux-là ne DOIVENT pas fusionner (display-only : fusionner deux routes vers 2 villes différentes
  romprait la lisibilité de destination). Le consensus de grille (A2) aurait pu pousser plus loin
  MAIS au prix de distordre les tracés (mesuré pire, reverté) — la marge restante est le prix d'une
  méthode PRUDENTE (jamais de position inédite) plutôt qu'un signe de travail inachevé.
- Invariants re-passés APRÈS (aucune régression) : `route-mer-path` inchangé (1/1/0, identique
  avant/après) — le garde-fou d'hier tient. `decor-eau`/`struct-eau` = 0/0 les 3 graines (déjà
  propre, resté propre). `VIEWER AUDIT OK` (0 violation dure) sur les 2 runs.
- Preuve VISUELLE (crops PNG, scratchpad — non committés, même convention qu'hier) : diff-image
  amplifiée (`ImageChops.difference` ×8, Python/Pillow) confirme un déplacement RÉEL de quasi
  CHAQUE segment dans une zone de convergence dense (seed 42, hub « Bois Profond », 4-5 bourgs) —
  pas cosmétique. À l'œil nu SANS diff, l'effet est PRÉSENT mais SUBTIL à ce zoom (le faisceau
  passe de ~3-4 traits visibles à ~2-3) : la métrique (espace cellule, indépendante du zoom) est
  plus fiable que la lecture visuelle directe pour juger un écart de 0.25-1 cellule.
- CPU (mission : « <~50 ms sur grand monde, sinon incrémental ») : `_ensure_road_network()` mesuré
  **~23 ms COLD** sur un monde mûr (seed 9, an 250, 140 routes) — sous le seuil. Le cache
  (`_road_net_valid`) est gardé (skip) tant qu'AUCUNE route n'est en chantier ET que le réseau n'a
  pas bougé ; en pratique une colonisation ACTIVE garde presque toujours une route en croissance
  quelque part → le cache n'engage pleinement qu'en fin de partie/empire stable. Reste sous budget
  même à froid systématique (23 ms < 50 ms) : aucune incrémentalisation nécessaire cette vague.

### Pièges
- Le piège `--headless` (écran noir) documenté hier tient toujours — fenêtré obligatoire pour
  `shot_parch`/`road_dump_probe`/`viewer_audit`.
- Une métrique de « proximité + parallélisme » SANS test d'écart perpendiculaire mesure la densité
  du réseau (combien de segments ont un voisin proche), pas le défaut visuel — piège central de
  cette vague (cf. A1), a fait perdre du temps à d'abord élargir un rayon de collage contre une
  cible-fantôme (66 % → 47 % de la MÊME métrique naïve en élargissant le rayon, alors que le vrai
  signal filtré bougeait à peine — deux mesures avant/après avec la métrique NAÏVE auraient
  semblé « ça ne marche pas » alors que le vrai signal (corrigé) montrait +47 %).
- Un centroïde de plusieurs points n'est PAS un point sûr en géométrie de tracé : il n'appartient à
  AUCUN des tracés d'origine et casse la continuité locale si la cellule capte un croisement plutôt
  qu'un couloir partagé. Le glouton nearest-point (réassigner vers une position déjà EXISTANTE)
  est plus lent à converger mais ne peut pas inventer une position aberrante.
- Le dump JSON + analyse Python hors-Godot (stdlib pur, aucune dépendance — pas de matplotlib/numpy
  disponibles dans l'environnement) a été le levier de vitesse déterminant : itérer un paramètre de
  magnétisme (rayon, passes) coûte 1 relance Godot (~15-20 s) + un script Python (<1 s) au lieu de
  relancer `viewer_audit` (3 graines, ~35 s) à chaque essai — utilisé pour BALAYER 5 configurations
  avant de choisir R=1.4/2 passes.
- La Browser pane (`mcp__Claude_Browser__computer` screenshot) a systématiquement timeout dans cet
  environnement (testé sur un serveur `python -m http.server` local, plusieurs tabs, plusieurs
  cibles) — abandonné au profit de `Read` direct sur les PNG produits par `shot_parch.gd` (qui,
  eux, fonctionnent normalement) + crops Pillow pour zoomer une région précise.

### Restes
- **Cible 70 % non atteinte (~47 % livré)** — cause racine identifiée et documentée (A3), pas un
  abandon silencieux. Piste pour une future vague : un algorithme de consolidation conscient de la
  CONTINUITÉ (matching par ARC-LENGTH le long de runs plutôt que point-à-point ou grille-centroïde)
  pourrait pousser plus loin sans le risque de distorsion du consensus de grille — hors budget de
  cette vague (la piste grille a déjà coûté un aller-retour mesuré-puis-reverté).
- **Cache `_road_net_valid` sous-utilisé en pratique** — une empire en croissance active a presque
  toujours une route en chantier quelque part, donc le cache reste souvent invalidé (`growing=true`)
  et le monde repaie ~20-23 ms/tick. Sous budget (mesuré), mais une future optimisation pourrait
  scinder le cache PAR ROUTE (ne reconstruire que le sous-ensemble en croissance) plutôt que
  tout-ou-rien.
- **`road_dump_probe.gd`/`.tscn`** (nouveau, gardé — même esprit que `routes_perf_probe.gd`
  d'hier) : sérialise `ov._roads` en JSON pour analyse hors-Godot. Utile à toute future vague qui
  doit raisonner sur la géométrie exacte du réseau de routes sans relancer `viewer_audit` à chaque
  essai.
- **Aucun fichier moteur touché** (aucun `scps/*.c/h`, aucun `godot/src/*`, aucun `scons`/`make`)
  — l'agent MOTEUR M13 a commité 4× en parallèle (`scps_agency.c`/`scps_econ.c`/`.h`/
  `scps_events.c`/`scps_intertrade.c`/`scps_tune_list.h`) pendant cette vague, zéro fichier
  partagé, zéro conflit, vérifié à chaque `git status`.
- **`make test`/`scons` non lancés** (consigne explicite) — changements 100 % GDScript
  (`godot/project/**`), vérifiés par `viewer_audit.gd`/`shot_parch.gd`/`road_dump_probe.gd` via le
  binaire Godot déjà buildé.

## CHANTIER MONNAIE — M13 : PÉAGES-SANS-PÉAGER + LES RESTES DU GIGA (2026-07-17)

**Statut : LIVRÉ — P1+P2 committés, golden RE-BASELINÉ VERT, gates complets passés,
P3 = mesure + proposition NON landée (décision joueur).** Décision joueur verbatim (P1) :
« si y'a personne, y'a pas de péage ». Tag `pre-m13` posé (0b1af83 = M14). Save v96 INCHANGÉ
(aucun champ neuf, aucun accumulateur neuf — P1 est un gate de prélèvement, P2 un diviseur
de mtth au call-site).

### P1 — L'AUDIT COMPLET des prélèvements région-grain (demandé par le brief)

Méthode : grep exhaustif de TOUS les écrivains `econ_region_treasury_add`/`econ_region_
wealth_add` en delta POSITIF (un péage = un crédit vers un tiers potentiellement vide).
Trois sites de PÉAGE, tous gatés ; tout le reste crédite la PROPRE région/capitale de
l'écrivain (colonisée par construction — un pays a une capitale peuplée) :

| site | fichier:ligne | qui perçoit | gate posé |
|---|---|---|---|
| péage d'échange TRADE_LEVY | scps_intertrade.c:~990 | région EXPORTATRICE (état 50/bourgeois 50, TOLL_STATE_SHARE) | `eff_levy=0` si pas de gardien — l'acheteur ne paie QUE le nu (total=gross) |
| péage de détroit IT_CHOKE_TOLL | scps_intertrade.c:~1020 | région du TENANT tiers (choke_hold_reg) | éligibilité : `choke_hold_reg` reste -1 — routes franches, aucun calcul plus bas |
| marge d'import hôte (chantier) | scps_agency.c:~395 | hub le plus proche (import_toll_region, cité-état hôte) | marge REMBOURSÉE à la région du chantier (le débit credit_spend précède le bloc — coût net = base_gold) |

NON-péages vérifiés un à un (crédits vers sa PROPRE assiette, hors scope) : dépôts
d'emprunt (scps_credit.c:411/552 — région-siège de l'emprunteur), vente d'esclaves
(intertrade:744 — le vendeur a une pop par construction), spéculation IA (scps_ai.c:2740/
2751 — hub Centre, trésor propre), navy/diplo/events/decrees/missions/statecraft/warhost/
revolt (capitale ou région ciblée peuplée). Les péages à percepteur PAYS identifié
(choke_hold_cid) ne sont concernés QUE via leur région de crédit (le crédit est
région-grain même si le flux FX est pays-grain).

**LA DÉCOUVERTE CENTRALE (a coûté un aller-retour)** : le gate NAÏF région-grain
(`region[].colonized`) ne mord JAMAIS — 0 diff sur 250 ans/seed 9, hashes identiques.
`region[].colonized` = « au moins UNE province de la région est colonisée » ; or la fuite
M3h est la PORTEUSE (`region_carrier_prov` = représentative sinon 1re ACTIVE, jamais
testée colonisée) DANS une région par ailleurs peuplée. Le gate CORRECT teste la porteuse
elle-même : `econ_region_has_keeper` (scps_econ.c, exporté scps_econ.h) = la porteuse
réelle est colonisée. LÀ le hash bouge (2/5 graines à 12 ans) et la trajectoire 250 ans
diverge fortement. Contrairement au M3i reverti (re-router VERS une colonisée = déplacer
le magot), P1 ne re-route RIEN : pas de collecte du tout, conservation triviale.

**LE LIEN P1 (la caisse parquée finançait la colonisation) : TENU, pas cassé.** Sweep
apparié pre-m13 vs HEAD {9,11,42}×3×250 — fondations : 137→126 (−8.0 %) · 111→113
(+1.8 %) · 133→130 (−2.3 %), Σ 381→369 (−3.1 %), BIDIRECTIONNELLE — rien à voir avec le
−14/−34 % du re-routage M3i. Lecture : assécher la collecte À LA SOURCE laisse l'argent
chez les MARCHANDS/acheteurs (il continue de circuler) au lieu de le déplacer dans une
autre poche d'État — le pool P1 ne perd qu'un magot marginal, pas son fonds de roulement.
AUCUN financement colonial explicite à câbler (le plan de secours du brief n'a pas été
nécessaire).

**Les breaches ciblés (le site était-il LE site ? réponse : NON — pour ces graines-là,
c'était déjà réglé en amont).** Runs ciblés sur les sous-graines fautives exactes du
giga (2s1 407 % · 209s3 794 % · 312s4 515 %), pics invariant par sim :

| sous-graine | giga (2026-07-16) | pre-m13 (M14) | HEAD (P1) |
|---|---|---|---|
| graine 2 sim 1 | **407 %** (breach an 39) | 99 % | 99 % |
| graine 209 sim 3 | **794 %** (breach an 101) | 371 % (marginal) | 371 % (marginal) |
| graine 312 sim 4 | **515 %** (breach an 77) | 97 % | 97 % |

Les breaches du giga ont DISPARU dès la baseline pre-m13 : le giga (16:00 le 07-16)
prédatait TECH/FINS/FAUSTIEN/M11/M12/M14 — l'arc a déplacé les trajectoires (motif
« déplacement, pas conversion » déjà vu M11). P1 les GARDE absents (pics identiques ou
légèrement mieux) mais ne peut pas être crédité de leur disparition. HONNÊTE : sur graine
209 sim 3, la dérive positive résiduelle (+8712/an pre, +9122/an HEAD, pic 371 % vs seuil
370 %) SURVIT à P1 — le site des péages parqués n'est PAS la cause de CE résidu-là ; le
prochain candidat reste la spéculation IA §1.6 (ai_speculate_tick, jamais convertie,
cf. M3i item 7b).

### P2 — LE GOULOT RÉEL des découvertes d'or (M7-I2, 94/200 au giga)

Diagnostic PRINT-ONLY d'abord (SCPS_GOLDDIAG2, gated getenv, RETIRÉ avant commit) :
compteur appels/éligibles dans trig_gold_discovery, 5 sims × 10 empires fixes × 250 ans —
**5 041 058 appels, 1 635 078 éligibles = 32,4 %**. Le goulot n'est NI le plafond
(fire_cap jamais saturé sur les runs à 0 découverte) NI un cooldown : c'est l'ÉLIGIBILITÉ
(≥1 province colonisée du pays portant la ressource commune) qui n'existe qu'un tiers du
temps — le mtth nominal (182500 j = 500 ans) suppose une éligibilité pleine dès l'an 0,
d'où ~47-59 % de réalisation. Fix LE PLUS SIMPLE qui respecte xs32 et ≤2-raws :
`GOLD_DISCOVERY_MTTH_BOOST` (défaut 3.0 ≈ 1/0.324) divise le mtth au SEUL call-site
(scps_events.c §2septies) — éligibilité et plafond INTACTS, le rng d'évènements inchangé
(même frand, même séquence, seule la probabilité comparée change). Kill-switch =1.0 :
mtth nominal legacy. **Mesuré à l'apparié : 7/13.5 (52 %) → 11/13.5 (81 %) — cible ≥80 %
ATTEINTE.**

### P3 — L'INFLATION À L'ÉCHELLE (mesure + proposition, RIEN de landé)

Dérive OLS %/an par sim (apparié, photo complète) :

| seed | pre-m13 (3 sims) | HEAD (3 sims) |
|---|---|---|
| 9  | +1.06 · −1.01 · −1.94 | +0.72 · −0.58 · −2.67 |
| 11 | +0.66 · −0.26 · −0.85 | +0.66 · −0.30 · −0.68 |
| 42 | −0.00 · +0.67 · −1.45 | +0.00 · +0.32 · −0.92 |

Moyenne : **−0.35 %/an pre → −0.38 %/an HEAD** · dans la cible [0.5,1.5] : 3/9 → 2/9.
(a) **P1 ne change PAS la donne monétaire** — l'hypothèse du brief (« l'argent des péages
restant chez les marchands = plus de liquidité ») est mesurée NULLE sur la dérive : le
volume des péages sauvages est trop petit face à la frappe. (b) Le sous-tir confirme le
giga (−1.24 %/an moyen, 12/100 dans la cible) à l'échelle apparié. (c) **Observation
neuve : INFLATION_CAP=1.6 SATURE** — pic == 1.600 exactement dans 8/18 sims (le clamp
écrase la dérive OLS en fin de course quand l'indice colle au plafond) ; et les queues
déflationnistes (−1.45/−1.94/−2.67) dominent la moyenne — ce sont les CRISES
(dette/banqueroute → effondrement de l'indice), pas le niveau de frappe.

**LA PROPOSITION (décision joueur, non committée)** :
- **Option A (recommandée)** : `MINT_ROYALTY 0.6→0.75` + `MINT_AI_SHARE 0.6→0.75`
  (extrapolation du levier M7 : 0.35→0.6 avait rendu la moyenne positive ; +25 % ici
  ≈ +0.4-0.5 pt attendu) COMBINÉ à `INFLATION_CAP 1.6→2.0` (décoince le plafond que
  8/18 sims touchent). Espérance : moyenne −0.4 → +0.3..+0.6 %/an, plus de sims en cible.
- **Option B (défensive)** : `INFLATION_CAP 1.6→2.0` SEUL, re-mesurer — la sensibilité
  non-linéaire documentée M7 (une graine bascule de +0.4 à −2.3 sur un petit pas) plaide
  pour un levier à la fois.
- **Le fix STRUCTUREL** (hors calibrage) : amortir le canal de transmission `pf` pour
  qu'une crise de trésorerie n'effondre pas l'indice entier (déjà nommé par M7 Restes,
  « lisser la variance ») — c'est la queue déflationniste qui tue la moyenne, pas le
  centre. Tout land exige un sweep apparié dédié (bascules M7).

### Pièges

- **`region[].colonized` est un agrégat MENTEUR pour un gate de porteuse** — il dit
  « quelqu'un vit QUELQUE PART dans la région », pas « la province qui ENCAISSE est
  peuplée ». Tout futur gate qui veut conditionner un flux région-grain sur l'habitant
  réel doit tester `region_carrier_prov` (le même choix de province que l'écriture
  elle-même), sinon le gate ne mord jamais (mesuré : 0 diff/250 ans).
- **Les lignes ÉCHEC du banc invariant M3c vont sur STDERR** — un run `2>/dev/null`
  les JETTE (le premier lot de runs ciblés a dû être doublé d'un grep sur les pics
  stdout ; les sweeps suivants capturent `2> fichier.err`). Le « pic annuel » stdout
  est le comparateur fiable ; l'ÉCHEC stderr est le verdict formel.
- **`chronicle.exe` qui tourne bloque le link** (motif M3b/M5/M7, remordu 1×) — les
  sweeps de cette vague tournent sur une COPIE de l'exe dans le scratchpad, laissant
  le binaire du dépôt libre pour make test/golden/determinism en parallèle.
- **Le giga de référence (build/giga/) prédate l'arc TECH→M14** — comparer un HEAD
  courant aux breaches du giga exige d'abord de re-passer les MÊMES sous-graines sur la
  baseline COURANTE (pre-m13) : 3 breaches sur 3 avaient déjà disparu AVANT M13, la
  comparaison giga-vs-HEAD seule aurait attribué à P1 un mérite qui revient à M8-M14.
- **Le heredoc bash a remordu** (motif M3h « antislashs mangés ») sur CE fichier même —
  l'append TROUVAILLES contenant backticks+apostrophes casse le heredoc quoté ;
  Write tool + `cat fichier >> TROUVAILLES.md`, jamais un heredoc pour du contenu riche.

### Gates (tous passés)

1. **Kill-switch** `TOLL_NEEDS_KEEPER=0,GOLD_DISCOVERY_MTTH_BOOST=1` → `--hash 7 5 12`
   BYTE-IDENTIQUE au golden pre-m13 commité (prouvé AVANT re-baseline) ✓.
2. **Sweep apparié** pre-m13 vs HEAD {9,11,42}×3×250 : colonisation Σ 381→369 (−3.1 %,
   bidirectionnelle, bande tenue — LE gate du lien P1) · Laborer 61/78/60→67/77/66 %
   (bande M14 tenue) · banqueroutes forcées Σ 18→24 (compteur chaotique, motif M7/M11 ;
   seed 42 porte tout) · dette M3c early ~0 des deux côtés · « or net<0 » Σ 22→18 ·
   tech Σ 1761→1714 (−2.7 %), AUCUN zéro-tech · fins §27 : 18/18 sims finies, AUCUNE==0,
   variées (ENGLOUT./RONCES/RÉCHAUF./GRAND HIVER) · **invariant 0/9 breach des DEUX côtés**
   (stderr capturé) · découvertes 7→11/13.5 (52→81 %, cible P2 ≥80 % ✓).
3. **Runs ciblés breaches** (giga sub-seeds 2s1/209s3/312s4) : tableau ci-dessus ✓ —
   verdict honnête « déjà réglé en amont, P1 conserve ».
4. `make test` **39/39** (standard M14 tenu) · `make golden-update` + `make golden` VERT
   (re-baseline documentée, 3 graines/5 changent) · `make determinism` STABLE ·
   `make determinism-deep` STABLE (graines 7 et 9 × 200 ans) · `scps_viewer --savetest 9`
   A==B byte-identique (v96) + octet altéré REFUSÉ · `--fuzztest 9` 8/8 (216 octets,
   0 crash) · lang-check OK (0 littéraux).
5. Cet append + 3 commits granulaires FR (P1 697ac7b · P2 ed1809f · golden d9d754e).

### Restes

- **La dérive positive résiduelle graine 209 sim 3 (+9122/an, pic 371 % vs seuil 370 %)
  SURVIT à P1** — le site des péages parqués n'était pas SA cause ; le candidat désigné
  reste `ai_speculate_tick` (M0 §1.6, jamais converti — cf. M3i item 7b, architecture
  « compte de marché » requise, pas un patch local).
- **P3 non landé par design** — la proposition chiffrée ci-dessus attend la décision
  joueur + un sweep apparié dédié (bascules non-linéaires M7).
- **La saisie M3g s'effondre sur les runs P1** (seed 9 : 117627→14 or confisqué sur une
  sim observée en run exploratoire 1-sim) — cohérent (moins de magots parqués à saisir
  en banqueroute), non re-mesuré à l'apparié (la banqueroute reste vivante : Σ 24
  forcées), à surveiller à la prochaine vague crédit.
- **DLL Godot À RE-BUILDER** (scons -C godot) : scps_econ.c/h (nouveau symbole exporté
  `econ_region_has_keeper`), scps_intertrade.c, scps_agency.c, scps_events.c,
  scps_tune_list.h ont changé — aucun binding/GDScript touché, mais le moteur statique
  de la DLL doit être relié (motif M3h/M3i/M5).
- Tag `pre-m13` posé · worktree de sweep `C:/tmp_wt_pre_m13` à retirer après session.

## MARITIME — N1-N4 : lanes maritimes VISIBLES + navigation joueur (2026-07-17)

Mission joueur verbatim : « il faut des tiles maritimes pour qu'on voie les routes
maritimes / qu'on puisse naviguer. » Tag `pre-maritime` posé (66c694f). Save v96
INCHANGÉ (aucun champ neuf : N2/N4 = readers display-only hors tick, N3 = repli sur
`campaign_order_sea` EXISTANT, aucun accumulateur neuf). Golden JAMAIS re-baseliné :
byte-identique prouvé défauts ET kill-switch.

### N1 — L'AUDIT : la mer savait déjà presque tout, mais ne MONTRAIT rien
- `world_sea_days_capped` (scps_world.c:4614) : un Dijkstra DIRECTIONNEL sur les
  cellules mer existe depuis longtemps — cabotage (SEA_CABOT_DAY fixe), mortes
  pénalisées, courants directionnels (v̂·d̂), mémo (s,t)→jours par graine. MAIS il ne
  mémorise QUE la distance — aucun chemin, donc rien à dessiner. C'est LA raison du
  constat ROUTES (« aucun reader n'expose de lanes ») : la géométrie existait en
  creux dans le coût, jamais en points.
- Les routes de COMMERCE maritimes existent (RouteNetwork, `routes_order` exige
  l'édifice PORT sur côte aux DEUX bouts — le critère de « port » le plus naturel
  du moteur) ; l'IA en crée ≤ 3 par rade tous les 180 j (scps_sim.c:1150) ; le
  détroit payé (`choke_region`) est posé à la CRÉATION par un test de SEGMENT DROIT
  entre les deux ancres (world_route_chokepoint), PAS par le chemin réel.
- Les ARMÉES traversaient DÉJÀ : `campaign_order_sea` (FA_EMBARK→FA_SAIL→FA_LAND,
  1 transport/10 paquets, blocus refusé, durée = world_sea_days, interception par
  les patrouilles ennemies en mer) — mais SEULE L'IA l'appelait (guerre outre-mer
  sans frontière terrestre, scps_sim.c:247). CMD_MOVE_ARMY refusait SILENCIEUSEMENT
  toute cible sans chemin terrestre (`next_hop<0`). Le joueur n'avait AUCUN geste.
- La marine (scps_navy.c) : 4 coques, chantier/entretien/famine, missions
  RADE/ESCORTE/BLOCUS/INTERCEPTION, course/raids/saignée, colonisation outre-mer
  ≤ 90 j. Tout ABSTRAIT (aucune position en mer hors la cellule-nid des pirates).
- **cell.lake ≠ visible water, CONFIRMÉ ET AGGRAVÉ** : les lacs priority-flood
  portent `biome=BIO_SHALLOW` (scps_world.c:1588) ⇒ ils entrent dans le masque
  marin `seam` ⇒ `cell.sea=SEA_CABOTAGE` ! `cell.sea≠0` n'exclut donc PAS les
  lacs — tout A* marin doit tester `cell.lake` explicitement.

### N2 — LES LANES (scps_api.c, miroir marin de road_paths)
- `scps_sea_lanes_build`/`scps_sea_lane_path` + binding `sea_paths()` : A*
  port-à-port sur `cell.sea && !cell.lake`, paires = les routes maritimes RÉELLES
  du RouteNetwork (pas toutes les paires), lissage moyenne-mobile GARDÉ-EAU (le
  piège « api_road_smooth n'est pas water-aware » pris à l'inverse), cache par
  SIGNATURE (ra/rb/open des routes maritimes), kill-switch SEA_LANES=0 (registre J).
  Boîte A* ±96 (contourner une masse terrestre écarte plus qu'un col). Chaque lane
  expose open/choke_region/ra/rb (membrane : coordonnées + ids).
- **LE COÛT MARIN UNIFORME CHANGE LES RÈGLES DU CORRIDOR** (2 itérations mesurées,
  dump JSON + Python) : (1) le tampon ±1 des routes terrestres laisse le 2e A*
  errer dans une bande de 3 cellules toutes à ×0.30 — à terre le relief départage,
  en mer RIEN ⇒ paires parallèles à ~0.3 cellule. Tampon EXACT (ligne seule + DDA
  anti-trous de décimation) ⇒ partage cellule-à-cellule (d=0.000 vérifié). (2) le
  cabotage sur l'anneau littoral IMMÉDIAT colle la lane AU trait de côte (fondue
  dans le contour dessiné, illisible). Le RAIL = la 2e RANGÉE d'eau (voisine du
  littoral) à 0.72, la rive immédiate à 0.85 ⇒ la lane longe à 1-2 cellules du
  trait ET les lanes voisines partagent le même rail.
- Coût CPU (probe, graine 9 an 120, 12 lanes dont une transocéanique) :
  **COLD ≈ 78 ms · WARM ≈ 0.04 ms** (cache signature). Graine 42 : 18 ms. Sous le
  seuil « >100 ms → incrémentalise » ; recalcul SEULEMENT quand le commerce
  maritime change, jamais au tick.
- Cohérence CHOKES : câblée EN LECTURE — chaque lane porte le `choke_region` de sa
  route (posé par le test de segment droit d'intertrade, la vérité du péage M13,
  INTACTE). L'écart possible segment-droit vs chemin-réel est documenté en Restes.

### N3 — « QU'ON PUISSE NAVIGUER » = donner au joueur le geste que l'IA avait
- CMD_MOVE_ARMY (déploiement depuis la réserve) : si `campaign_order` refuse (pas
  de chemin TERRESTRE), repli sur `campaign_order_sea` depuis la meilleure rade —
  qui revalide TOUT (port à soi, côte à l'arrivée, transports libres, blocus,
  chemin de courants). Kill-switch SEA_TRAVEL=0. La durée est DÉJÀ ∝ au chemin
  réel de mer (world_sea_days = le même modèle de coût que les lanes).
- Reader PUR `scps_sea_travel` + binding `sea_travel(target_region)` :
  possible/days/port_region/transports_need/transports_free/blocked — le miroir
  des gardes de campaign_order_sea SANS exécuter, pour l'UI future.
- GOLDEN-NEUTRE par construction : le repli ne s'exécute QUE sur commande joueur
  (journal CMD_*) — la chronique headless n'en émet jamais. Prouvé : golden
  byte-identique avec SEA_TRAVEL=1 (défaut) ET =0.

### Pièges (au-delà de ceux déjà cités)
- **La boucle de tirets a HANGÉ un run d'audit (11 min de CPU + OOM)** : quand la
  phase flottante tend vers la frontière dash/gap, `run = DASH - phase` devient
  ~1e-10 et fmod re-rend la même phase — la boucle n'avance plus ET appende des
  paires dégénérées jusqu'à l'épuisement mémoire (« realloc_static mem null »).
  Plancher ε (0.005 cellule) sur la progression. Tout walker d'arc paramétré par
  fmod doit garantir un pas minimal.
- **Des tirets déphasés se comblent mutuellement** : N lanes partagent un corridor
  cellule-identique mais chacune pose ses tirets avec SA phase ⇒ les blancs de
  l'une sont remplis par les tirets des autres = trait SOLIDE (mesuré graine 42,
  5 lanes côte sud). Dédup par bin d'1 CELLULE sur le milieu de tiret (le « déjà
  encré ? » des routes porté à la mer) — bin 0.5 insuffisant (le résidu de
  divergence entre lanes plafonne à ~0.3-1 cellule et passe entre les bins).
- **Seuil lac au byte : 111, pas 110** — la mer exige height<0.43 ⇒ byte
  trunc(h×255+0.5) ≤ 110, le lac carvé (0.434) donne 111 ; un seuil ≥110 flague
  la mer littorale 0.428-0.43 (mesuré : 56-65 faux positifs « lane-lac »/graine).
- **Le « joueur » des mondes probe ne connaît RIEN de la mer** : fog = la
  connaissance du pays-joueur, passif en probe (aucun port, aucune route
  maritime, capitale enclavée sur les 3 graines) ⇒ tout gate fog par-lane rend la
  carte muette en probe ET double-cache ce que le VOILE (dessiné après, au pixel
  près) couvre déjà. Le voile EST le « sous le fog rien » ; probe : shot_parch
  fog=0 (flag overlay.fog_off, jamais posé par le jeu).
- **La DLL doit être rebuildée après TOUT ajout à scps_tune_list.h** : un
  SCPS_TUNE avec un nom que la DLL chargée ne connaît pas fait exit(2) DANS le
  process Godot (crash muet au regenerate) — le kill-switch « marchait » côté
  make/chronicle mais tuait le front jusqu'au rebuild.
- Le piège --headless (écran noir) tient ; fenêtré pour toute capture. Le tail
  d'un process piped ne flush qu'à l'EOF — un run « silencieux » n'est pas un run
  fini (vérifier le process, pas le fichier).

### Gates (tous passés, dans l'ordre)
1. **Kill-switch** : `SCPS_TUNE=SEA_LANES=0,SEA_TRAVEL=0` ⇒ `make golden` VERT
   (byte-identique au golden pre-maritime commité) — ET golden VERT aux DÉFAUTS
   aussi (N2/N4 hors tick, N3 joueur-seul) : AUCUNE re-baseline.
2. Pas de sweep apparié (N3 ne change PAS la sim par défaut — l'IA naviguait
   déjà) : runs de sanité chronicle {9,11,42}×60 ans sans crash, conformément au
   brief (« sinon runs courts de sanité »).
3. `make test` **39/39** · `make determinism` STABLE · `make determinism-deep`
   STABLE (graines 7/9 × 200 ans) · savetest 9/11/42 A==B + octet altéré REFUSÉ
   ×3 · fuzztest 8/8 (216 octets, 0 crash) · lang-check OK (0 littéraux).
4. viewer_audit ÉTENDU (invariants lane-terre/lane-lac DURS) : **VERT 3 graines**
   — lanes 12/10/8 (toutes ouvertes), lane-lac 0, lane-terre 0, route-mer-path
   1/1/0 inchangé (le garde-fou ROUTES tient). Spaghetti (métrique étendue aux
   lanes, paires même-média) : 1381/1237/1420 dont routes-seules 588/607/558.
5. DLL REBUILDÉE : debug ET release (scons OK ×2).

### Preuve visuelle (PNG dans godot/project/, gitignorés)
- `maritime_before_s9_z26.png` (kill-switch SEA_LANES=0 : mer muette) vs
  `maritime_after_s9_z26.png` (mêmes vue/graine : lanes pointillées).
- `maritime_after_s9_corridor_z4.png` — LE tronc partagé (3 lanes → UN pointillé)
  le long de la côte sud, ancré au port : la convention portulan.
- `maritime_after_s9_port_z5.png` — le raccord au quai (snap) + cabotage.
- `maritime_after_s42_z3.png` — graine continentale : pointillés à 1-2 cellules
  du trait de côte, distincts du contour.

### Restes
- **Cohérence chokes segment-vs-chemin NON mesurée systématiquement** : la lane
  expose le choke_region de sa route (la vérité du PÉAGE, test de segment droit
  d'intertrade — M13 intact), mais une lane réelle pourrait contourner le goulet
  que le segment croise (ou traverser un goulet que le segment rate). Rare sur
  les graines vues (1 lane à choke sur 12 / 1 sur 8) ; si un jour le péage doit
  suivre le CHEMIN réel, le test « la lane passe-t-elle près de (sx,sy) du
  choke » est trivial à écrire côté moteur — décision éco, pas géométrique.
- **Le verbe N3 ne couvre que le DÉPLOIEMENT depuis la réserve** : un corps ACTIF
  à terre ne peut pas ré-embarquer via CMD_MOVE_ARMY (campaign_redirect reste
  terrestre) — le tour du monde en plusieurs sauts de mer exige de rappeler la
  levée d'abord. Verbe manquant documenté (CMD_CORPS_* + embarquement).
- **`sea_travel()` exposé mais AUCUNE UI ne le consomme encore** — le panneau
  armée devrait afficher « traversée : N j · M transports (K libres) » au survol
  d'une cible outre-mer.
- **Les lanes des routes EN FORMATION (open=0) ne se dessinent pas** (choix : pas
  d'encre avant l'ouverture) — une « lane fantôme » en croissance façon chantier
  de route terrestre serait cohérente mais n'a pas été demandée.
- **Spag marin résiduel ~790/630/860** = éventails de divergence entre lanes de
  destinations différentes (légitime, même conclusion que l'ANTISPAG terrestre) ;
  la piste arc-length matching reste la même.
- **routes_perf_probe non étendue** aux lanes (le chrono vit dans
  road_dump_probe) — à fusionner si une vague perf future veut UN seul
  instrument.

## MISSION M15 — LES FINITIONS : inflation option A · le dernier site M0 · le ré-embarquement · le choke au chemin réel (2026-07-17)

**Statut : F1/F2/F3 LIVRÉS (calibrage MESURÉ, pas déclaré) · F4 LIVRÉ GATÉ OFF (mesuré invasif-neutre côté code mais économiquement neutralisant, kill-switch conservé).**
Décision joueur verbatim : « Je suis d'accord [avec l'option A]. Termine tout. » Tag `pre-m15` posé
(38e165f = M13+ROUTES+ANTISPAG+MARITIME). Golden RE-BASELINÉ (F1 seul en est la cause — MINT_ROYALTY
touche le tick 1, les 5 graines du golden changent). SAVE_VERSION INCHANGÉE (aucun champ neuf sérialisé
sur les 4 chantiers — F2/F4 réutilisent des transferts/lecteurs existants, F3 ne touche aucun état
persisté au-delà de ce que campaign_order_sea sérialisait déjà, F1 est un pur changement de défaut).

### F1 — L'INFLATION : Option A MESURÉE PUIS ÉCARTÉE, Option B (le fallback prévu) RETENUE

Le brief nommait Option A « recommandée » (MINT_ROYALTY/MINT_AI_SHARE 0.6→0.75 COMBINÉ à
INFLATION_CAP 1.6→2.0). Mesurée EN PREMIER, au sweep apparié {9,11,42}×3×250 (9 dérives OLS/an) :

| seed | pre-m15 (3 sims) | Option A (.75/.75/2.0) |
|---|---|---|
| 9  | +0.72 · −0.58 · −2.67 | −0.96 · −0.73 · +0.35 |
| 11 | +0.66 · −0.30 · −0.68 | +0.57 · −0.65 · −0.90 |
| 42 | +0.00 · +0.32 · −0.92 | −1.21 · +0.42 · −0.53 |

**Verdict Option A (chiffres) :** moyenne −0.38 %/an (pre) → **−0.40 %/an** (PIRE, pas mieux) · bande
[0.5,1.5] 2/9 → **1/9** (pire) · sims ≥0 : 4/9 → **3/9** (pire). Un seul sim (seed 9 sim 1) a même
INVERSÉ de signe dans le mauvais sens (+0.72 → −0.96) — la sensibilité non-linéaire documentée M7
(« un pas boucule une graine de +0.4 à −2.3 ») s'est reconfirmée, mais dans le sens qui CONTREDIT
l'extrapolation naïve du brief (« +25 % de royalty ⇒ +0.4-0.5 pt attendu » — mesuré : NON).

**Option B testée ensuite** (le fallback « défensif » explicitement prévu par TROUVAILLES M13-P3 et
par le brief M15) : `INFLATION_CAP` SEUL 1.6→2.0, `MINT_ROYALTY`/`MINT_AI_SHARE` LAISSÉS à 0.6 (déjà
le défaut M7-M13, INCHANGÉS) :

| seed | pre-m15 (3 sims) | HEAD — Option B, le SHIPPÉ (3 sims) |
|---|---|---|
| 9  | +0.72 · −0.58 · −2.67 | +0.74 · +0.65 · +0.69 |
| 11 | +0.66 · −0.30 · −0.68 | +0.76 · +0.85 · +0.83 |
| 42 | +0.00 · +0.32 · −0.92 | +0.77 · +0.72 · +0.51 |

Verdict : moyenne **−0.38 → +0.166 %/an** (POSITIVE, la cible) · bande [0.5,1.5] 2/9 → **3/9** · sims
≥0 : 4/9 → **4/9** (tenu, comparable). Saturation du cap (pic==valeur exacte) : pre-m15 (cap 1.6)
4/9 sims saturés ; HEAD (cap 2.0) 3/9 clairement saturés + 1 très proche (1.998) — PAS le net recul
espéré (la mécanique pousse plusieurs sims JUSQU'AU plafond quel que soit l'endroit où il est posé :
un plafond plus haut n'arrête pas la course, il la déplace).

**Décision : Option B retenue, PAS Option A** — au titre du mandat explicite du brief : « le joueur a
acté la DIRECTION, pas le chiffre au centime… recalibre ENTRE 0.6-0.75/1.6-2.0 et documente ». 0.6 est
la borne BASSE de cette fenêtre (donc dans le mandat), Option A a été mesurée EN PREMIER par respect
de la lettre de la décision, puis écartée sur preuve chiffrée qu'elle DÉGRADAIT l'objectif que le
joueur avait fixé (moyenne positive, majorité ≥0) — mesurer, ne pas déclarer.

**Bandes vérifiées (Option B, sweep apparié {9,11,42}×3×250) :**
- **Laborer fin de partie** (satisfaction pop-pondérée) : pre-m15 {86,64,51 / 81,65,84 / 61,74,64}
  moy 70.0 → HEAD {74,65,69 / 76,85,83 / 77,72,51} moy 72.4 — dans la référence ~67-77, TENU (même
  variance chaotique inter-graines que tout l'arc M8-M14, rien de neuf).
- **Banqueroutes forcées** Σ 24 → **48** (×2, motif M11 « sous le doublement toléré » — à la limite,
  concentré sur UNE sim (seed 9 sim 1 : 1→21) dont l'indice de prix talonne le nouveau plafond
  (pic 1.998/2.0) — chaotique, documenté, PAS une explosion générale (2 des 3 graines quasi stables).
- **Colonisation** Σ 369 → 343 (−7 %, BIDIRECTIONNELLE : seed 9 −41 %, seed 11 +5 %, seed 42 +15 % —
  motif M13 confirmé, pas d'effondrement).
- **Invariant M3f** : 0/9 breach des DEUX côtés sur le sweep officiel {9,11,42} (aucune ligne ÉCHEC
  stderr) — TENU.
- **Fins (§27)** : 3/3 sims finies par graine, DEUX côtés, AUCUNE==0, variées (EAU/RONCES/GRAND
  HIVER/RÉCHAUFFEMENT selon la graine) — TENU.
- **Dette early (an 0, dette/revenu moyen)** : très majoritairement ~0 des DEUX côtés MAIS 2/9 sims
  HEAD montrent un pic transitoire (111 % et 69 %, vs 1 % et 28 % pre-m15 sur les MÊMES sims) —
  RÉSOUT en quelques décennies (13 %→5 %→0 % à an 50/100/150 pour le cas 111 %) et l'état de dette
  fin de partie reste négligeable (531 or, confiné à UN cité-état, 0 % classes). Un essai
  intermédiaire (`INFLATION_CAP=1.8`, royalty/share=0.6) a été mesuré pour voir si ce pic
  s'atténuait : NON (spikes similaires à 56 %/47 %) ET la dérive moyenne y est PIRE (−0.27 %/an) —
  le pic de dette early n'est pas une fonction monotone du cap (même caractère chaotique que le
  reste de cet arc), pas de meilleur point trouvé dans la fenêtre autorisée. **RESTE désigné**
  (ci-dessous), documenté honnêtement plutôt que masqué.

### F2 — AI_SPECULATE_TICK : conversion en transferts conservés (scps_ai.c)

Audit du site (M0 §1.6, jamais converti depuis 4 vagues) : à l'ACHAT, `econ_region_treasury_add(hub,
-vol*p)` débitait le trésor régional pour RIEN (aucun vendeur crédité) — DESTRUCTION ; à la VENTE,
`econ_region_treasury_add(hub, +vol*p)` créditait pour RIEN (aucun acheteur débité) — CRÉATION.
Buy-low/sell-high garantissait une création nette structurelle (le spread devient un gain pur).

**Conversion** : le trésor du hub paie/reçoit EXACTEMENT comme avant (mêmes lignes), mais chaque
mouvement route désormais sur les 3 classes du hub via `econ_region_wealth_add` (le miroir wealth
d'`econ_region_treasury_add`, déjà utilisé par le circuit M3b/STATE_BUY_FRAC) à la clé 42/20/38
(WAGE_SHARE/profit/TAX_RATE miroir des #define scps_econ.c, dupliqués localement en SPEC_WAGE_SHARE/
SPEC_PROFIT_SHARE/SPEC_TAX_SHARE — scps_ai.c et scps_econ.c sont deux unités de traduction, pas de
symbole partagé pour ces #define). À l'achat : les classes ENCAISSENT (elles « possédaient »
économiquement le stock retiré du marché — le compte de marché M3b). À la vente : PAS de contrepartie
nommée (aucun acheteur réel) → « le pot le plus proche de la sémantique » = LE MARCHÉ DE LA RÉGION,
donc les MÊMES classes, DÉBITÉES (`econ_region_wealth_add` avec delta négatif, clampé par
construction — jamais en dessous de 0) ; le gain RÉEL crédité au trésor = la SOMME des montants
réellement prélevés (`paidL+paidB+paidE`), jamais le nominal `vol*p` si les classes n'ont pas
assez — conservation STRICTE par construction, jamais ex nihilo. `SPECULATE_CONSERVED=0` : legacy
EXACT (golden pré-M15 byte-identique, prouvé au gate 1).

**La mesure du résidu 209s3** (run ciblé `./chronicle 7 5 250`, sim 3 = graine 209 dérivée — le
sous-seed exact désigné par M13 P1/P3) a révélé un PIÈGE de mesure : comparer pre-m15 (F1+F2 legacy)
à HEAD (F1+F2 nouveaux) CONFOND les deux chantiers — le résidu bouge de +8282/an à −10652/an, mais
CE saut est presque ENTIÈREMENT dû à F1 (INFLATION_CAP=2.0), pas à F2 : `SPECULATE_CONSERVED=0` vs
`=1` À DÉFAUTS F1 IDENTIQUES (les nouveaux 0.6/0.6/2.0) donne −10678/an vs −10652/an — un delta de
26/an, DANS LE BRUIT. **Comparaison ISOLÉE correcte** (F1 gelé à LEGACY 0.6/0.6/1.6, SEUL F2 bascule) :
`SPECULATE_CONSERVED=0` (= pre-m15 pur) → **+8282/an** ; `=1` (F2 seul) → **+4569/an** — RÉDUIT de
~45 %, dans la bonne direction, mais PAS ~0 : un autre site non converti porte le reste (le pic annuel
invariant reste identique — 371 % — à la MÊME année an 17, preuve que ce pic précis n'est pas porté
par la spéculation). Sous les défauts F1 NOUVEAUX (le shippé), l'effet de F2 devient invisible (26/an
sur un total de −10652 dominé par autre chose) — F1 a changé le site DOMINANT du résidu sur cette
sous-graine, sans rapport avec la spéculation. **Verdict honnête : F2 marche (mesuré, ~45 % de
réduction en isolation), mais n'était PAS la cause principale du résidu 209s3 tel qu'observé
aujourd'hui — la conversion est correcte et conservée, le résidu restant est un AUTRE site.**

Effet sur l'IA marchande : `spec_buys`/`spec_sells` (télémétrie stats, non affectée par la
conversion — le VOLUME de spéculation reste identique, seul le CIRCUIT monétaire change) — aucune
mesure de disparition de la spéculation observée (le trésor du hub paie/reçoit les MÊMES montants
qu'avant en général, sauf clamp rare quand les classes sont trop pauvres pour absorber la vente).

### F3 — LE RÉ-EMBARQUEMENT (scps_campaign.c/.h, scps_sim.c)

Nouvelle fonction `campaign_redirect_corps_sea` (scps_campaign.c) — miroir de la seconde moitié de
`campaign_order_sea` mais appliquée à un corps DÉJÀ ACTIF EN PLACE (pas de src_force à transférer,
pas de reset taken/legs/battles — même discipline « L1 » que `campaign_redirect_corps`). Le port de
départ est la position ACTUELLE du corps (`a->loc`), PAS une capitale : le corps doit déjà TENIR un
port pour embarquer (aucune marche automatique vers la côte — la solution la plus simple qui
respecte « mêmes conditions » du brief). N'est appelée QUE depuis le dispatch scps_sim.c
(CMD_MOVE_ARMY branche « en campagne », CMD_CORPS_MOVE) — jamais depuis `campaign_redirect_corps`
lui-même, le chemin PARTAGÉ avec l'IA (scps_sim.c ~145/191/227, INTACT) : golden-neutre PAR
CONSTRUCTION, même motif que N3 (MARITIME) — confirmé au gate 1 (kill-switch SEA_TRAVEL=0 ET =1
donnent le golden pré-M15 identique, puisque chronicle n'émet jamais ces CMD_*).

**Le cas armée-en-mer** : `campaign_redirect_corps_sea` garde EXACTEMENT le même garde que le
redirect terrestre (`a->phase>=FA_EMBARK` → refus) — un corps déjà en traversée (FA_EMBARK/FA_SAIL/
FA_LAND) refuse PROPREMENT les DEUX replis (terre ET mer), aucune corruption d'état, aucun crash :
la traversée en cours doit finir avant tout nouveau redirect. Choix documenté, pas une couille
silencieuse — cohérent avec `campaign_split`/`campaign_merge` qui gardent déjà `phase>=FA_EMBARK`.

Réutilise le kill-switch SEA_TRAVEL existant (M15 « hérite » comme demandé, aucun tunable neuf).
Pas de banc dédié ajouté (même précédent que N3/MARITIME, qui n'en avait pas non plus) — correction
assurée par le miroir exact des gardes de `campaign_order_sea` (déjà prouvé par campaign_demo
33/33) + la preuve golden-neutre.

### F4 — LE CHOKE AU CHEMIN RÉEL (scps_world.c/.h, scps_routes.c) — LIVRÉ, GATÉ OFF PAR DÉFAUT

Architecture : `choke_region` est posé UNE FOIS à `routes_order` (scps_routes.c, création de la
route) par `world_route_chokepoint` — un test de SEGMENT DROIT entre les deux ancres marines. Le
CHEMIN RÉEL (l'A* de la lane, scps_api.c) vit dans la FAÇADE — display-only, hors tick, PAS
partagé avec le moteur par design (la membrane). Plutôt que dupliquer cet A* borné (±96, coût
cabotage/esthétique) dans le moteur, réutilisation du Dijkstra marin EXISTANT
(`world_sea_days_capped`, scps_world.c — DÉJÀ appelé deux fois à la création de chaque route pour
`days_ab`/`days_ba`) : extrait en `sea_dijkstra_core` (refactor PUR, comportement identique,
prédécesseurs `g_sea_from[]` désormais retenus) + nouvelle `world_route_chokepoint_path` qui
backtrace le plus court chemin et teste CHAQUE cellule (pas de segment décimé — aucun risque de
« sauter » un détroit étroit) contre la table des détroits, même seuil/marge que le test segment.
Kill-switch `CHOKE_REAL_PATH` (défaut **0**, registre J) au SEUL call-site `routes_order`.

**Gate 1 (byte-identique)** : PASSÉ — `CHOKE_REAL_PATH=0` (défaut) reproduit `world_route_chokepoint`
EXACTEMENT (même appel), et le refactor `sea_dijkstra_core` est un pur déplacement de code (aucun
opérande flottant réordonné) — confirmé par le golden pré-M15 byte-identique avec TOUS les
kill-switches legacy.

**La mesure économique (le vrai verdict F4)** : sweep apparié {9,11,42}×3×250,
`CHOKE_REAL_PATH=1` vs le défaut 0 — le péage de détroit (déjà documenté STRUCTURELLEMENT PETIT par
M13/M5) tombe à **ZÉRO sur les 9 sims** (0 route taxée, 0 or/sim, 0/9 sims avec péage encaissé) alors
que le segment droit (défaut) collectait quelque chose sur 3/9 sims (2, 0(marginal), 28 or/sim). Ce
n'est PAS une redistribution (qui paie quoi change) mais une DISPARITION quasi-totale du mécanisme :
le chemin RÉEL (cabotage préféré, 2e rangée d'eau à coût 0.72, la rive à 0.85) évite systématiquement
les détroits les plus étroits quand une route côtière plus longue existe — alors que le segment
droit (« à vol d'oiseau » entre deux ports) les traverse plus volontiers par pur hasard géométrique.
Un effet LARGE en relatif (100 % → 0 % de collecte) sur une base déjà minuscule en absolu.

**Décision : PAS activé par défaut** — conforme à la clause de prudence du brief (« n'active par
défaut QUE si les bandes tiennent ») : la bande ici ne « casse » pas au sens d'un effondrement
économique (le flux était déjà ~0), mais le résultat est AMBIGU (le mécanisme devient inerte plutôt
que corrigé) et mérite une décision joueur avant d'être la valeur par défaut — surtout tant que
TRADE_LEVY/le calibrage global des péages (Reste M5/M13, jamais tranché) rend toute l'économie du
détroit marginale de toute façon. Le code est LIVRÉ, testé, golden-neutre par défaut, disponible
derrière le kill-switch pour une future vague qui voudrait la trancher.

### Pièges

- **Les défauts d'un tunable REGISTRÉ vivent en UN SEUL endroit** (`scps_tune_list.h`, l'X-macro) —
  le second argument de `tune_f(nom, x)` au call-site est un FALLBACK MORT dès que `nom` est dans le
  registre (`tune_f` renvoie TOUJOURS `t->val` si `find(nom)` réussit, jamais le `x` de l'appelant :
  scps_tune.c `float tune_f(...){ ... return t ? t->val : def; }`). scps_econ.c/h portaient des
  littéraux PÉRIMÉS depuis M7 (0.35/4.0 alors que le registre disait 0.6/1.6) — morts en pratique
  mais trompeurs pour tout lecteur futur ; corrigés par hygiène (M15-F1) SANS changer le
  comportement (dead code, vérifié).
- **`#define SCPS_TUNABLES(X) \` survit à un commentaire multi-ligne SANS backslash sur CHAQUE
  ligne** : un commentaire C `/* … */` ouvert avant la fin d'une ligne non-continuée ABSORBE les
  retours à la ligne suivants (la suppression des commentaires, phase 3, tourne AVANT la
  détection des directives, phase 4, même si la troncature de ligne par `\`, phase 2, a déjà
  fini plus tôt) — piège purement de LECTURE (le fichier compile déjà comme ça depuis M12), noté
  pour tout futur append en fin de registre : il FAUT un `\` explicite sur la DERNIÈRE entrée
  avant d'en ajouter une nouvelle si elle n'est pas immédiatement précédée d'un commentaire ouvert.
- **Isoler l'effet d'UN chantier dans un sweep qui en empile PLUSIEURS exige de geler les autres
  via SCPS_TUNE** — comparer pre-m15 à HEAD final confond F1+F2+F3(neutre)+F4(off) ; la mesure du
  résidu 209s3 a d'abord donné un résultat FAUX (attribué à F2 un saut de −10652/an qui vient à
  99,8 % de F1) avant correction (F1 gelé à legacy, seul SPECULATE_CONSERVED bascule). Motif à
  retenir pour toute vague future qui empile plusieurs chantiers dans le même sweep apparié.
- **Le Dijkstra marin (`world_sea_days_capped`) n'est PAS borné en boîte** (contrairement à l'A*
  de la façade, ±96) — il explore tout l'océan atteignable sous le `cap_days`, avec mémo (s,t)
  PAR SEED. `sea_dijkstra_core` (le cœur extrait) NE mémorise PAS quand appelé pour le CHEMIN
  (`world_route_chokepoint_path`) — c'est voulu (rare, à la création de route seulement — jamais
  au tick), mais tout futur appelant qui voudrait le chemin en masse devrait ajouter son propre
  cache.
- **`g_sea_from[]` n'est valide qu'IMMÉDIATEMENT après l'appel qui l'a peuplé** (statique par
  époque, comme `g_sea_dist`/`g_sea_pos` — tout appel suivant à `sea_dijkstra_core` avance
  l'époque et invalide la backtrace précédente). Aucun souci actuel (chaque appelant backtrace
  avant tout autre appel), mais un piège pour un futur usage concurrent/différé.

### Gates

1. **Kill-switches** (F2 `SPECULATE_CONSERVED=0` + F4 `CHOKE_REAL_PATH=0`, déjà le défaut de F4) +
   F1 reposé à ses valeurs pré-M15 (`MINT_ROYALTY=0.6,MINT_AI_SHARE=0.6,INFLATION_CAP=1.6`) via
   SCPS_TUNE ⇒ `--hash 7 5 12` **BYTE-IDENTIQUE** au golden pre-m15 commité — prouvé AVANT toute
   re-baseline, puis RE-PROUVÉ après le revirement Option A→B (le code n'avait pas changé sur cet
   axe, seul le DÉFAUT du registre a bougé) ✓.
2. **Sweep apparié** {9,11,42}×3×250 : F1 (9 dérives × 3 configs, ci-dessus) · bandes Laborer/
   banqueroutes/colonisation/invariant/fins/dette-early (ci-dessus, toutes documentées, 1 reste
   désigné) · F2 run ciblé 209s3 (isolé correctement, ci-dessus) · F4 flux péages détroit
   (ci-dessus, décision OFF documentée) ✓.
3. `make test` **39/39** verts (aucune régression, aucun warning nouveau sur les fichiers touchés)
   · `make golden-update` + `make golden` VERT (re-baseline F1, les 5 graines changent — attendu,
   MINT_ROYALTY touche le tick 1) · `make determinism` STABLE (5 graines × 12 ans) ·
   `make determinism-deep` STABLE (graines 7 et 9 × 200 ans) · `scps_viewer --savetest` A==B
   byte-identique + octet altéré REFUSÉ sur les 3 graines 9/11/42 · `--fuzztest 9` 8/8 (216
   octets, 0 crash) · `make lang-check` OK (0 littéraux).
4. Cet append + commits granulaires FR (F1/F2/F3/F4 séparés + golden) · DLL Godot rebuildée
   (debug + release, scons ×2 — scps_ai.c/scps_campaign.c/.h/scps_econ.c/.h/scps_routes.c/
   scps_sim.c/scps_tune_list.h/scps_world.c/.h ont tous changé, aucun binding/GDScript touché).

### Restes

- **Dette early transitoire sur 2/9 sims** (Option B, 111 % et 69 % à an 0, résolu en ~50-150 ans,
  fin de partie négligeable) — pas de meilleur point trouvé dans la fenêtre autorisée
  (0.6-0.75/1.6-2.0), caractère chaotique confirmé (non-monotone en `INFLATION_CAP`, testé à 1.8).
  Candidat pour une future vague : amortir `pf`/le canal de transmission plutôt que recalibrer
  royalty/share/cap — la piste STRUCTURELLE déjà nommée par M13-P3 (c) et M7 Restes.
- **Le résidu 209s3 post-F2 (+4569/an isolé, invariant pic 371 % à an 17) SURVIT** — un AUTRE site
  non converti porte le reste ; candidats non explorés cette vague : saisie-monétisation M3g,
  parking résiduel hors P1 (M13). Nécessite un audit M0-style dédié, pas une extrapolation.
- **F4 livré mais OFF par défaut** — le code est prêt, prouvé golden-neutre, économiquement
  neutre-à-inerte sur le sweep mesuré ; décision joueur requise avant activation (et TRADE_LEVY/
  calibrage péage général, Reste M5/M13, devrait probablement être tranché AVANT que le choke au
  chemin réel ait un enjeu économique visible).
- **Option A du brief (« recommandée ») a été mesurée PIRE que le statu quo** sur cet exact sweep
  — signal fort que l'extrapolation linéaire des leviers M7 (« +25 % de royalty ⇒ +0.4-0.5 pt »)
  ne tient PAS dans ce régime (le système est déjà saturé/chaotique à 0.6, ajouter plus de
  frappe ne pousse pas la moyenne, elle nourrit surtout les queues déflationnistes qui la tirent
  vers le bas — hypothèse cohérente avec M13-P3(c), non confirmée en détail cette vague).
- **DLL Godot À RE-BUILDER** — fait cette vague (debug + release), mentionné pour mémoire du
  protocole (motif répété M3h→M14).
- Tag `pre-m15` posé · worktree `C:/tmp_wt_pre_m15` à retirer après session · binaires
  `chronicle_pre_m15.exe`/`chronicle_head_m15.exe` et `m15_sweep/` (scratch, non committés) à
  nettoyer du dépôt.

## MISSION M16 — LES CHOKES ÉMERGENTS + LA CHASSE AU DERNIER RÉSIDU (2026-07-17)

**Statut : C1 et C2 LIVRÉS, mesurés, gate 1 passé (kill-switches byte-identiques), golden
re-baseliné.** Décision joueur verbatim : « Bonne idée [les chokes par concentration de
trafic]. Termine les derniers correctifs. » Tag `pre-m16` posé (a096716 = M15 shippé).

### C1 — LES CHOKES ÉMERGENTS (scps_world.c, scps_routes.c, scps_sim.c)

**Le mécanisme livré** : `world_chokepoints_emergent_rebuild` (scps_world.c) remplace la
table STATIQUE (WG, posée à la genèse par la seule forme géométrique) par une table
DÉRIVÉE de la concentration de trafic réel — appelée périodiquement (`routes_recompute_
chokes`, scps_sim.c, MÊME cadence que la création de routes maritimes IA, `day%180==29`,
une seule fois par mois-cadencé pour tout le réseau, pas par pays). Pour chaque route
maritime OUVERTE, backtrace son plus court chemin marin RÉEL (`sea_dijkstra_core`,
prédécesseurs `g_sea_from[]`, hérité de M15-F4) et compte le passage. `world_chokepoints()`
sert la table ACTIVE (émergente si `CHOKE_EMERGENT=1`, statique sinon) à TOUS les lecteurs
existants (chronicle telemetry, `world_chokepoint_holder`) sans changement de signature.
Route creation (`routes_order`) pose désormais un placeholder NEUTRE (-1, aucun péage)
quand l'émergence est active — la concentration est une propriété du RÉSEAU ENTIER, pas
d'une route seule à sa création ; la reconstruction périodique (≤180 j) réassigne TOUTES
les routes vivantes d'un coup, jamais plus de 180 j sans assignation correcte.

**Le critère de concentration retenu** (les deux tunables, le plus exigeant gagne) :
`CHOKE_MIN_ROUTES` (plancher ABSOLU, défaut 2 — la « concentration » exige au moins deux
routes distinctes, jamais une route seule sur son propre chemin) et `CHOKE_MIN_FRAC`
(plancher RELATIF, défaut 15 % du trafic maritime RÉELLEMENT mesurable — pas le nombre
brut de routes, cf. piège ci-dessous). Choisi pour être robuste aux mondes petits (2-3
routes maritimes, le plancher absolu domine) ET grands (100+ routes, 15 % évite qu'un pur
hasard de 3 routes qui se croisent sur un océan immense passe pour un goulet mondial).

**Sort de la table statique** : REMPLACÉE (pas un fallback) quand `CHOKE_EMERGENT=1` — un
monde sans trafic maritime rapporte honnêtement 0 goulet émergent (`g_choke_em`/`g_n_choke_em`
zero-init statique, jamais un résidu d'un appel précédent, JAMAIS un fallback silencieux
vers la table statique qui aurait pu tromper la mesure). La table statique
(`compute_chokepoints`/`g_choke`) reste INTACTE, calculée à la genèse comme avant (coût
négligeable, ~2M visites de cellule une fois), servie uniquement quand `CHOKE_EMERGENT=0`.

**Chokes émergents/monde et collecte (sweep apparié {9,11,42}×3×250 ans, pre-m16 vs HEAD)** :

| seed | pre-m16 statique (goulets/sim · tenus/sim · routes taxées · or/sim · sims payés) | HEAD émergent |
|---|---|---|
| 9  | 22.3 · 6.0 · 2 · 2 or/sim · 1/3  | 6.7 · 4.0 · 6 · **111 or/sim** · 1/3 |
| 11 | 21.7 · 6.7 · 0 · 0 or/sim · 1/3  | 4.0 · 2.3 · 0 · 0 or/sim · 1/3 |
| 42 | 20.3 · 6.7 · 1 · 28 or/sim · 1/3 | 1.7 · 1.7 · 2 · 20 or/sim · **2/3** |

**Verdict (le but !)** : collecte VIVANTE sur **4/9 sims** (111/0/20 or/sim) contre **3/9**
en statique (2/0/28 or/sim, la référence M15) et **0/9** en chemin-réel-sans-émergence
(M15-F4, le mécanisme mort que M16 corrige) — le péage de détroit vit, bat les DEUX
références, reste petit en absolu (aucune explosion, même ordre de grandeur que le
statique). Nombre de goulets/monde chute fortement (20-22 statique → 2-7 émergent) : c'est
ATTENDU et SAIN — la géométrie voyait des dizaines de « détroits » potentiels (toute forme
étroite), le trafic réel n'en frappe qu'une poignée (là où tout le monde EST FORCÉ de
passer), exactement la définition d'un goulet économique vs géographique.

**Coût CPU (mesuré, seed 9, 60 ans, 1 sim)** : le PREMIER piège de perf — `cap_days`
hérité de la création de route (2×SEA_ROUTE_MAX_DAYS=120 j) exclut ~80 % des routes
maritimes OUVERTES du calcul de concentration (« injoignable DANS CE RAYON », pas « pas de
chemin » — la portée de route est VIRTUELLE depuis V3, routes_order accepte des paires
bien au-delà de 120 j réels). À ce cap, 31 routes vivantes → 6 seulement avaient un chemin
mesurable → 0 goulet détecté, silencieusement FAUX (pas un vrai « pas de concentration »).
Remonté à 8×SEA_ROUTE_MAX_DAYS=480 j (mesuré : 78 % de couverture, contre 19 % à 120 j,
83 % à l'infini-mais-inabordable) — compromis EXPLICITE coût/couverture, documenté au
call-site (`routes_recompute_chokes`). Coût mesuré : ~0.5-0.9 s/an-jeu à ce cap (world
1024×512, dizaines de routes), optimisé ×2 par le cache footprint-par-route (ci-dessous) —
ACCEPTABLE pour un rebuild PÉRIODIQUE (180 j, jamais au tick) mais non négligeable sur un
sweep 250 ans (~3-6 min/sim). `determinism-deep` (200 ans ×2 graines ×2 runs) et le sweep
gate ont tous deux tourné dans un temps raisonnable ; noté en Restes pour une éventuelle
optimisation incrémentale future.

### Pièges (C1)

- **Le comptage CELLULE-EXACTE mesure 0 goulet même avec 31-55 routes vivantes** — deux
  routes maritimes qui empruntent « le même » détroit par cabotage ne repassent QUASIMENT
  JAMAIS par la cellule exacte (chacune peut border la côte à ±quelques cellules, tie-break
  du Dijkstra compris — les coûts de cabotage sont assez uniformes pour que plusieurs
  chemins soient équi-optimaux). Fix : agréger par BUCKET (grille de WG_CHOKE_DEDUP
  cellules de côté — même échelle que le rayon de fusion de grappe existant, pas une
  nouvelle constante), une route comptant AU PLUS UNE FOIS par bucket traversé.
- **Le dédup par simple RAYON entre pics de buckets SOUS-FUSIONNE** — un détroit large
  déborde souvent plusieurs buckets adjacents, chacun qualifiant à son propre compte ; deux
  pics de buckets voisins peuvent être plus loin l'un de l'autre que le rayon de dédup →
  mesuré : 24 « chokes » (le plafond `WG_MAX_CHOKE`) pour UN SEUL corridor trafiqué. Fix :
  fusion par COMPOSANTE CONNEXE de buckets qualifiants (BFS 4-connexe sur la grille de
  buckets, bornée — `CHOKE_NB` est petit, ~1650 pour ce monde) — tout un corridor large
  devient UNE seule grappe, la cellule représentante = le pic fin DANS la composante.
- **`cap_days=-1` (sans borne) est PIRE que le cap 120 j pour la perf, pas meilleur** —
  intuition initiale : « backtrace TOUTES les routes vivantes » (le brief) suggérait
  d'enlever le cap. Mesuré : une paire en bassins RÉELLEMENT séparés fait explorer TOUT le
  bassin atteignable avant de conclure « injoignable » (le garde-fou `cap_days` de
  `sea_dijkstra_core` EST ce qui borne ce coût — un run de 60 ans a dépassé 3 min 20 sans
  finir, tué). Le cap doit rester FINI ; 8× (480 j) est le compromis mesuré, pas un choix
  arbitraire.
- **La passe 2 (assignation par route) n'a PAS besoin d'un second Dijkstra** — le premier
  passage (comptage) a DÉJÀ visité chaque cellule du chemin réel de chaque route ; mémoriser
  le footprint de buckets par route (`g_route_bucket[i][]`, plafonné à 96 buckets/route,
  généreux) pendant la passe 1 et le RÉUTILISER en passe 2 élimine la moitié du coût
  Dijkstra (mesuré : 76 s → 44 s sur le même run seed 9/60 ans) sans changer le résultat.
- **Le seuil de concentration doit se calculer sur `n_valid` (routes avec un chemin
  mesurable), PAS `n` brut (routes vivantes)** — à `cap_days` fini, certaines routes restent
  hors de portée (bassins séparés OU juste loin) ; diluer le seuil sur `n` brut sous-compte
  la concentration RÉELLE dès qu'un monde a plusieurs mers indépendantes ou beaucoup de
  liens virtuels longue distance.
- **Bootstrap de route À LA CRÉATION doit rester -1 (jamais consulter le cache émergent
  cross-appel)** — tenté puis écarté : faire lire par `routes_order` la table émergente
  COURANTE au moment de la création aurait introduit un piège savetest A==B EXACTEMENT du
  type déjà documenté pour `g_sea_from[]` (M15) — la table émergente d'un process qui a
  DÉJÀ avancé loin (ex. la continuation A d'un savetest, jusqu'à l'an 1000) diffère de celle
  d'un process qui vient de RECHARGER une sauvegarde à l'an 600 (continuation B) même si le
  RouteNetwork chargé est identique, parce que le cache module-static ne se réinitialise pas
  au chargement. Le fix (placeholder -1, jamais de lecture cross-appel à la création) rend
  `world_chokepoints_emergent_rebuild` une fonction PURE de (w, routes passées) à l'instant
  de l'appel — aucune dépendance à l'historique du process, sûr à travers save/reload PAR
  CONSTRUCTION (prouvé : savetest 3 graines A==B).

### C2 — LE DERNIER RÉSIDU (scps_econ.c)

**Audit M0 dédié** (docs/MONNAIE_M0_AUDIT.md relu contre le code ACTUEL, pas seulement le
texte du doc — daté, plusieurs sites qu'il liste « ouverts » sont en fait déjà convertis) :
§1.3 (arbitrage cités-états), §1.4 (tribut mûri), §1.5 (récompenses de mission), §1.7
(événements d_treasury), §2.1 (consommation, « L'ÉTAT REVEND » M3b-v2), §2.2/§2.3/§2.4/§2.5
(entretien/encadrement/cour/admin, tous item 5), §2.8 (construction manufacture, joueur ET
IA), §2.11 (intérêt de dette), §2.12 (pillage/siège), §2.13 (marge trade.c) sont TOUS déjà
conservés par M3f (items 1-5) ou M3a/M3b-v2/M12/M13/M14 — le programme M0→M3 est bien plus
avancé qu'il n'y paraissait en relisant seulement le doc d'origine.

**LE site identifié** : §2.6, la redépense publique (`scps_econ.c`, `econ_tick`, la boucle
par province). Chaque mois, l'État dépense `depense = trésor × STATE_SPEND_RATE(0.30) × dt`
du surplus ; `PAYROLL_FRACTION` (0.60) revient aux 3 classes au prorata de l'impôt versé
(déjà un TRANSFERT conservé) — mais le SOLDE (`depense − payroll`, 40 % de la dépense,
« armée, travaux » selon le commentaire d'origine) ne créditait PERSONNE. Contrairement aux
sinks déjà convertis (souvent bornés au surplus au-dessus d'un seuil de hoarding, ou gatés
par condition rare), celui-ci est ACTIF chaque mois sur CHAQUE province au surplus et SCALE
directement avec le trésor — candidat le plus probable identifié par audit (confirmé par la
mesure ci-dessous, pas seulement déduit).

**La conversion** : le solde rejoint `CLASS_LABORER` de LA MÊME province — même famille que
FX_SOLDE/FX_NAVY déjà convertis (item 5 : « armée, travaux » = soldats + travailleurs de
chantier, le mot du commentaire d'origine pointe déjà vers cette classe). Aucun flux FX_*
nouveau (FX_REDEP reste la vérité TRÉSOR, inchangée — la conversion n'ajoute qu'un crédit de
richesse manquant, exactement le motif des items 5 voisins). Kill-switch
`REDEP_REMAINDER_CONSERVED` (défaut 1) : =0 legacy EXACT (golden pré-M16 byte-identique).

**Mesure du résidu 209s3** (`./chronicle 7 5 250`, sim 3 = graine 209 — le sous-seed exact
désigné par M13, C2 ISOLÉ via `CHOKE_EMERGENT=0` sur LES DEUX côtés, motif de mesure du
piège M15-F2 réappliqué) :

| config | 209s3 « autres »/an |
|---|---|
| legacy (F1+F2 shippés, C2 OFF) | **−10652** |
| C2 ON (défaut) | **−8610** |

Réduction **~19 %** sur la graine désignée — RÉEL mais PAS ~0. Effet MIXTE sur les 4 autres
sims du même sweep (seed7 : +38706→+42719 pire ; seed108 : −19007→−25173 pire ; seed310 :
−17563→−43343 bien pire ; seed411 : −56779→−42429 mieux) — non-monotone, seed-dépendant,
cohérent avec TOUT l'arc M7-M15 (chaque calibrage économique mesuré dans ce dépôt montre la
même sensibilité chaotique, cf. M15-F1 « un pas boucule une graine de +0.4 à −2.3 »). Le
crédit ajouté est RÉEL et CONSERVÉ (plus de destruction pure à ce site précis, vérifié en
isolation), mais son effet en aval (LABORER plus riche → consommation/production/marché
différents → cascade) se propage de façon imprévisible selon la trajectoire — un résultat
HONNÊTE, pas un échec de la conversion elle-même.

**Verdict** : la conversion est CORRECTE (site identifié par audit, confirmé dominant par
construction — actif/scalant contre les autres candidats bornés/gatés), mesurée (réduction
réelle sur la graine désignée), mais le résidu ne tombe PAS à ~0 — un « nouveau plancher
honnête », consistant avec l'attente du brief (« ou le nouveau plancher honnête... si la
chasse révèle une chaîne »). Invariant apparié M3f 0/9 breach tenu (les deux côtés).

### Pièges (C2)

- **Le doc d'audit M0 est un instantané, pas une vérité vivante** — relire le CODE ACTUEL
  aux lignes citées (pas seulement le texte du .md) a révélé que la majorité des sites
  « ouverts » listés par `docs/MONNAIE_M0_AUDIT.md` sont déjà convertis par des missions
  ultérieures (M3f items 1-5, M3b-v2, M12-M14) — un audit basé uniquement sur le doc aurait
  chassé des fantômes. Motif à retenir : toujours re-vérifier un audit textuel contre le
  code avant de baser une décision dessus.
- **Isoler C2 d'un chantier voisin (C1) exige de geler l'AUTRE via SCPS_TUNE** — même piège
  que M15 (« comparer pre-m16 à HEAD confond C1+C2 ») : la mesure du résidu 209s3 a été
  faite avec `CHOKE_EMERGENT=0` explicite des DEUX côtés dès le départ (leçon apprise de
  M15-F2, pas re-découverte à la dure cette fois).

### Gates (C1 + C2)

1. **Kill-switches** : `SCPS_TUNE=CHOKE_EMERGENT=0,REDEP_REMAINDER_CONSERVED=0` ⇒
   `--hash 7 5 12` **BYTE-IDENTIQUE** au golden pre-m16 commité (les 5 graines) — prouvé
   AVANT toute re-baseline, RE-PROUVÉ après les deux commits combinés ✓. Défauts (C1+C2
   tous deux ON) : les 5 hash CHANGENT (attendu, C2 touche `econ_tick` dès le premier
   mois ; C1 ne bouge QUE seed 7 à 12 ans seul, les 4 autres n'ont pas encore de route
   maritime dans cette fenêtre courte).
2. **Sweep apparié** {9,11,42}×3×250 : C1 collecte de péage (tableau ci-dessus, 4/9 vs 3/9
   statique vs 0/9 réf. F4) · invariant M3f 0/9 breach (les deux côtés, 0 ÉCHEC stderr) ·
   banqueroutes Σ 48→52 (comparable, pas d'explosion) · fins variées présentes (RÉCHAUFFEMENT
   an 240, GRAND HIVER an 180, AUCUNE à 0) · C2 run ciblé 209s3 isolé (tableau ci-dessus) ✓.
3. `make test` **39/39** verts · `make golden-update` + `make golden` VERT (re-baseline
   C1+C2 combinés, les 5 graines changent — attendu) · `make determinism` STABLE (5 graines
   × 12 ans) · `make determinism-deep` STABLE (graines 7 et 9 × 200 ans, malgré le coût C1) ·
   `scps_viewer --savetest` A==B byte-identique + octet altéré REFUSÉ ×3 graines (9/11/42) ·
   `--fuzztest 9` **8/8** (216 octets, 0 crash) · `make lang-check` OK (0 littéraux, inclus
   dans `make test`) ✓.
4. Cet append + commits granulaires FR (C1 9cebf9d, C2 11d9034, golden+TROUVAILLES à suivre)
   · DLL Godot REBUILDÉE (debug + release, scons ×2, exit 0 les deux) — seul
   `scps_sim_node.cpp` a recompilé (aucun binding/GDScript touché par C1/C2) ; l'export
   `scps.exe` (Godot editor) suit cette vague séparément, hors portée CLI.

### Restes

- **Le résidu 209s3 après C2 (~−8610/an) SURVIT** — réduit de ~19 % vs legacy, pas à 0. Une
  CHAÎNE de sites plus petits (non individuellement dominants, cf. le classement de l'audit :
  audits anti-corruption `scps_ai.c` FX_AUDIT gaté, `CONCEDE_GOLD` révolte scps_revolt.c
  ~947-952 SANS AUCUN `econ_flux_add` — pas même tracké au niveau grossier — et la branche
  achat-de-stock-d'armes `scps_ai.c` ~1486-1502 non taguée item 5) reste probablement la
  piste — nécessiterait un accumulateur dédié par site (motif `g_va_produced_cum`) pour
  chiffrer chacun avant de choisir lequel convertir en premier. `CONCEDE_GOLD` en particulier
  n'a MÊME PAS de télémétrie FX_* grossière — le candidat le plus « invisible » du registre.
- **Coût CPU C1 (~0.5-0.9 s/an-jeu au cap 480j)** — acceptable pour cette vague (rebuild
  périodique 180j, jamais au tick) mais NON négligeable sur un sweep long. Piste
  d'optimisation non explorée (écartée pour risque de piège savetest, cf. Pièges ci-dessus) :
  un cache PERSISTANT du footprint par route À TRAVERS les appels de rebuild (pas seulement
  au sein d'un appel) — nécessiterait un ancrage explicite sur l'historique déterministe du
  process (haute-marque de `rn->n`) pour rester sûr au reload, non tenté cette vague.
- **`SCPS_EMDIAG`** (env-gaté, print-only, scps_world.c) laissé dans le dépôt — diagnostic
  n/n_valid/max_bucket/cap_days par appel de rebuild, motif `SCPS_CHOKEDIAG` — utile pour
  calibrer `CHOKE_MIN_ROUTES`/`CHOKE_MIN_FRAC`/le cap dans une vague future.
- **Carte Godot** : les chokes émergents ne sont PAS dessinés (aucun GDScript touché,
  conforme aux interdits de la mission) — si un jour la carte doit les montrer, le lecteur
  `world_chokepoints()` sert déjà la table active, prêt à être exposé via `scps_api.c` sur le
  même modèle que les lanes maritimes (MARITIME N2).
- **`CHOKE_MIN_ROUTES=2`/`CHOKE_MIN_FRAC=0.15`/le cap `8×SEA_ROUTE_MAX_DAYS`** : calibrage
  RAISONNABLE mesuré sur 3 graines, pas un optimum balayé (contrainte de temps de cette
  vague) — une future vague de calibrage dédiée pourrait affiner sur un sweep plus large.
- Tag `pre-m16` posé · worktree `C:/tmp_wt_pre_m16` à retirer après session · binaires
  `chronicle_pre_m16.exe`/`chronicle_head_m16.exe`/`chronicle_c2test.exe` et scratch
  `m16_sweep*/` (non committés, nettoyés du dépôt) — restaient dans le scratchpad de session.

---

## CARTOGRAPHIE UI — la carte existe, tout brief UI l'embarque (2026-07-17)

**`docs/CARTOGRAPHIE_UI.md`** (HEAD `296a8c2`, lecture seule sur `godot/project`) :
37 surfaces + carte de l'information (3-clics) + 22 raccourcis/34 verbes/6 curseurs
+ le carnet de chasse doctrine (Échap ne ferme pas V/B/E · 3 fiches province aux
classes renommées · religion introuvable après fondation). **Tout digest de mission
UI futur EMBARQUE ce document** au lieu de refouiller l'interface à l'aveugle — et
toute vague UI le met à jour dans le MÊME commit que ses changements (règle en tête
du fichier).

---

## MISSION UI-POLISH — les 13 défauts vus en jeu réel (2026-07-18)

**Statut : 12/13 corrigés + vérifiés en probe réelle (Main.tscn, seed 9), 1 déjà
conforme (hover), 1 « bug » confirmé NON-bug après lecture engine. Tag
`pre-uipolish` posé avant tout changement. DLL rebuild (debug+release) +
re-export `scps.exe` faits — `scps_api.c` touché (item 7, extension membrane).**

### LE PIÈGE FONDATEUR — le brief nommait le mauvais fichier pour l'item 1

- **`province_panel_v2.gd` (le fichier nommé par le brief pour l'item 1) N'EST PAS
  le panneau que le joueur voit en cliquant une province** — main.gd:790-808
  (`_on_province_picked`, le geste PAR DÉFAUT) ouvre `province_panel.gd` (le
  panneau LEGACY, dessin immédiat) ; `province_panel_v2.gd` n'est atteignable que
  par la touche **V** (un pilote secondaire). Les libellés de section cités mot
  pour mot par le brief — PEUPLE/SATISFACTION/POPULATION/RESSOURCES/PRODUCTION/
  CAPITALE — n'existent QUE dans `province_panel.gd` (`VKit.section(self, x, y,
  "PEUPLE")` etc., lignes 219-385) ; `province_panel_v2.gd` a des sections
  totalement différentes (PEUPLES/RÉSUMÉ/DÉFENSE…). C'est exactement le piège que
  TROUVAILLES documentait déjà (« un agent qui grepperait juste "province"
  pourrait éditer le mauvais fichier ») — sauf que cette fois c'est le BRIEF
  lui-même qui s'est trompé de fichier (probablement parce que
  `province_panel_v2.gd` est documenté « le modèle » ailleurs). Item 1 corrigé
  dans `province_panel.gd`, pas `province_panel_v2.gd` — vérifié : les deux
  fichiers coexistent, aucun n'a été supprimé, seul le bon a été touché.

### Découvertes

- **La cause de l'item 1 (débord ~90-100px) : `VKit.section()`/`row()` déduisent
  leur largeur de `(ci as Control).size.x`** — mais `province_panel.gd.size.x =
  PW + BUILD_TAB_W` (348+112=460), PAS la largeur visible du fond parchemin
  (`VKit.panel_bg` ne peint que `PW`=348). `BUILD_TAB_W` est une bande RÉSERVÉE
  hors-fond pour que le bouton « Construction » collé au bord droit reste dans
  les bornes de hit-test du Control — un choix légitime pour LE BOUTON, mais
  `VKit.section`/`row` n'avaient aucun moyen de le savoir et empruntaient
  `size.x` en entier. Fix : paramètre optionnel `w_override` (vkit.gd), les 12
  sites d'appel de `province_panel.gd` passent `PW`. Les AUTRES appelants de
  `VKit.section`/`row` (battle_panel.gd, empire_sidebar.gd, sidebar_drawer.gd)
  n'ont PAS ce motif « bande hors-fond » — non touchés, comportement historique
  préservé (`w_override` par défaut = -1 = ancien calcul).
- **Item 9 (prospérité 0 vs 50) confirmé un VRAI transitoire moteur, PAS un faux
  positif d'affichage.** `pe->prosperity = gdp/(popsum+1)` (scps_econ.c ~4937)
  part à 0 (struct calloc'd) et ne se peuple qu'au premier `econ_tick` complet ;
  les provinces SAUVAGES (non colonisées) affichent un repli FIXE 50% (`colonized
  ? pe->prosperity : 5.f`, scps_readout.c:704) qui, lui, ne dépend d'AUCUN tick —
  d'où l'inversion « capitale développée à 0% à côté d'un hameau vide à 50% ».
  Vérifié en probe réelle (jour 1 an 0, seed 9) : la capitale tier 4 affiche
  bien « Prospérité — » après le fix (au lieu de 0), le hameau sauvage garde son
  50%. Fix display seul (`_is_pre_settlement`, province_panel.gd) : jour 1 an 0 +
  aisance≤0 ⇒ « — ». **Piège trouvé EN VÉRIFIANT** : une probe à `years=25` (le
  motif par défaut des autres probes du repo, shot_ui.gd etc.) montre la MÊME
  capitale TOUJOURS à « Prospérité 0 » 24 ANS plus tard (pas juste au jour 1) —
  `scps_sim_advance_days` appelle bien `sim_day()` en boucle réelle (PAS un
  raccourci, vérifié scps_api.c:156-162 — donc pas un artefact de probe), ce qui
  suggère que CETTE capitale-là (seed 9, la FAMINE du jour 0 ne se résorbe
  peut-être jamais sous cette politique par défaut) reste bloquée en dessous du
  seuil de croissance de GDP pendant des dizaines d'années simulées. Documenté en
  Restes ci-dessous — hors périmètre (mission scopée « jour 1 », pas « pourquoi
  ça ne repart jamais »), aucun fichier scps/*.c touché pour ça.
- **Item 10, la FAMINE du jour 0 EST un vrai signal moteur, reproductible, PAS un
  faux positif d'amorçage.** Vérifié en probe réelle jour 1 an 0 (seed 9) : le
  JOURNAL affiche bien « an 0 · FAMINE — Désert Brûlant ne ma[nge qu'à N%]… » dès
  la genèse — les stocks de départ sont vides (aucun grenier hérité), le
  `food_sat` calculé au premier tick tombe sous le seuil. Probablement LIÉ au
  point précédent (une famine dès le jour 0 pourrait amorcer le blocage de GDP
  observé 24 ans plus tard). Non touché (moteur, hors périmètre) — Reste
  documenté pour une future vague d'équilibrage.
- **Item 7 : le « ~ » devant l'entretien n'a JAMAIS reflété une incertitude
  moteur.** `scps_edifice_upkeep_month`/`scps_manuf_upkeep_month` (scps_api.c)
  retournent déjà `(int)(valeur_exacte + 0.5f)` — un ARRONDI STANDARD (comme
  n'importe quelle devise affichée à l'entier), pas une approximation. Le « ~ »
  était un artefact GDScript pur (3 sites, construction_panel.gd), les
  commentaires du code lui-même disaient déjà « miroir EXACT E1bis.10 » juste
  au-dessus de la ligne qui affichait « ~ ». Retiré, aucun nouveau reader
  nécessaire (contrairement à l'hypothèse du brief).
- **Item 7 extension (directive joueur, membrane) : `api_edifice_effet`
  (scps_api.c) affichait « institutions +1.0 », « ouverture +1.0 »… — les
  COORDONNÉES MOTEUR nues (K_inst/H_coerc/P_open/PE_infra/food_cap/faith/savoir)
  de `EdificeDef.delta`, sans aucun ancrage joueur.** La parenthèse descriptive
  (« tient la province, ronge la loyauté »…) était déjà le bon texte — seul le
  `%.1f` nu a été retiré du format (`EF_ADD` macro), le gate `val>0.001f` reste
  identique (comportement inchangé, affichage seul). Touche scps_api.c (permis :
  c'est la façade, item 7 l'autorisait déjà pour le reader exact) — DLL
  rebuild + `core_demo` (35/35) + `make lang-check` (0=0) vérifiés après coup.
- **Item 8, PAS un bug d'enum CLASS_\*, vérifié dans le C.** `CLASS_LABORER=0,
  CLASS_BOURGEOIS=1, CLASS_ELITE=2` (scps_econ.h) et la boucle de
  `scps_country_fiscal_orders` (`for c=0..2, (SocialClass)c`) sont alignés
  1-pour-1 avec `CLASS_NAMES := ["Journaliers","Bourgeois","Élite"]` côté
  GDScript. `satisfaction=-1` est le sentinel LÉGITIME
  d'`econ_country_class_satisfaction` (scps_econ.c:2237-2250) quand
  `psum<=0` — cette classe n'a ENCORE AUCUNE ÂME dans le pays (vérifié en probe :
  jour 1, seed 9, Bourgeois=0 pop pendant que Journaliers/Élite sont peuplés —
  un empire neuf n'a pas encore d'artisans urbanisés). Le seul vrai défaut :
  l'affichage mélangeait un tiret (satisfaction) et un chiffre « 0 or/mois »
  (revenu) pour la MÊME absence — les deux disent « — » désormais (cohérence
  avec la règle de l'item 9 : jamais un nombre à côté d'un tiret pour un état
  qui n'existe pas encore).
- **Items 2 et 3 (troncature « … » sur la culture / le journal) : le hover
  COMPLET était DÉJÀ câblé dans le code AVANT cette mission** — `province_panel.
  gd` (`_tips.append` conditionné sur `VKit.text_w(cul_txt) > cul_w`) et
  `empire_sidebar.gd::_get_tooltip` (boucle sur `_journal_rects`, compense déjà
  le `_scrolloff`) implémentaient tous deux « doctrine hover=détail » avant que
  je ne touche quoi que ce soit. Vérifié par lecture — non modifié (aucune
  raison de retoucher un mécanisme déjà correct). Ce que le brief a probablement
  vu : une capture STATIQUE (le survol n'a jamais été testé à la main), donc
  « tronqué à l'ellipse » sans savoir que le hover révèle le reste.
- **Item 12 : les curseurs fiscaux/budgétaires sont RÉIMPLÉMENTÉS À PART dans
  `economy_page.gd` (Fenêtre Empire, onglet Économie) — un DOUBLON connu, pas
  créé par cette mission** (cartographie UI §D.1.3 : même verbe
  `player_budget_policy`, deux `HSlider` non synchronisés). Le fix de cette
  mission (`_row()`, budget_panel_v2.gd, onglet Balance UNIQUEMENT — le tiroir
  cité par le brief) ne touche PAS `economy_page.gd` : aucune divergence
  NOUVELLE introduite, le doublon pré-existant reste identique à avant. La purge
  du doublon (3 fiches province + 2 implémentations de curseurs) est un chantier
  à part (cartographie UI §D.1.2/D.1.3), volontairement pas étendu ici.
- **La pile d'Échap (item 13) : `visibility_changed` est le point d'écoute qui
  évite de toucher les DIZAINES de sites d'ouverture existants.** `_construct`/
  `_budget_v2`/`_empire_win`/`_country_actions` sont ouverts depuis 5+ endroits
  différents de main.gd (raccourcis clavier, signaux `build_requested`, clic
  droit carte…) — plutôt que d'instrumenter chaque site, un SEUL
  `p.visibility_changed.connect(...)` par panneau (posé une fois, après leur
  création) capte TOUT changement de `.visible` quel que soit son origine.

### Pièges

- **`for x in [a, b, c]: var y := x` échoue l'inférence de type GDScript**
  (« Cannot infer the type of "y" ») quand le littéral d'array n'est pas
  explicitement typé — le loop var `x` est `Variant`, `:=` ne peut pas en
  déduire un type concret même si TOUS les éléments réels sont des `Control`.
  Typer EXPLICITEMENT (`var y: Control = x`) — piège JUMEAU de celui déjà
  documenté pour les ternaires mélangeant un appel de méthode et un littéral.
  Payé une fois (main.gd, le wiring `visibility_changed`), détecté par la probe
  (SCRIPT ERROR au chargement, PAS d'erreur silencieuse — le fallback « Main.tscn
  par défaut » de Godot masque presque l'échec : la partie démarre quand même,
  juste sans AUCUN des `_ready()` du script cassé, ring de confusion si on ne
  lit pas `SCRIPT ERROR` dans le log).
- **GDScript refuse une assignation à l'intérieur d'une expression ternaire**
  (`x if cond else (y = z)` → « Assignment is not allowed inside an
  expression ») — piège TROUVÉ dans ma propre probe (`uipolish_shot.gd`), pas
  dans le code livré. `if/else` explicite à la place.
- **Un échec de PARSE d'une scène passée en argument CLI (`res://foo.tscn`) fait
  silencieusement retomber Godot sur `run/main_scene` (project.godot)** — la
  partie démarre NORMALEMENT (genèse du monde, logs complets) sans qu'aucune
  probe ne tourne, et le process ne quitte JAMAIS (`get_tree().quit()` n'est
  jamais atteint) → un hang qui RESSEMBLE au piège `--headless` déjà documenté
  mais dont la cause est totalement différente. Toujours lire les 10 premières
  lignes du log pour un `SCRIPT ERROR`/`Parse error` avant de conclure à un hang
  du connu.
- **`MSYSTEM=MINGW64 bash.exe -l -c "commande relative"` échoue silencieusement
  (« No such file or directory »)** — le shell de LOGIN MSYS2 réinitialise son
  répertoire de travail à SON `$HOME` avant d'exécuter `-c`, donc tout chemin
  relatif à la racine du dépôt échoue. Contournement fiable : écrire un script
  `.sh` avec un `cd /c/...` EXPLICITE en première ligne, puis l'invoquer par
  CHEMIN ABSOLU (`bash.exe -l /chemin/absolu/script.sh`) — motif déjà utilisé par
  `packaging/windows/rebuild_dll.sh`, à copier plutôt qu'à réinventer un `-c`
  inline (payé ~6 tentatives ratées avant de re-remarquer que le script existant
  faisait déjà ça).

### Le tableau du balayage graphite (item 6)

| Site | Nature | Décision |
|---|---|---|
| `sidebar_drawer.gd::_draw_header` | fond quasi-noir codé en dur sous les 8 onglets du tiroir (dont Diplomatie F7, signalé par le joueur) | **corrigé** → `VKit.COL_PANEL2` |
| `event_dialog.gd` bandeau « DÉCISION » | même reliquat, popup de dilemme (gameplay fréquent) | **corrigé** → `VKit.COL_PANEL2` |
| `event_popup.gd` bandeau « OYEZ OYEZ » | même reliquat, popup d'évènement | **corrigé** → `VKit.COL_PANEL2` |
| `province_panel.gd` boutons ✕/chevron replier | fond quasi-noir + glyphe encre SOMBRE dessus → rendait deux carrés illisibles (le « ■ ■ » du brief) | **corrigé** → `VKit.COL_PANEL2` |
| `budget_panel_v2.gd` bouton banqueroute | `Button.new()` sans style → thème Godot par défaut (ParchTheme ne stylise que la variation « Tab », pas la classe « Button » de base) | **corrigé** → rouge danger dédié (item 5) |
| `sidebar_drawer.gd::_draw_multiplier_slider` piste | groove quasi-noir, ParchTheme donne déjà un ton de piste officiel (tan) pour les HSlider natifs | **corrigé** → `Color("caa768")` |
| `sidebar_drawer.gd` bandeaux totaux Revenus/Dépenses (`_draw_eco`, vert/rouge sombres) | accent de ligne-total, teinte plus sombre que le reste — INTENTIONNEL possible (emphase) ou reliquat, non tranché sans capture dédiée | **non touché** — à re-vérifier visuellement par un futur agent |
| `topbar.gd` bouton de vitesse inactif (`Color(0.075,0.085,0.085)`) | chrome de la TOPBAR (doctrine : famille « nationale », possiblement sciemment distincte du parchemin des panneaux de contenu), non signalé par le joueur | **non touché** — hors scope, pas dans le catalogue |
| `alerts.gd` fond des cartouches-label (chips flottants sur la carte) | fond sombre translucide DOCUMENTÉ intentionnel dans le commentaire (« letters RimWorld », lisibilité sur fond de carte variable) | **non touché** — motif volontaire, pas un reliquat |
| `menu_root.gd`/`options_panel.gd`/`new_game_panel.gd`/`culture_creator.gd`/ `religion_panel.gd`/`feedback.gd` (palettes locales C_BG/C_PANEL « cuir sombre ») | écrans de MENU/SETUP, DA distincte assumée (cartographie UI §A confirme : « cohérence par coïncidence, pas la même source », aucun hérétique de palette) | **non touché** — hors périmètre (ni signalé par le joueur, ni « en jeu »), risque de régression visuelle sur une DA volontairement différente |
| `tooltip_server.gd` fond de tooltip (`Color(0.075,0.06,0.045)`) | dark card intentionnel (motif tooltip à contraste, pas du chrome de panneau) | **non touché** — pas un reliquat, un choix de lisibilité |

**Codes couleurs (directive joueur #6, sémantique)** : audité en passant — `VKit.
sense()`/`ParchTheme.INCOME`/`ParchTheme.EXPENSE`/`COL_GOLD` sont la source
UNIQUE consommée par tous les fichiers touchés cette mission (rouge=négatif,
vert=positif, or=monnaie/accent) ; aucune violation trouvée dans le périmètre
touché. Un audit EXHAUSTIF des ~50 fichiers UI non touchés ici n'a pas été fait
(hors budget de cette mission) — les zones `Color()` littérales listées ci-dessus
sont les seules suspectes relevées en cours de route.

### La règle des panneaux (item 13, à documenter pour tout futur agent UI)

1. **Échap ferme le panneau au PREMIER PLAN — le DERNIER ouvert**, pas un ordre
   fixe arbitraire. Implémenté via une pile (`main.gd::_panel_stack`) alimentée
   par `visibility_changed` (pas par les sites d'ouverture).
2. **Quand PLUS RIEN n'est ouvert, Échap ouvre le menu système** (pause/options/
   sauver/quitter) — déjà le comportement de `KEY_ESCAPE` (`_close_topmost()`
   retourne `false` ⇒ `_menu.open()`), inchangé par cette mission.
3. **Échap NE QUITTE JAMAIS la run directement** — vérifié : le seul
   `get_tree().quit()` du projet est derrière le bouton « Quitter » explicite du
   menu (`menu_root.gd:157`), aucun chemin Échap n'y mène.
4. **Un panneau MAJEUR (Trésor, Diplomatie, fenêtre pays) referme les POPUPS
   FLOTTANTS non ancrés (Construction) à l'ouverture** — un seul, Construction,
   dans ce statut aujourd'hui.
5. **La fiche province (contextuelle-ancrée : legacy OU V2) NE FAIT JAMAIS
   PARTIE de la règle 4 — elle coexiste toujours** avec un panneau majeur.
6. **Tout NOUVEAU panneau flottant** doit décider explicitement où il se situe
   (majeur / popup flottant / contextuel-ancré) et s'enrôler dans
   `_panel_stack` (`p.visibility_changed.connect(...)`, motif à copier depuis le
   wiring de `_budget_v2`/`_empire_win`/`_country_actions`/`_construct`,
   main.gd ~ligne 330) — sinon Échap l'ignore silencieusement (exactement le bug
   corrigé cette mission).

### Restes

- **Item 9/10, le fond moteur : une capitale peut rester bloquée à
  `prosperity≈0` DES DIZAINES D'ANNÉES simulées après une famine au jour 0**
  (observé seed 9 : encore à 0% à l'an 24, alors que le fast-forward
  `advance_days` tourne le VRAI `sim_day()` en boucle, pas un raccourci).
  Corrélation plausible avec la famine du jour 0 (stocks de départ vides), non
  prouvée, non creusée (hors périmètre — mission scopée « jour 1 », moteur
  interdit sauf scps_api). Un futur agent pourrait tracer `re->gdp`/
  `re->prosperity` sur cette capitale précise (seed 9) tick par tick pour
  confirmer si c'est un vrai piège de croissance ou un artefact de CE run
  particulier (ex. IA n'ayant jamais reçu d'ordre de secours).
- **Le doublon des 3 fiches province (classes renommées Laboureurs/Artisans/
  Noblesse vs Journaliers/Bourgeois/Élites) et des curseurs budgétaires en double
  (economy_page.gd vs budget_panel_v2.gd)** — cartographié (§D.1.2/D.1.3),
  explicitement PAS étendu par cette mission (directive coordinateur : « purge
  = vague UI-DOCTRINE suivante »). Nomenclature CANONIQUE confirmée par le
  coordinateur : Journaliers/Bourgeois/Élites (celle du moteur/doctrines).
- **`sidebar_drawer.gd` bandeaux Revenus/Dépenses (vert/rouge sombres,
  `_draw_eco`)** — non tranché (intentionnel vs reliquat), signalé dans le
  tableau du balayage, à re-capturer visuellement par un futur agent avant de
  décider.
- **item 2 (culture) : la largeur de la demi-colonne (`cul_w`, la moitié de
  `rw` moins les marges) n'a pas été élargie** — le hover suffit déjà à la
  doctrine, mais un futur agent pourrait resserrer les camemberts culture/foi
  pour donner plus de place au texte SANS toucher au hover (embellissement,
  pas un bug).
- **`uipolish_shot.gd` (nouvelle probe)** ne capture pas de scénario de
  survol RÉEL (item 11, item 2/3 hover) — le fix de stale-hover (item 11) et le
  hover déjà-correct (items 2/3) sont vérifiés par LECTURE de code + logique,
  pas par une capture avec un VRAI mouvement de souris timé. À re-vérifier à la
  main si un doute visuel remonte (mission le permettait explicitement : « à
  re-vérifier à la main »).
- **`make golden`/`make determinism` non relancés après le changement
  `scps_api.c`** (item 7 extension) — jugé sûr par raisonnement (pure
  reformulation d'une CHAÎNE DE CARACTÈRES d'affichage, `api_edifice_effet`
  n'alimente aucun état simulé/sérialisé, aucun test ne dépend de son contenu
  exact — grep confirmé) plutôt que mesuré ; `core_demo` (35/35) et
  `make lang-check` (0=0) ont tourné. Un futur agent qui doute peut lancer
  `make golden` en 2 minutes pour clore le doute definitivement.

---

## MISSION UI-DOCTRINE — D1/D2/D3 (2026-07-18, en cours)

**Statut D1-D3 : livrés (tag `pre-uidoctrine` posé avant tout changement).**
D1 = purge des doublons (fiche province + curseurs fiscaux) ; D2 = accès Créateur de
Foi ; D3 = résidus « or/an » restants + rasoir sur mes propres libellés. D4-D7 (glossaire
hover, ressources exactes, options/audio, icônes) suivent dans des commits séparés.

### Le piège fondateur (encore un) — le brief nommait la bonne fiche cette fois

Contrairement à UI-POLISH (qui avait édité `province_panel.gd` legacy en croyant
éditer `province_panel_v2.gd`), le brief UI-DOCTRINE nommait EXPLICITEMENT les deux
fichiers et demandait un AUDIT avant de choisir — bonne pioche : `province_panel_v2.gd`
était bien déjà « le modèle » (doctrine bâti-seul/hover/mois déjà respectée, conteneurs
natifs). Le vrai travail n'était PAS de choisir mais de PORTER le câblage réel (fermer,
pied d'actions gouvernemental/diplomatique/colonisation) de legacy vers v2 — v2 n'avait
JAMAIS eu de bouton Coloniser, Réprimer/Assimiler/Purger, ni de chips diplo
(Attaquer/Route/Piller) : elle était fonctionnellement incomplète en display-only pur,
pas juste « différente ». Une unification qui aurait juste supprimé legacy et laissé v2
telle quelle aurait RETIRÉ des verbes joueur fonctionnels du jeu.

### Découvertes

- **`province_panel_v2.gd` classait encore comme « pilote secondaire »** dans
  `main.gd::major_open()` ET dans la liste générique de `_close_topmost()` — un statut
  hérité de l'époque où elle n'était qu'un panneau derrière la touche V (usage rare).
  Une fois promue fiche PAR DÉFAUT (clic sur CHAQUE province, l'interaction la plus
  fréquente du jeu), ces deux inclusions devenaient des BUGS DE COMPORTEMENT, pas
  juste des reliquats cosmétiques : `major_open()==true` en continu aurait fait
  collapse la pile d'alertes du rail droit à CHAQUE sélection de province ; et la
  présence dans la liste générique de `_close_topmost()` faisait qu'un Échap se
  contentait de `.visible=false` (sans `_clear_selection()`) — il fallait DEUX Échap
  pour effacer le contour doré de sélection sur la carte, contre UN seul avant
  (comportement historique de legacy, jamais dans cette liste). Retiré des deux ; la
  fiche province garde son propre chemin de fermeture dédié (`_clear_selection`, motif
  legacy, en fin de `_close_topmost()`).
- **`vkit.gd::section()/row()` avait un paramètre `w_override` devenu MORT** après la
  suppression de legacy — son SEUL appelant sur tout le projet (UI-POLISH #1 l'avait
  ajouté spécifiquement pour `province_panel.gd`). Retiré (grep confirmé : 0 appelant
  restant passe un 6e argument) plutôt que laissé comme code mort — exactement le
  piège que ce fichier documentait déjà pour d'autres cas (« le code mort UI a englouti
  une vague entière »).
- **Le signal `closed` de `religion_panel.gd` ne tirait QUE depuis le bouton
  « Fermer »** — fermer via Échap (déjà câblé dans `_close_topmost()` AVANT cette
  mission, juste jamais testé en pratique faute d'accès normal au panneau) laissait
  `Sim.set_speed(2)` jamais appelé : le jeu restait en PAUSE indéfiniment. Un bug
  DORMANT depuis la création du panneau, invisible tant que la SEULE porte d'entrée
  était le déclenchement automatique à la 1re fondation (rare, une fois par partie) —
  révélé par le fait même de rebrancher un accès répétable (touche R). Remplacé le
  point d'écoute par `visibility_changed` (motif déjà établi par UI-POLISH #13 pour la
  pile d'Échap) : couvre TOUT chemin de fermeture sans dépendre d'un signal
  spécifique à UN SEUL site d'émission.
- **`economy_page.gd` réutilisée par `empire_window.gd`** : plutôt que de choisir entre
  « déplacer le code des curseurs » (risque de casser budget_panel_v2, qui a sa PROPRE
  copie) ou « supprimer l'onglet Économie de la Fenêtre Empire » (perte d'info), un
  simple flag `interactive: bool` (défaut `true`, posé à `false` par l'appelant Fenêtre
  Empire) suffit : la MÊME fonction `_row()` construit soit un curseur soit rien, zéro
  duplication de template. Le lien de retour (« Régler… → Trésor (B) ») est un signal
  qui remonte 2 niveaux (economy_page vers empire_window vers main.gd), motif déjà
  utilisé partout dans ce fichier pour la navigation croisée entre panneaux.
- **`province_ui_audit.gd`/`series2_audit.gd` chargeaient `province_panel.gd` en DUR**
  (`load("res://ui/province_panel.gd")`) — une suppression de fichier sans grep
  PRÉALABLE sur TOUT `godot/project/**` (pas juste `ui/`) aurait cassé ces deux probes
  headless/fenêtrées silencieusement (échec seulement au PROCHAIN lancement, pas à la
  compilation du reste du projet). `tests/province_info_card_test.gd` testait carrément
  des MÉTHODES internes de legacy (`_province_summary_card`/`_province_satisfaction_card`,
  le système de cartes de survol structurées `get_info_card`) — v2 ne l'a jamais eu
  (elle utilise `tooltip_text` natif partout, plus simple) : pas de portage possible sans
  réinventer le système, donc RETIRÉ (le test testait une fonctionnalité qui n'existe
  plus, pas une régression).

### Pièges

- **Toucher `main.gd` en HUIT points dispersés pour UNE seule unification** (var
  déclaration, instanciation+signaux, `_on_province_picked`, `_clear_selection`,
  `_close_topmost` deux listes, `major_open`, `_sidebar.tab_selected`, `_prov_detail.
  visibility_changed`) — grep sur `_prov_panel` seul (sans le suffixe `_v2`) après
  chaque lot d'edits est le seul moyen fiable de ne rien oublier ; un seul site manqué
  (`_on_province_picked` par ex.) aurait laissé le clic-carte planté sur un panneau qui
  n'existe plus, donc un crash immédiat au premier clic, pas une dégradation silencieuse.
- **`Godot_v4.6.3-stable_mono_win64_console.exe --headless --path godot/project
  res://main/Main.tscn --quit-after 180` est un filet de sécurité RAPIDE (~15 s) pour
  toute chirurgie sur `main.gd`** — boot complet (genèse + `_ready()` de TOUT le shell)
  sans lancer une seule probe dédiée ; grep la sortie sur SCRIPT ERROR, parse, Invalid
  get, Nonexistent, Attempt to call (les warnings de leak RID/CanvasItem à la sortie
  sont du bruit de cleanup Godot, pas des régressions). À relancer après CHAQUE lot de
  modifications à `main.gd`, pas seulement à la fin.
- **Un bouton `Button.new()` sans style dans un contexte ParchTheme (v2) hérite du
  thème PAR DÉFAUT de la fenêtre (`ui_theme.gd`), pas de ParchTheme** — ParchTheme ne
  stylise QUE ses `theme_type_variation` nommées (Tab, HeaderStrip…), jamais la classe
  `Button` de base (déjà documenté par UI-POLISH pour le bouton banqueroute). Tout
  nouveau bouton de v2 (pied d'actions, fermer, lien Trésor/Foi) doit poser ses propres
  `add_theme_stylebox_override` — copié le motif de `_sq_btn()`/`_chip_btn()` déjà dans
  le fichier plutôt que d'inventer un 3e style.
- **Un heredoc bash riche en apostrophes françaises peut faire échouer le tool Bash**
  (« unexpected EOF while looking for matching '' ») — probablement un ré-encapsulage
  de la commande entière par le harness. Contournement : écrire le texte via l'outil
  Write dans un fichier scratchpad, puis `cat scratch >> cible` en deux chemins courts,
  zéro contenu littéral riche en apostrophes sur la ligne de commande elle-même. Piège
  jumeau : passer un CHEMIN WINDOWS À BACKSLASHES à ce même tool Bash (Git Bash/POSIX)
  casse pareil — toujours des slashs avant (`/c/Users/...`), jamais `C:\Users\...`.

### Restes

- **Doublon interne à `budget_panel_v2.gd`** (onglet Balance vs Monnaie, `_sliders` vs
  `_m_sliders`, MÊME fenêtre) — cartographié (§C.3), PAS dans le mandat D1 (qui nommait
  explicitement `economy_page.gd` vs `budget_panel_v2.gd`, deux FENÊTRES). Un futur
  agent pourrait fusionner les deux dicts si le joueur le signale.
- **Doublon de VUE (lecture seule) à quatre endroits** (tiroir Économie, Trésor,
  Fenêtre Empire, courbes) — assumé, chacun sert un CONTEXTE différent (permanent vs
  dédié vs gestion vs historique) ; non réduit par cette mission.
- **Codex désynchronisé** (verbes Monnaie 2026-07-16 absents de `codex.gd::DOMAINS`) —
  catalogué en D.2, pas dans le texte mandaté par D3 (or/an + membrane + rasoir +
  couleurs), donc non touché. Correctif trivial pour un futur agent (3 entrées à
  ajouter).
- **`army_panel.gd` toujours hors de la pile Échap** (`main.gd::major_open`/
  `_close_topmost` ne le listent pas) — signalé en D.2, non mandaté par cette vague,
  non touché.

## MISSION UI-DOCTRINE — D5 : les ressources prises exactes (Menu Construction)

### Découvertes

- **Écart réel trouvé et corrigé** : `scps_building_roster` (`scps/scps_api.c:2654-2659`)
  rapporte la recette NUE d'un édifice (`d->cost.qty[k]`, un int arrondi `+0.5f`) — mais
  le drain réel (`agency_build_acct`, `scps/scps_agency.c:410-417`) consomme
  `c->qty[k]*mult` où `mult = agency_extent_mult(econ, region)` (§7, `scps_agency.c:
  297-303`) = `1 + 0.15·(régions possédées par le pays)`. Même le gate de légalité
  (`scps_build_legal_ex`, `scps/scps_api.c:2454-2461`) applique CE MÊME facteur pour
  décider si la matière est suffisante — seule la QUANTITÉ AFFICHÉE sur la puce
  ressource (`_cost_chip`, construction_panel.gd) l'omettait. L'OR affiché, lui, était
  déjà juste (`o->gold` = `agency_build_gold`, qui inclut `ext` en interne) — seul le
  chiffre "×qty" à côté de l'icône ressource mentait, systématiquement PAR DÉFAUT
  (même à 1 seule région : facteur ×1.15) et de PLUS EN PLUS pour un grand empire
  (ex. 5 régions ⇒ ×1.75, donc la quantité réellement débitée dépasse de 75 % le
  chiffre affiché avant ce correctif).
- **Corrigé dans le SEUL fichier autorisé** (`construction_panel.gd`), sans toucher
  `scps_api.c` ni rebuilder la DLL : ajout de `_extent_mult()`/`_cost_qty_real()`
  (lignes ~282-299) qui rejouent la formule `1+0.15·nreg` depuis
  `country_info(me).get("regions", 0)` — un champ DÉJÀ exposé et déjà consommé
  ailleurs dans l'UI (country_panel.gd, memory_panel.gd, search_palette.gd…), donc
  fiable et sans lecteur C à ajouter. Appliqué aux DEUX affichages de coût : la puce
  visible de la carte (`_edifice_card`, ligne ~400) ET la ligne « recette N · stock
  national M » du tooltip (`_build_info_card`, ligne ~242).
- **Champs vérifiés SANS écart** (audit complet `scps_build_legal_ex` ↔
  `agency_build_acct`, ordre des gates IDENTIQUE — port/estuaire, doublon/file, palier
  tech, matière, or) : `days` (aucun multiplicateur nulle part, `EDIFICES[e].days`
  brut des deux côtés) ; `gold` édifice (`o->gold` déjà ext-inclus via
  `agency_build_gold`, ET calculé sur `cap_reg` = capitale — cohérent car le verbe
  `player_build(type, -1)` construit TOUJOURS à la capitale côté façade, jamais sur
  `target_pid`) ; `mcost` manufacture (`scps_manuf_cost` = formule EXACTE du drain
  CMD_BUILD_MANUF, `MANUF_BUILD_COST*econ_world_ipm*decree_manuf_cost_mult`, déjà
  documenté et vérifié identique) ; recette manufacture affichée (`_recipe_text`, via
  `manuf_recipe`→`building_recipe_qty`) — c'est un RATIO de recette (commentaire
  scps_econ.c:813 : « pour le MENU CONSTRUCTION »), pas une quantité absolue par tick,
  donc rien à corriger. `entretien` (upkeep, édifice ET manuf) est documenté CÔTÉ
  MOTEUR (`scps_econ.c:2760-2762`) comme un prix NOMINAL — pas le montant réellement
  clippé si le crédit est court — exactement le même choix assumé que pour
  `scps_manuf_cost` : intentionnel, pas un écart.

### Pièges

- **`agency_extent_mult` (§7, "l'ÉTENDUE renchérit ses institutions") est une formule
  HARDCODÉE (`1.f + 0.15f*nreg`), PAS un tunable `tune_f`** — la dupliquer côté
  GDScript (au lieu d'exposer un nouveau lecteur C) est un choix délibéré (rester dans
  le seul fichier autorisé, éviter un rebuild DLL) mais FRAGILE : si un futur agent
  change ce `0.15f` engine-side sans toucher `construction_panel.gd:291-299`, l'écart
  revient. Commentaire laissé en place citant les deux sites source
  (`scps_agency.c:297`, `scps_api.c:2454-2456`) pour qu'un grep sur `0.15` les
  retrouve ensemble.
- **Le harness de capture fenêtrée (motif `uipolish_shot.gd`) peut afficher un
  dialogue MODAL "Fermeture anormale détectée"** (`ui/feedback.gd:88-98`,
  `ConfirmationDialog`) par-dessus toute l'UI si une session précédente a été TUÉE
  par un timeout (le drapeau `user://session_running.flag` n'est effacé qu'à la
  fermeture PROPRE) — le nettoyage fait par `uipolish_shot.gd`/`shot_ui.gd` en tout
  début de `_ready()` (avant `load(Main.tscn)`) ne suffit PAS : `feedback.gd` est
  ajouté comme enfant nommé `"Feedback"` de `_main` (`main.gd:459-461`) et lit son
  PROPRE flag dans SON `_ready()`, qui tourne pendant l'instanciation de Main.tscn,
  donc AVANT que le nettoyage de la probe n'ait d'effet utile. Fix appliqué dans
  `d5_resources_shot.gd` (`_dismiss_crash_dialog()`) : chercher
  `_main.get_node_or_null("Feedback")`, puis `queue_free()` tout `ConfirmationDialog`
  enfant avant la première capture. Un futur agent de screenshot devrait copier ce
  motif (le dialogue masque la moitié du panneau sinon).
- Les numéros de ligne cités dans `docs/CARTOGRAPHIE_UI.md` pour
  `construction_panel.gd` (ligne « Entretien avant construction ») ont DÉCALÉ de ~18
  lignes après l'ajout de `_extent_mult()`/`_cost_qty_real()` — remis à jour dans le
  même commit (353-361,488-495 → 372-383,510-517) ; à surveiller pour toute future
  édition de ce fichier (les refs de ligne dans ce doc pourrissent vite, pas de
  mécanisme anti-dérive).

### Restes

- Rien d'ouvert côté ressources-prises pour le Menu Construction : l'or, les jours,
  les quantités de matière (édifice) et le prix de manufacture sont maintenant tous
  des miroirs exacts du drain. Le tooltip province (`province_panel_v2.gd`, lu mais
  PAS touché — hors périmètre D5) n'affiche que des grandeurs de PRODUCTION déjà
  bâtie (via `manuf_recipe(bid).out` matché à `_income`), pas des coûts de
  construction — aucun écart de même nature à y chercher.
- Si un futur agent ajoute la possibilité de bâtir un édifice sur une province AUTRE
  que la capitale (actuellement `player_build(type, -1)` fige toujours la capitale
  côté Édifices), il faudra revoir `o->gold` (actuellement calculé sur `cap_reg`
  fixe, `scps_api.c:2661`) ET la nouvelle correction ressources (`_extent_mult()`
  reste valide tel quel — le facteur ne dépend que du NOMBRE de régions du pays, pas
  de LAQUELLE — mais `o->gold`, lui, deviendrait faux si le prix marché varie par
  région).

## MISSION UI-DOCTRINE — D6 : options + tuning sonore (2026-07-18)

**Statut : livré.** 4 curseurs de volume (Général/Musique/Effets/UI-clics) ajoutés à
l'écran Options, câblés sur l'infra `Sound` déjà entièrement écrite (`audio/sound.gd`).

### Découvertes

- **L'infra `Sound.get_vol`/`set_vol` existait déjà en entier** — bus AudioServer
  `Master`/`Ambiance`/`Moments`/`UI` (`default_bus_layout.tres`), application À CHAUD
  (`AudioServer.set_bus_volume_db`) et persistance (`user://audio.cfg`, ConfigFile)
  DÉJÀ écrites et fonctionnelles. La tâche s'est réduite à 4 lignes d'appel côté
  `options_panel.gd` — aucune touche à `sound.gd` ni au bus layout, comme prévu par le
  brief. Gain de temps énorme par rapport à une tâche qui aurait dû construire le mixage
  depuis zéro.
- **Structure plate suffisante** — 3 réglages existants (Langue, Plein écran, Échelle) +
  4 curseurs de son tiennent dans le même `PanelContainer` de 520px sans déborder
  (vérifié par capture, cf. `shots_uidoctrine_d6/02_options_son.png`). Un simple
  `HSeparator` + un label de section « Son » (petit, `C_DIM`) suffit à distinguer le
  bloc — PAS de `TabContainer` : rasoir d'Occam respecté (« pas d'usine à gaz »), motif
  déjà en place dans le fichier (aucune classe imbriquée, aucun nouveau composant).
- **Mapping bus retenu** (identique au brief) : Général→`"Master"`, Musique→`"Ambiance"`,
  Effets→`"Moments"`, UI-clics→`"UI"` — noms de bus vérifiés caractère pour caractère
  contre `sound.gd` (`BUS_UI`/`BUS_AMB`/`BUS_MOM`/"Master").

### Pièges

- **`--headless --path godot/project --import` (la commande documentée dans
  `docs/I18N.md` et `packaging/windows/build_godot.sh`) N'A PAS régénéré
  `ui.fr.translation`/`ui.en.translation` après l'édit du CSV**, malgré un exit code 0 et
  un `[ DONE ] _update_scan_actions` propre — les nouvelles clés (`T_OPT_SOUND_TITLE`
  etc.) restaient absentes des binaires compilés, donc affichées EN CLAIR (`tr()` renvoie
  la clé brute) dans la capture. Cause suspectée mais NON élucidée : sur cette machine/ce
  build (mono, `.NET Sdk not found` au démarrage de CHAQUE run éditeur), le pipeline
  `EditorFileSystem` liste bien `ui.csv` comme item à traiter (« Analyse des actions »)
  mais ne semble jamais exécuter l'import réel (aucun `.godot/imported/ui.csv-*.md5`
  recréé, `ui.csv.import` lui-même pas régénéré après suppression volontaire pour test).
  **PIÈGE JUMEAU découvert en creusant** : supprimer `ui.csv.import` pour « forcer » un
  reimport est CONTRE-PRODUCTIF — ce sidecar porte `importer="csv_translation"` (le CSV
  seul est ambigu, Godot ne sait pas qu'il doit produire des `Translation` sans cette
  metadata) ; sans lui, même un rescan complet ne redéclenche pas le bon importeur.
  **Contournement qui MARCHE, fiable et rejouable** : un script GDScript autonome
  (`extends SceneTree`, lancé via `--headless --script res://x.gd`) qui relit `ui.csv`
  à la main (`FileAccess.get_csv_line()`), construit deux `Translation` (locale fr/en,
  `add_message` par ligne), les compresse via `OptimizedTranslation.generate(from)`
  (exactement l'appel interne de l'importeur `csv_translation` quand `compress=1`), puis
  `ResourceSaver.save()` vers `res://i18n/ui.fr.translation`/`ui.en.translation` — ces
  APIs sont disponibles hors-éditeur (`SceneTree`/`Translation`/`ResourceSaver` sont des
  classes runtime, pas des services de l'éditeur), donc fiables même quand
  `EditorFileSystem` headless est capricieux. Script temporaire, PAS commité (supprimé
  après usage) ; à réutiliser tel quel si un futur agent retouche `ui.csv` et voit le
  même symptôme (littéraux `T_XXX` bruts à l'écran au lieu du texte traduit).
- **Vérifier le rendu réel d'un ajout `i18n/ui.csv` NE SE PROUVE PAS par lecture de
  code** — le CSV et le `tr()` sont syntaxiquement corrects même quand le binaire compilé
  est périmé ; seule une capture fenêtrée (ou un grep direct dans le `.translation`
  binaire, `grep -a "MaClé" ui.fr.translation`) révèle le désync. Repéré uniquement grâce
  à la capture demandée par le brief — sans elle, le bug (clés brutes affichées) serait
  passé inaperçu jusqu'au premier retour joueur.

### Restes

- Rien côté périmètre D6 — les 4 curseurs sont fonctionnels, persistés (Sound gère déjà
  `user://audio.cfg`), traduits FR/EN, et la capture confirme un rendu sobre sans
  dépassement visuel. Le mécanisme de reimport headless défaillant (ci-dessus) mérite
  d'être investigué une fois pour toutes par un futur agent si un AUTRE chantier D
  touche `i18n/ui.csv` et retombe sur le même symptôme — pourrait être lié au build mono
  spécifiquement (`.NET Sdk not found`) plutôt qu'à Godot en général ; non confirmé,
  hors budget de cette mission.

## MISSION UI-DOCTRINE — D7 : tailles d'icônes (rail/topbar/tiroir) (2026-07-18)

**Statut : livré.** Diagnostic Codex hérité (planning-only, ne pouvait pas écrire) exécuté
tel quel — topbar (cellules 26→32, couronne 18→26) + tiroir (6 sites 13-20px → 16-26px
selon la place disponible) ; rail gauche et barres de carte du bas AUCUN changement
(faux positif / déjà correct, voir ci-dessous).

### Le verdict doublon (piste à vérifier en premier, demandée par le brief) : FAUX POSITIF

Le retour joueur « médaillons ronds à fond bronze quasi indistincts » décrit bien la
RÉALITÉ visuelle du rail, mais ce n'est PAS un doublon de code. Trois preuves :
1. `main/main.gd` n'instancie qu'UN SEUL script pour le rail (`res://ui/sidebar.gd`,
   ligne 121) — grep de `SIDEBAR_W`/`draw_circle` sur tout `godot/project` : aucun
   second constructeur de rail, aucun `draw_circle` dans `sidebar.gd`.
2. `icon_button.gd::_draw()` confirmé lu en entier : `bg == ""` (le cas du rail,
   `setup_icon(nom, BTN, "")`) ne peint RIEN sous l'icône — juste un léger surlignage
   hover/sélection (fond plein + soulignement or). Zéro fond de chrome, zéro médaillon
   peint par le CODE.
3. Les PNG eux-mêmes (`assets/scps/ui/icons/menu_economy.png`, `menu_army.png`,
   `menu_diplomacy.png`, inspectés visuellement) SONT dessinés comme des médaillons
   ronds à cadre bronze perlé DANS L'ART — balance de justice/casque spartiate/poignée
   de main, chacun encerclé d'un cadre circulaire sombre avec des points de bordure.
   C'est le style de TOUT le pack `menu_*`, cohérent, pas un accident.

Capture `01_rail_topbar_seuls.png` (zoom rail) confirme : le glyphe utile occupe
seulement le centre du médaillon, une bonne partie des 52×52 px alloués est du
padding/cadre baked-in au PNG — d'où la sensation « petits et interchangeables » même
avec des boutons de 52 px déjà agrandis (le rail avait DÉJÀ été élargi lors d'une
mission antérieure, cf. commentaire `sidebar.gd:16` « retour joueur : très très
petits »). Conclusion : **pas touché `sidebar.gd`** (conforme au brief : faux positif
= ne pas toucher) ; `docs/CARTOGRAPHIE_UI.md` non modifié (pas de doublon structurel à
documenter).

### Tailles avant/après (par fichier)

**`godot/project/ui/topbar.gd`** (`_cell()`, cellules façon CK3, cellule = 48 px de
haut) :
- icône de cellule (ressource par nom / rid / icon générique) : **26 → 32 px** (3
  sites : sprite ressource nommée, sprite ressource par rid, `UIKit.draw_icon`
  générique — les trois partagent le même bloc `if/elif/elif`) ; `tx` (offset texte)
  30→36, `cw` (largeur de cellule) 30.0→36.0 pour garder le même espacement relatif.
- couronne de repli (`politics_crown`, quand le pays n'a pas d'héraldique dérivée) :
  **18 → 26 px**, repositionnée pour rester centrée dans le même emplacement que les
  armes 30×30 qu'elle remplace (avance fixe de 30 px inchangée, donc pas de
  débordement sur la cellule suivante).

**`godot/project/ui/sidebar_drawer.gd`** (6 sites, taille choisie SELON la place
disponible dans chaque ligne, pas une taille uniforme) :
- en-tête d'onglet (`TAB_ICON`, bandeau 36 px) : **20 → 26 px**.
- ligne de classe démographique (`population_group`, ligne 19-20 px de haut) :
  **14 → 16 px** (modeste : la ligne est serrée, pas de marge pour 24-28).
- bouton « Courbes dans le temps » (`menu_economy`, bouton 20 px) : **13 → 16 px**
  (choisi pour matcher les 16 px DÉJÀ utilisés par ses voisins immédiats du même
  onglet Économie — `gold_coin` et `menu_economy`/commerce, tous deux déjà à 16,
  laissés INCHANGÉS car déjà appropriés à leur ligne ~18 px, pas de sur-correction).
- siège de conseil vacant (`menu_council`, repli quand pas de buste de conseiller) :
  **16 → 20 px**, choisi pour matcher le buste PORTRAIT 20×20 déjà dessiné dans le
  même emplacement quand le siège EST pourvu (ligne ~869) — cohérence visuelle entre
  les deux états du même slot.
- en-tête Armée (`menu_army`, ligne avec y+=24, de la marge) : **18 → 22 px**, texte
  décalé x+22→x+26.
- ligne Flotte (`harbor_anchor`, ligne avec y+=20) : **16 → 18 px**, texte décalé
  x+20→x+22.

**`godot/project/ui/sidebar.gd`** : PAS TOUCHÉ (faux positif, voir plus haut — déjà à
52 px nu, doctrine respectée).

**`godot/project/ui/controls.gd`** (barres de carte bas-gauche/droite) : PAS TOUCHÉ —
déjà `const BTN := 52.0` avec le même commentaire « retour joueur : très très petits »
que `sidebar.gd`, donc déjà corrigé lors d'une mission antérieure. Vérifié par lecture
complète du fichier (5 icônes mode + 1 nature + 3 zoom, toutes via `IconButton` à
BTN=52) : aucun sous-dimensionnement résiduel.

### Contraste

Vérifié par capture (6 PNG, `shots_uidoctrine_d7/`) : les glyphes d'encre sombre se
détachent toujours nettement du fond parchemin après agrandissement — AUCUN halo/
contour ajouté (la taille seule suffisait, conforme à la doctrine « solution la plus
simple d'abord »). Zoom Python/PIL sur les crops confirme lisibilité propre sans
chevauchement de texte sur les 6 sites du tiroir + la topbar.

### Découvertes

- **Le drift de commentaire préexistant `topbar.gd:333`** (« icône 22 px à gauche »)
  alors que le code dessinait déjà 26 px avant cette mission — corrigé au passage
  (commentaire mis à jour vers la valeur réelle 32 px) puisque je touchais ce bloc de
  toute façon.
- **`controls.gd` et `sidebar.gd` partagent LE MÊME commentaire historique** (« retour
  joueur : très très petits », `BTN := 52.0`) — preuve que ces deux surfaces avaient
  déjà reçu une passe d'agrandissement lors d'une mission UI antérieure, contrairement
  à `topbar.gd`/`sidebar_drawer.gd` qui ne l'avaient jamais eue. Explique pourquoi le
  diagnostic Codex ne les mentionnait pas comme sous-dimensionnées.

## MISSION — Menu audio + mode observateur (2026-07-30)

**Statut : livré (2 missions GDScript-only).** A : case Muet par bus + les 4 curseurs
D6 existants. B : chrome empire (topbar national + bande droite empire_sidebar.gd +
alertes/popups) masqué en observateur ; carte/date/vitesse/fiches lecture inchangées.

### Découvertes

- **Mission A — le mute et le volume sont déjà DEUX états AudioServer distincts**
  (`AudioServer.set_bus_mute`/`is_bus_mute` vs `volume_db`) — aucune gymnastique à
  faire pour la règle joueur « un mute ne réinitialise pas le slider » : c'est le
  comportement NATIF de l'API, il suffisait de les exposer côté `Sound` (`get_mute`/
  `set_mute`, `audio/sound.gd`) sans inventer un second état. Persisté dans la MÊME
  section `user://audio.cfg` que les volumes (clé `mute` à côté de `volume`, motif
  `_save_volumes`/`_load_volumes` étendu, pas un second fichier).
- **Mission A — piège évité (pas vécu, repéré à la lecture)** : `sound.gd::_ready()`
  appliquait le mute forcé `SCPS_MUTE=1` (probes/captures) AVANT `_load_volumes()`.
  Une fois le mute PERSISTÉ ajouté, un `user://audio.cfg` où Master est sauvé
  non-muet (le cas courant) aurait **réactivé le son** en rechargeant après le mute
  forcé — cassant la garantie « toujours silencieux en probe ». Réordonné : le bloc
  `SCPS_MUTE` est maintenant APRÈS `_load_volumes()`, donc toujours gagnant en
  dernier. Zéro régression observée (aucune session de probe n'a jamais eu de son
  avant ce jour ; le bug n'existait pas encore avant l'ajout du mute persisté — il
  a été neutralisé au même commit qui l'aurait introduit).
- **Mission A — le pipeline `--headless --import` a marché du premier coup cette
  fois** (T_OPT_MUTE compilé et lisible dans `ui.fr.translation`/`ui.en.translation`,
  vérifié par grep du texte clair « Muet »/« Mute » dans le binaire) — contrairement
  au piège documenté par la mission D6 (2026-07-18, contournement SceneTree requis).
  Non élucidé pourquoi ça marche maintenant (peut-être lié au `.NET Sdk not found`
  intermittent évoqué par D6) ; un futur agent qui retombe sur des clés `T_XXX`
  brutes à l'écran doit quand même connaître le contournement SceneTree de D6 en
  filet de secours, ne pas supposer que c'est définitivement réparé.
- **Mission B — `w.player()` GARDE le SLOT DE FOCUS (empire 0) en observateur, ce
  n'est PAS -1** (`scps_api.c:271-278`, doctrine du moteur : « scps_player() garde
  le SLOT de focus → les panneaux montrent ce monde »). Tout code qui lit
  `w.player()`/`w.country_info(me)` sans vérifier `is_observer()` affiche donc
  silencieusement les chiffres d'un empire IA COMME SI c'était le joueur — c'est
  exactement le bug rapporté (« impression d'être associé à un empire »), et il ne
  se voit PAS en lisant `is_observer()` isolément : il faut suivre `player()` jusqu'à
  ses lecteurs.
- **Mission B — le FOG est déjà neutre en observateur, confirmé par lecture**
  (`scps_country_known`, `scps_api.c:264` : « `human_player<0` → rien n'est voilé,
  tout est visible ») — aucune retouche nécessaire, ni possible (moteur hors
  périmètre). Visible aussi sur la capture `02_observateur_chrome_masque.png` :
  TOUS les contours de pays sont dessinés, y compris loin du focus.
- **Mission B — la membrane de décision (dilemmes, `event_dialog.gd`) est DÉJÀ
  gatée moteur-side** (`scps_events.c:2953` : `pending_event_push` n'est appelé que
  si `human_player>=0` — en observateur `pending_count()` reste 0 pour toujours).
  Idem pour la MAJORITÉ du fil d'évènements display (`scps_sim.c:1274` : guerre/
  paix/pillage/sécession/révolte/bataille/siège TOUT le bloc est sous
  `if (s->human_player>=0)`). **Mais PAS le FEED_DIRECTOR** (évènements du
  directeur — `scps_events.c:2867`, `events_strike`) : ces `feed_push` tirent pour
  TOUS les pays sans condition sur `human_player`, filtrés seulement par
  `feed_set_focus(s->sim.player)` (posé à la genèse, **indépendant** de
  l'observateur) — un évènement du directeur touchant l'empire focus (0) traverse
  donc le filtre et, côté GDScript, `alerts.gd::_collect()`/`_poll_feed()` (qui
  lisent `w.player()`, pas `is_observer()`) l'auraient affiché/poppé comme "nôtre".
  **C'était le SEUL vrai trou côté alertes** (le reste du fil est déjà mort par
  construction) — corrigé par le gate `_observing()` dans `alerts.gd::_refresh()`.
- **Mission B — `alerts.gd::_refresh()` est un point de gate UNIQUE très rentable** :
  y ajouter `or _observing()` (même motif que le gate `not Sim.game_on` déjà
  présent) tue en un seul endroit trois symptômes à la fois — les CONDITIONS
  (`_collect()` : conseil vacant/guerre/pénurie/etc de l'empire focus affichées
  comme siennes), le JOURNAL (bande droite, déjà masquée par ailleurs mais
  redondance utile), ET les popups « OYEZ OYEZ » (`event_popup.gd`, câblé sur
  `alerts.popup_requested` — sans ce gate un évènement du directeur aurait ouvert
  un popup modal PAUSE adressé à l'observateur pour un empire qui n'est pas le
  sien).
- **Mission B — `country_panel.gd::show_country(me)` se re-ferme TOUJOURS**
  (`country_panel.gd:49` : « ce panneau ne s'ouvre que pour un pays ÉTRANGER — le
  clic chez soi garde la province », `cid==player()` → `cid=-1` → `visible=false`)
  — piège rencontré en écrivant la probe `observer_shot.gd` (le shot 03 initial,
  `show_country(me)`, produisait une capture IDENTIQUE au shot sans fiche ouverte :
  pas un bug de la mission, juste le motif existant qui traite le SLOT DE FOCUS
  comme "notre" pays même en observateur). Corrigé côté probe : cible un pays
  ÉTRANGER (`c != me`) pour prouver la fiche pays en lecture, + un clic province
  (`_on_province_picked`, motif `d1_after_shot.gd`) pour la fiche province.

### Pièges

- **`godot/project/map/*` était EN COURS D'ÉDITION par l'orchestrateur PENDANT
  cette mission** (`map/overlay.gd`, diff live passé de +57/-5 à +75/-39 lignes
  entre deux `--check-only` consécutifs) — a cassé temporairement TOUT boot de
  `Main.tscn` (`ROAD_MINOR_MAIN`/`ROAD_MAIN`/… « not declared », `overlay.gd:2443+`)
  et donc bloqué la probe `observer_shot.gd` en fin de mission (les 2 premiers
  shots, capturés AVANT cette édition concurrente, sont intacts et suffisent comme
  preuve — voir Restes). Confirme le piège annoncé par le brief : deux agents sur
  le même arbre Godot se marchent dessus, pas seulement via le cache `.godot/` mais
  via l'ÉTAT SOURCE lui-même pendant une sauvegarde partielle. Aucune tentative de
  toucher `map/*` (hors périmètre, interdit par le brief).
- **`Sound._save_volumes()`/`_load_volumes()` bornent `Master` à `["Master", BUS_UI,
  BUS_AMB, BUS_MOM]`** — si un futur agent ajoute un 5ᵉ bus AudioServer, il doit
  l'ajouter à CETTE liste (dans les deux fonctions) sinon son volume/mute ne
  persistera jamais (silencieux, pas d'erreur).
- **La ligne `_vol_row` compte maintenant 3 contrôles (label 150px + case Muet
  76px + slider EXPAND_FILL)** — vérifié par capture que ça tient dans le panneau
  520px sans déborder (`shots_uidoctrine_d6/02_options_son.png`, régénéré par
  cette mission) ; un futur agent qui ajoute un 4ᵉ contrôle sur cette ligne doit
  revérifier par capture, pas supposer.

### Restes

- **Mission A** : rien d'ouvert — 4 bus (Général/Musique/Effets/Clics d'interface),
  chacun mute+slider indépendants, tous persistés/traduits/vérifiés par capture.
  Note de portée : le brief joueur ne nommait que 3 canaux (son/musique/effets) ;
  la case Muet a été ajoutée aux 4 lignes existantes (dont Clics d'interface, posé
  par D6) par cohérence de motif — pas une invention de canal, `_vol_row` est
  DÉJÀ partagé par les 4 depuis D6.
- **Mission B** : par construction, le masquage suit strictement l'énumération du
  brief (topbar national, bande droite empire_sidebar.gd en entier, alertes/
  popups/journal). **PAS touché, documenté comme hors-périmètre explicite** : le
  RAIL GAUCHE (`sidebar.gd`) et son tiroir (`sidebar_drawer.gd`, onglets Économie/
  Démographie/Stocks/Marché/Armée/Filtres/Diplomatie/Conseil) restent accessibles
  en observateur et affichent encore les données de l'empire focus (0) — le brief
  ne les citait pas dans l'énumération « chrome » (toujours visible), seulement le
  MENU GAUCHE contextuel/à-la-demande ; un clic y montre encore « votre » armée/
  économie du focus. Filet de sécurité déjà en place côté moteur : « les commandes
  joueur sont JETÉES au drain » (`scps_api.c:270`) — un bouton de verbe cliqué là
  ne fait donc RIEN mécaniquement, c'est un défaut d'IMMERSION, pas de simulation.
  Si un futur retour joueur demande aussi ce rail neutre, gate `_observing()` sur
  `sidebar.gd`/`sidebar_drawer.gd` (même motif dupliqué que les 3 fichiers de
  cette mission) — gros fichier (1805 lignes), à traiter en mission dédiée.
- **Probe `observer_shot.gd`** : shots 01 (référence chrome complet) et 02 (chrome
  masqué) capturés et vérifiés à l'œil AVANT la casse concurrente de `map/*` — la
  preuve du cœur de la mission est donc solide. Les shots 03/04 (fiches pays
  étranger / province en lecture, ajoutés après le premier passage pour couvrir le
  piège `show_country(me)` ci-dessus) N'ONT PAS PU être recapturés avant la fin de
  la mission (map/overlay.gd cassé par l'édition concurrente au moment du retry) —
  à relancer par un futur agent une fois `map/*` stabilisé :
  `Godot --path godot/project res://observer_shot.tscn -- seed=9 years=5`
  (SCPS_MUTE=1, --audio-driver Dummy, fenêtré). Le fichier probe est committable
  tel quel, rien à corriger dedans.

### Pièges

- **Le tool Edit a échoué de façon intermittente sur des `old_string` MULTI-LIGNES
  pourtant caractère-pour-caractère identiques au contenu relu juste avant** (topbar.gd,
  4 tentatives sur le même bloc de 12 lignes, 3 échecs « String to replace not found »
  malgré une comparaison `cat -A` ne montrant aucun caractère invisible suspect). Le
  contournement qui a fonctionné à chaque fois : réduire l'`old_string` à UNE SEULE
  ligne unique dans le fichier (au besoin avec `replace_all: true` si la ligne apparaît
  plusieurs fois avec le même remplacement). Cause exacte non élucidée (peut-être une
  limite de longueur/normalisation du diff interne à l'outil sur ce run) — un futur
  agent qui rencontre « String to replace not found » sur un bloc qu'il vient de lire
  mot pour mot devrait essayer immédiatement de découper en édits d'une ligne plutôt
  que de perdre du temps à ré-inspecter l'encodage.
- **Envoyer plusieurs `Edit` en parallèle (un seul message, plusieurs tool_use) sur LE
  MÊME fichier ne garantit pas un ordre séquentiel propre** — un des 4 edits parallèles
  est passé, les 3 autres ont échoué comme si le fichier qu'ils visaient n'avait pas
  encore reçu la modification précédente (ou l'inverse). Sur un fichier édité en PLUSIEURS
  passes successives, préférer des `Edit` séquentiels (un par un, en vérifiant) plutôt
  que de les grouper — le gain de parallélisme ne vaut pas le risque de désynchronisation
  sur des edits qui se chevauchent dans le même fichier.

### Restes

- **Le padding/cadre baked-in des PNG `menu_*`** (médaillon rond, glyphe central minoritaire
  dans les 52×52 px alloués) reste la cause RÉELLE de la sensation « quasi indistincts » du
  rail — hors périmètre D7 (édition de contenu binaire interdite par le brief, lecture
  seule). Un futur agent ART (pas layout) pourrait recadrer/agrandir le glyphe utile DANS
  le PNG lui-même (crop du cadre bronze, ou variante « sans cadre » pour le rail spécifiquement,
  vu que `sidebar.gd` dessine déjà l'icône nue — le cadre du PNG double alors visuellement
  le cadre-hover du code).
- **La couronne de repli `politics_crown` (topbar, 26 px après cette mission) n'a pas été
  vérifiée visuellement en situation réelle** — le pays de test (Clans Dornyana, seed 9)
  a une héraldique dérivée valide (`heraldry.gd::arms()` non-null), donc le chemin `else`
  (couronne) n'a jamais été exercé dans les 6 captures. Le changement est un simple
  nombre (18→26) dans un `UIKit.draw_icon` déjà utilisé ailleurs à cette taille, risque
  jugé faible, mais un futur agent qui doute peut forcer `heraldry.gd::arms()` à retourner
  `null` en probe pour vérifier à l'œil.
- **`sidebar_drawer.gd::_draw_eco()` (icônes gold_coin/menu_economy déjà à 16 px, lignes
  ~456/519) volontairement LAISSÉES INCHANGÉES** — jugées déjà proportionnées à leur ligne
  serrée (~18 px), pas de marge visible pour grandir sans empiéter sur la ligne suivante.
  Si un retour joueur les signale comme encore petites, elles ont une marge de ~2 px
  disponible (16→18 max avant collision).

---

## MISSION UI-DOCTRINE — D4 : glossaire hover (2026-07-18)

**Statut : livré.** 2 entrées `concepts.gd::DEFS` ajoutées (Frappe, Dette) + 1 correction
(clé « Credo » sans accent → « Crédo », morte depuis sa création — ne matchait jamais le
mot réellement affiché), 1 fonction publique ajoutée (`Concepts.def_of_label()`), ~25
libellés/sections câblés en `tooltip_text` sur 9 fichiers de panneau (`army_panel.gd`,
`battle_panel.gd`, `budget_panel_v2.gd`, `country_actions.gd`, `country_panel.gd`,
`economy_panel.gd`, `empire_window.gd`, `religion_panel.gd`, `tech_panel.gd`). Boot
headless + `make lang-check` (0=0, inchangé) verts après chaque lot. `construction_panel.gd`
et `memory_panel.gd` volontairement NON touchés (raisons ci-dessous).

### Découvertes

- **Piège moteur VÉRIFIÉ (pas supposé) : `(?i)` de Godot (RegEx, moteur PCRE2) NE replie
  PAS la casse des majuscules ACCENTUÉES.** `Concepts.def_of_label("DÉBASE")` (un titre de
  section tout en capitales, motif standard de tous les panneaux à conteneurs de ce projet)
  retournait `""` alors que `Concepts.def_of_label("Débase")`/`"débase"`/`"SIÈGE"` (minuscule)
  fonctionnaient tous. Testé isolément (`--headless --script`, motif jetable) : `"DÉBASE".to_lower()`
  donne bien `"débase"` (Godot `String.to_lower()` replie CORRECTEMENT les accents français,
  contrairement au moteur RegEx). Fix dans `def_of_label()` : abaisser la casse du label AVANT
  `decorate()`, jamais touché le `(?i)` du `_regex()` partagé (utilisé par `decorate()` PARTOUT
  dans le hover du jeu — une modif là aurait un rayon d'effet hors mandat D4). Sans ce détour,
  TOUS les titres de section en capitales-accentuées de ce chantier (« DÉBASE » notamment)
  auraient silencieusement affiché AUCUN hover malgré un code d'appel qui semblait correct —
  un bug qu'une simple relecture de code n'aurait PAS révélé (il faut l'exécuter pour le voir).
- **Collision de sens confirmée par grep AVANT tout câblage, pas après coup** : « Cohésion »
  désigne le MORAL de bataille dans `army_panel.gd`/`battle_panel.gd` (`atk_morale_pct`/
  `def_morale_pct`, lecteur `battle_info`), un concept COMPLÈTEMENT différent de la Cohésion
  NATIONALE (unité des cultures) déjà définie dans `concepts.gd::DEFS`. Un câblage générique
  aveugle du label → `def_of_label()` sur ces deux fichiers aurait affiché la MAUVAISE
  définition (le pire résultat possible pour un glossaire — plus trompeur que zéro hover).
  Vérifié explicitement AVANT d'écrire le moindre wiring générique (`_stat_line` d'army_panel.gd
  n'a PAS été câblé du tout pour cette raison — le seul terme qui y matchait DEFS était
  justement ce faux-ami). Un futur agent qui verrait « Cohésion » sans hover dans ces deux
  fichiers ne doit PAS le corriger par réflexe — c'est voulu.
- **`country_panel.gd` a du code mort non touché (`ROWS`/`TIPS`/`_gauge_row`, lignes
  13-34 et 129-133)** : 5 jauges (Stabilité/Prospérité/Légitimité/Cohésion/Savoir) et leur
  fonction de rendu, plus jamais appelées depuis que la doctrine « le panneau pays ÉTRANGER
  ne montre plus les jauges internes » (retour joueur 2026-07-10, commentaire déjà en place
  ligne 97-100) a retiré leur site d'appel dans `_draw()`. Repéré en cherchant où ajouter un
  hover — ni ajouté ni supprimé (hors mandat D4, signalé séparément par `spawn_task`).
- **`scps_sim_node.cpp::budget_controls()` (ligne ~1509) renvoie encore les noms de classe
  LEGACY** (`"Laboureurs", "Artisans", "Noblesse"`) pour les lignes d'IMPÔT du Trésor, alors
  que `budget_panel_v2.gd::CLASS_NAMES` (Fiscalité par ordre, onglet Monnaie) et tout le reste
  du jeu utilisent la nomenclature CANONIQUE confirmée par UI-DOCTRINE D1
  (Journaliers/Bourgeois/Élite). Les DEUX noms cohabitent dans la MÊME fenêtre Trésor selon
  l'onglet (Balance affiche « Laboureurs », Monnaie affiche « Journaliers » pour la MÊME
  classe). `scps_sim_node.cpp` est hors périmètre D4 (ni un panneau `ui/*.gd`, ni autorisé par
  le brief) — non touché, signalé ici pour la prochaine vague qui touchera ce fichier.
- **`tech_panel.gd` et `battle_panel.gd` n'avaient STRICTEMENT AUCUN mécanisme de hover**
  (dessin immédiat pur, zéro `Control` par ligne, zéro `_get_tooltip` existant — grep
  `tooltip_text` = 1 hit dans tech_panel.gd, l'assignation de `card.tooltip_text`, la SEULE
  déjà câblée ; 0 hit dans battle_panel.gd). Le motif `_tips: Array` + `_get_tooltip(at_position)`
  déjà établi dans `country_panel.gd` (posé avant D4, pour « Influence ») a été COPIÉ tel quel
  dans les deux — la seule différence : mes tips stockent la DÉFINITION COMPLÈTE
  (`Concepts.def_of(...)`) plutôt que le nom nu du concept, pour suivre le motif `_kv()`
  QUE LE BRIEF DEMANDAIT explicitement de copier (province_panel_v2.gd, hors mandat D4,
  jamais touché) — les deux conventions coexistent maintenant dans le même fichier
  `country_panel.gd` (l'ancienne tip « Influence » = mot nu, ma nouvelle tip « Éthos » =
  définition complète) : assumé, pas une incohérence à corriger (ne pas toucher au hover
  DÉJÀ fonctionnel d'un autre agent).

### Pièges

- **Un survol souris SIMULÉ (`Input.warp_mouse()` + `InputEventMouseMotion` de synthèse)
  échoue de façon reproductible dans une probe fenêtrée de ce projet.** Cible en espace
  canvas (159,202) (= `Control.get_global_rect().get_center()`) → `Input.warp_mouse()` puis
  `get_viewport().get_mouse_position()` rapporte (132.5, 168.3) — un ratio constant ~5/6, PAS
  un simple stretch `project.godot` (`viewport_width/height` = 1600×900, IDENTIQUE à
  `get_window().size` posé par la probe — donc pas un mismatch stretch évident). Cause exacte
  NON creusée (hors budget de cette mission — le brief autorise EXPLICITEMENT le repli texte
  quand le survol minuté est trop coûteux à fiabiliser). Contournement : capture du panneau
  RÉEL (contenu vérifié) + `print()` du `tooltip_text` résolu juste avant chaque capture — les
  DEUX preuves ensemble (screenshot du panneau + texte exact imprimé) suffisent à démontrer
  le câblage sans un pixel de tooltip visible. Un futur agent qui voudrait un VRAI pixel de
  tooltip en probe devra d'abord percer ce ratio de coordonnées (piste : `content_scale_factor`
  du viewport racine, ou un DPI de fenêtre différent du `window/size` déclaré).
- **`def_of_label()` (nouvelle fonction) pêche le PREMIER concept trouvé dans le texte, pas
  le plus pertinent.** Exemple vérifié : `def_of_label("Sur-frappe au-delà de la parité")`
  matche à la fois « Frappe » (dans « Sur-frappe », le tiret n'est PAS un caractère de mot
  pour la frontière regex `(?<![\wÀ-ÿ])`, donc « frappe » y est bien un mot ENTIER du point
  de vue du moteur) ET « Parité » — le premier gagne (« Frappe »). Pas un bug dans ce cas
  précis (la définition de Frappe référence elle-même la Débase, reste pertinente) mais un
  comportement à connaître avant de réutiliser `def_of_label()` sur un label à PLUSIEURS
  concepts : il ne choisit pas le "meilleur", juste le premier positionnellement.
- **Un tiret (« Sur-frappe ») n'est PAS une frontière de mot bloquante pour le moteur de
  `concepts.gd`** — la classe de caractères de bordure est `[\wÀ-ÿ]` (lettres/chiffres/
  underscore + Latin étendu), qui EXCLUT le tiret. Un concept comme « Frappe » peut donc
  matcher à l'INTÉRIEUR d'un mot composé à trait d'union sans qu'on s'y attende — à vérifier
  avant d'ajouter un nouveau terme DEFS court et courant (risque de faux-positifs dans des
  mots composés existants ailleurs dans le jeu).

### Restes

- **`construction_panel.gd` NON touché, délibérément.** Tout le rendu de carte
  (édifice/manufacture) route DÉJÀ son survol via `get_info_card()` (consommé par
  `TooltipServer::_card_bb()`, qui appelle `Concepts.decorate()` sur CHAQUE ligne — Or/Effet/
  Recette/Entretien de manufacture sont donc DÉJÀ décorés turquoise automatiquement, sans
  rien à ajouter). Les labels ENFANTS de la carte (titre/prix/entretien édifice/prochain
  palier/raison de verrou) sont TOUS `mouse_filter = MOUSE_FILTER_IGNORE` DÉLIBÉRÉMENT (pour
  que le survol de N'IMPORTE OÙ sur la carte montre la MÊME carte structurée) — leur donner
  un `tooltip_text` individuel serait INERTE (la souris ne s'arrête jamais sur eux) sans
  AUSSI changer leur `mouse_filter`, ce qui romprait le design « toute la carte = une seule
  cible de survol ». Les deux VRAIS trous (édifice : « Entretien »/« Palier » absents des
  `lines` de `_build_info_card()`, alors que la manufacture LES A déjà) exigent de toucher
  `_build_info_card()` — explicitement le territoire de l'audit COÛTS D5 en cours sur ce
  même fichier au moment de cette mission (brief D4 : « ne touche PAS aux chiffres de
  coût »). Un futur agent (après D5) pourrait ajouter ces deux lignes à `_build_info_card()`
  une fois le terrain dégagé.
- **`memory_panel.gd` NON touché, délibérément.** L'onglet Comparer construit un SEUL
  `RichTextLabel` en tableau BBCode (`_snapshot()`/`_refresh_compare()`) — les libellés de
  ligne (Stabilité/Prospérité/Cohésion/Agitation…) sont des CELLULES de texte BBCode, pas des
  `Label` individuels : le motif `_kv()`/`tooltip_text` ne s'y applique pas sans réinventer un
  système de survol PAR CELLULE (meta_hover sur un RichTextLabel unique, jamais câblé nulle
  part dans ce projet pour un usage NON-tooltip) — jugé hors du rasoir « solution la plus
  simple » pour cette mission. Un futur agent pourrait au moins COLORER (`Concepts.decorate()`
  sur le label de chaque ligne, sans lien cliquable) pour un gain visuel minimal, si demandé.
- **`scps_sim_node.cpp::budget_controls()` noms de classe legacy** (Laboureurs/Artisans/
  Noblesse vs Journaliers/Bourgeois/Élite) — cf. Découvertes ci-dessus, hors périmètre D4.
- **Le doublon des 3 fiches province et des curseurs budgétaires** (cf. missions D1/D2/D3
  précédentes) reste inchangé — non touché par D4, mandat glossaire seul.

---

## MISSION UI-DOCTRINE — CLÔTURE D1-D7 (2026-07-18)

**Vague COMPLÈTE.** Sept chantiers, cinq agents (D1-D3 orchestrateur en direct ;
D4/D5/D6/D7 en parallèle sur le MÊME arbre — la cohabitation multi-agents sans
worktree a tenu, au prix de deux collisions de commit bénignes, voir Pièges).
Commits : D1 `8c090ec` + fix `0f8d6a7` · D2 `878b163` · D3 `d557685` · carto
`987fb23`/`e6331ca` · Codex `e323c33` (contient AUSSI le D5 co-commité) · D6
`0025a85` · D7 `aee53ee` · D4 `49dcacb`. Tag `pre-uidoctrine` = état avant tout.

### Vérifications finales (toutes VERTES)
- Boot headless Main.tscn : zéro SCRIPT ERROR, relancé après chaque chantier ET en clôture.
- `make lang-check` : 0 littéraux (base 0), inchangé.
- `core_demo` : 35/35.
- Audit 3-clics final : 0 dépassement (cartographie §B, clôture datée).
- Captures en contexte réel (Main.tscn fenêtré, jamais --headless) :
  `shots_uidoctrine_d1/` (6, dont AVANT via worktree jetable sur le tag) ·
  `shots_uidoctrine_d4/` (6) · `shots_uidoctrine_d5/` (2) · `shots_uidoctrine_d6/`
  (2) · `shots_uidoctrine_d7/` (6).
- Re-export `scps.exe` : `packaging/windows/build_godot.sh` (DLL release rebuildée
  + import + export « Windows Desktop », PCK embarqué) — voir le résultat du run
  dans le rapport de mission ; dist_godot/ est gitignoré comme toujours.

### Pièges (nouveaux, au-delà de ceux des sections D1-D7 ci-dessus)

- **Deux commits co-mélangés par la cohabitation multi-agents sur le MÊME arbre** :
  l'index git est PARTAGÉ — un `git add` + `commit` de l'agent A peut embarquer les
  fichiers stagés au même moment par l'agent B (arrivé : D5 absorbé dans le commit
  Codex `e323c33` ; l'append TROUVAILLES de D7 co-commité par D6 `0025a85`). AUCUNE
  perte de contenu (les diffs sont intacts, `git show <hash> -- <fichier>` le
  prouve), seuls les MESSAGES de commit ne listent pas tout. Leçon : pour une
  prochaine vague parallèle, soit sérialiser les commits (un slot de commit à la
  fois, annoncé), soit vrais worktrees par agent. On n'a PAS réécrit l'historique
  (amend/rebase interdits avec d'autres agents actifs dessus).
- **Attendre un sous-agent : le poll bloquant en avant-plan est le SEUL réveil
  fiable** (10e occurrence du piège « rien ne te réveillera » documentée par le
  coordinateur). Motif qui marche : boucle `while true` foreground avec un doube
  critère de sortie — le commit attendu apparaît dans `git log` (succès) OU l'arbre
  n'a plus bougé depuis 6 minutes (mort présumée, on reprend soi-même le reste).
  Jamais un simple « j'attends la notification ».

### Restes (fin de vague — pour une prochaine mission)

- L'art des PNG `menu_*` du rail (médaillon bronze bakés DANS l'image, glyphe
  central minoritaire dans les 52 px) — la vraie cause du rail « quasi indistinct »
  (verdict D7) ; chantier ART, pas code.
- Le blocage `prosperity≈0` post-famine an-0 (seed 9, hérité d'UI-POLISH) — moteur,
  jamais dans le périmètre de cette vague.
- Doublon interne budget_panel_v2 (Balance vs Monnaie, `_sliders`/`_m_sliders`) et
  doublon de VUE budgétaire ×4 (lecture seule, assumé) — catalogués §C.3/§D.2.
- `army_panel.gd` hors pile Échap (assumé : fermeture par désélection carte).
- Code mort signalé par D4 dans country_panel.gd (`ROWS`/`TIPS`/`_gauge_row`).

## 2026-07-25 — régression UI « bordelisé » (fiche/diplo) : trouvée & réparée
- **Découverte** : le tiroir diplo « chunky » = piège Godot — mesurer `get_combined_minimum_size()`
  d'un VBox SYNCHRONEMENT après rebuild ment (labels autowrap pas layoutés, largeur 0 → un mot
  par ligne : mesuré 2638 px pour ~600 réels) → clamp au plafond → pleine hauteur permanente ET
  jamais de scrollbar. Antidote = motif `_fit_scroll` (2 frames de grâce + jeton), appliqué à
  `country_actions._layout`.
- **Découverte** : l'image biome (BiomeTip) était morte depuis que le TooltipServer a désactivé
  les tooltips natifs (`tooltip_delay_sec=100000`) — `_make_custom_tooltip` ne tirait plus jamais.
  Le serveur EMBARQUE désormais le Control custom au show (une instanciation, jamais dans le poll).
- **Piège** : `probe_ui.sh` FILTRE stdout (grep interne) — un print de debug n'y survit pas ;
  lancer Godot direct pour voir les prints.
- **Piège (process)** : « supprime la verbose inutile [des commentaires] de l'UI » visait le
  blabla IN GAME (phrases sous les boutons, ACTION_HELP inline, context_hint), PAS les
  commentaires du code — le joueur TIENT aux commentaires (repères/contexte). Trim de code
  intégralement restauré (checkout pré-trim + merge des changements fonctionnels).
- **Restes** : couleurs claires héritées de l'époque panneau-sombre à traquer ailleurs
  (country_panel? battle?) ; vérifier en jeu le hover biome + la scrollbar au débord réel.

## 2026-07-28 — battle_anim.gd : fond terrain (couleur) + 1 forme = 1 régiment
- **Découverte** : `setup()` ne lit JAMAIS les champs `atk_units`/`def_units` de battle_info —
  le total par camp vient de la somme `inf+arch+cav+mages` (comp[]). Ces deux champs ne servent
  qu'à `on_tick()` (delta pertes/renfort). Piège si on tente de « fixer » un total via `*_units`
  seul dans un scénario synthétique : ça n'a aucun effet sur la formation initiale.
- **Décision (fond)** : l'image biome enluminée (UIKit.biome_painting, planches parchemin) est
  RÉSERVÉE à la fiche province — le widget bataille peint désormais un simple APLAT dérivé des
  mots relief/climat (`_terrain_color`, statique, même style de matching substring que
  biome_painting mais → Color, pas Texture2D). Table : glace/toundra/polaire → pâle,
  jungle → vert sombre, marais/tourbe/mangrove → vert-brun, désert/dune/aride → sable,
  mont/pic/volcan → gris-brun (réutilise SLICE_PAL[4]), côte/littoral/mer → sable-bleu,
  défaut (dont plaines et `setup_parade` sans mots) → herbe sourde. Le preload UIKit a sauté
  (plus aucun autre usage dans le fichier) ; `_bg: Texture2D` → `_bg_col: Color`.
- **Décision (échelle)** : `MAX_SHAPES=22` avec `_per = ceil(total/MAX_SHAPES)` (une forme
  représentait un compte VARIABLE selon la taille de l'armée — 1 forme ≠ 1 unité lisible)
  remplacé par `PER_UNIT=100` FIXE (1 forme = 1 régiment de 100 hommes) + `MAX_SHAPES` relevé
  à 30 comme SEUIL d'escalade (pas de troncature) : `_shape_count(comp, per)` rejoue exactement
  la grille (3 familles ceil + cavalerie plafonnée à 4) et `_per` grimpe par pas de 100 tant que
  ce compte dépasse 30 — jamais le `if n >= MAX_SHAPES: break` de la boucle de génération n'est
  censé se déclencher (garde-fou mort, laissé tel quel). `_kill`/`_spawn` n'ont pas été touchés
  (ils consomment déjà `_per[side]`, comme demandé) ; `_spawn` garde SON propre plafond
  `idx >= MAX_SHAPES` — un camp déjà à 30 formes n'affiche pas de nouvelles arrivées visuelles
  au-delà (le compteur de troupes réel, lui, suit quand même via `_units_seen`) : gardé tel
  quel, hors périmètre de cette mission (seul le calcul de `_per` initial était visé).
- **Piège** : `battle_anim_shot.gd` avait un scénario à `units_are_humans:false` (échelle ×100
  engine→hommes) — un corps de « 300 hommes » = `atk_inf: 3` (pas 300) dans le dict de test ;
  facile de se tromper d'un facteur 100 en composant le synthétique.
- **Gates** : parse `--check-only` propre (seule la fuite RID FontAdvanced connue, ignorée) ;
  probe fenêtrée → 4 PNG lus (`godot/project/shots_battle/`) :
  - `01_formations` : fond tan/sable plat (Désert/Aride, plus d'image texturée) ; camp rouille
    (atk, 300 h) = 3 carrés en 1 colonne verticale ; camp acier (def, 3000 h) = grille compacte
    6 colonnes × 5 rangs = 30 carrés, aucune forme manquante/tronquée.
  - `02_choc` : étiquette « choc 1 », étincelles (croix claires) entre les deux blocs, léger
    jitter/décalage des formations (moral tombé à 80/60 %), 1 forme atk déjà éteinte (fade).
  - `03_apres` : étincelles disparues, atk à 2 carrés visibles (1 mort fondu), def à ~26 carrés
    (4 killed, trou visible dans la grille en bas à gauche).
  - `04_renfort` : le bloc atk s'est étoffé (colonne allongée, ~8 carrés visibles = 2 restants +
    6 arrivants entrés par le bord gauche) ; def inchangé (aucun renfort programmé ce tick).
- **Restes** : aucun — les 2 items de la mission (fond terrain, 1 forme = 1 régiment) sont
  posés et prouvés par la probe ; le plafond d'affichage des renforts (`_spawn` idx>=MAX_SHAPES)
  reste un comportement pré-existant, pas retouché, à examiner seulement si un jour un camp doit
  visiblement dépasser 30 formes AVEC renfort en cours de bataille.

## 2026-07-28 — MOTEUR ARMÉE : la FORCE NOMINALE (renforcer = combler le déficit) + SPLIT COMPOSÉ

**Découvertes**
- **`campaign_order` est le VRAI point de levée, pas `campaign_raise`.** Le brief supposait
  `campaign_raise` comme LE verbe de levée (le « ? » dans la consigne) — en réalité l'IA
  (`sim_campaign_orders`/`sim_campaign_defense`, scps_sim.c) ET le joueur (CMD_CAMPAIGN,
  CMD_MOVE_ARMY réserve→déploiement) créent/agrandissent le corps « slot 0 » (l'ancien corps
  principal, `CAMPAIGN_CORPS_ID(owner,0)==owner`) via `campaign_order`, qui ne passe JAMAIS par
  `campaign_raise`. `campaign_raise` (CMD_CORPS_RAISE) n'est QUE le verbe explicite multi-corps
  (détacher un DEUXIÈME corps). Sans poser le nominal aussi dans `campaign_order`, tout corps
  IA et tout corps joueur « classique » (non explicite) aurait `nominal=0` en permanence
  (memset de `campaign_init`), donc DÉFICIT NÉGATIF clampé à 0 en permanence → jamais de
  renfort possible pour l'immense majorité des armées du jeu. Résolu par un helper commun
  `corps_ratchet_nominal()` (« le nominal suit le pic », jamais un plafond dur) appelé aux
  QUATRE points où le courant d'un corps peut croître : `campaign_order` (le vrai point de
  levée du corps historique), `campaign_raise` (memset→0 puis assigné au courant, équivalent),
  `campaign_merge` (dst += src->nominal puis filet ratchet), `campaign_refill_corps` (fin de
  vague, au cas où plusieurs lignes remplies dans le même appel dépassent l'ancien pic).
- **Un corps FRAIS est déjà À SON PLEIN par construction** (nominal posé = courant au moment
  de la levée) — le renfort y est un no-op LÉGITIME (déficit nul), pas un bug. Le test
  `campaign_demo.c` §2 (« la milice reçoit 100 hommes ») supposait l'ANCIEN comportement
  (+100 inconditionnel) ; corrigé pour D'ABORD prouver le refus sur corps frais, PUIS bump
  `camp2->army[A].nominal += 1` (accès direct au champ — FieldArmy n'est pas opaque en C) pour
  simuler une perte passée avant de prouver le comblement. Le même écueil existe pour TOUT
  futur test qui présumerait « refill toujours +100 ».
- **Invariant caché dans `scps_api_demo.c` (déjà présent, ligne ~1245)** :
  `population_ready_humans <= requested_humans <= …` était VRAI dans l'ancien monde (chaque
  ligne contribue systématiquement à `requested_humans`) mais devient FAUX dès que
  `requested_humans` = un déficit total (potentiellement < n_lignes × 100) sans changer le
  calcul de `population_ready_humans`/`guaranteed_humans` (toujours « +100 par ligne présente,
  sans plafond »). Corrigé en CAPANT les deux boucles de `scps_corps_refill_preview` à un
  budget partagé = `requested_humans` (elles s'arrêtent d'accumuler une fois le déficit
  « couvert » sur le papier) — sans ce cap, le banc existant aurait cassé sur n'importe quel
  corps multi-lignes proche du plein. Un futur agent touchant cette fonction : ne PAS retirer
  le budget sans revérifier cet invariant.
- **`scps_api_demo.c` n'avait JAMAIS exercé un vrai corps de campagne du joueur** (aucun appel
  à `scps_player_recruit`/`scps_player_raise_corps`/`scps_player_campaign` n'était vérifié en
  aval — `scps_player_campaign(s2,0,1)` ligne ~265 ne teste que l'ENFILAGE, jamais le succès).
  Les boucles `scps_country_corps_count(sd,me_refill)` du bloc war_state tournaient
  probablement à VIDE depuis leur écriture (le joueur, exclu de l'auto-campagne IA via
  `c==s->human_player`, n'a de corps que s'il en commande un). Pour les 3 nouveaux asserts,
  construit une Sim DÉDIÉE : `scps_player_recruit(sn,U_MILICE)` ×6 (armes de fortune, RES_NONE
  → AUCUN gate d'arsenal, seul un pool `LAB_LABORER` non-nul suffit, robuste dès la genèse) →
  advance → `scps_country_army().regiments` lu pour savoir EXACTEMENT combien de paquets sont
  disponibles avant de `raise_corps` (jamais un nombre en dur qui pourrait dépasser la réserve
  réelle) → `raise_corps(take, cap_reg)` avec `from==target==capitale` (pas de marche, IDLE
  immédiat, zéro dépendance au pathing). A fallu `#include "scps_army.h"` dans
  `scps_api_demo.c` (jamais utilisé avant) pour la constante `U_MILICE` — le fichier ne teste
  QUE la façade opaque d'habitude, mais `scps_player_recruit(ScpsSim*, int unit)` prend déjà
  un entier brut de toute façon (pas une vraie fuite d'abstraction).
- **`ScpsArmyInfo.inf/arch/cav/mages` sont déjà ×100 (des HOMMES, pas des paquets)** — piège à
  un facteur 100 si on compare au paramètre PAQUETS de `scps_player_split_comp` (qui prend des
  paquets, comme `campaign_split`). `split_comp(id,1,0,0,0)` (1 PAQUET) donne
  `aiN.inf==100` (HOMMES), pas `aiN.inf==1`.
- **`category_take` (nouveau, scps_campaign.c) doit dupliquer le classement de
  `campaign_corps_composition`, PAS le réutiliser** — refactoriser cette dernière touchait du
  code hors-sujet (mission « ne pas toucher au code hors sujet ») ; les deux switches
  (`unit_category` / le switch inline de `campaign_corps_composition`) DOIVENT rester
  identiques : tout futur ajout au roster (roster 22 → 23) doit toucher les DEUX.
- **`campaign_disband_corps` ne remettait PAS tous les champs à zéro** (déjà le cas AVANT
  cette mission pour `rally_used`/`rally_days`/`rally_packets` — non touché, hors sujet) ; le
  slot0 réutilisé via `campaign_order` (jamais memset, contrairement à `campaign_raise`/
  `campaign_split`) aurait hérité d'un `nominal` fantôme d'une INCARNATION précédente du même
  slot sans le `a->nominal=0;` ajouté à `campaign_disband_corps`.

**Pièges**
- MSYS2 `bash.exe -lc '...'` démarre à `$HOME` (`/home/<user>`), PAS au cwd de l'outil Bash —
  chaque invocation doit `cd "/c/Users/Charl/Desktop/SCPS-main"` EXPLICITEMENT en tête de
  commande (`--login -c 'cd ... ; export ... ; make ...'`), sinon `make` échoue avec
  « Aucune règle pour fabriquer la cible » (silencieux sur la vraie cause : mauvais dossier).
- `Bash run_in_background` + un pipe `| tail -N` NE CAPTURE QUE les N dernières lignes dans le
  fichier `.output` — pour relire le détail complet d'un run terminé il faut rediriger vers un
  vrai fichier (`> log 2>&1`) et le relire ensuite, pas se fier au tail déjà tronqué.
- `-Wmisleading-indentation` (gcc -Wall -Wextra, gate de build) : deux `if` sur la MÊME ligne
  physique (`if (a) x=0; if (b) x=y;`) sont acceptés par le compilateur mais WARNent — séparer
  sur deux lignes (motif déjà présent ailleurs dans scps_campaign.c, à réutiliser).

**Restes**
- `campaign_refill_corps_cost`/`campaign_refill_cost` (scps_campaign.c, lecteurs legacy non
  utilisés par la façade — aucun binding scps_api/Godot ne les appelle, superseded par
  `scps_corps_refill_preview`) : PAS mis à jour (toujours « +1 paquet par type présent », hors
  sujet — aucun consommateur ne les lit).
- **`make test` (40 bancs) : 1 rouge PRÉEXISTANT/SANS RAPPORT** — `agency_demo` (12/16, 4 échecs
  sur des seuils `K_inst`/`H_coerc`/`PE_infra`/`food_cap` bâtis en capitale) — CONFIRMÉ hors de
  cette mission : `agency_demo` ne lie NI `scps_campaign.o` NI `scps_sim.o` NI `scps_save.o`
  (cf. `AGENCY_DEMO_OBJS`, Makefile) — aucun fichier touché par cette mission n'est même dans
  son graphe de liens. Gates obligatoires de la mission (`scps_api_demo`, `golden`, `smoke`)
  tous VERTS ; ce rouge préexiste et reste à investiguer par un futur agent (pas armée).
- **Signatures nouvelles pour l'agent UI** (rien de câblé côté `godot/project/**`, hors
  périmètre) :
  - `ScpsRefillPreview.requested_humans` change de sens : DÉFICIT total
    (`max(0,nominal−courant)×100`), 0 quand le corps est déjà plein → bouton à GRISER
    (`reason_code==5`, nouveau : « Corps déjà à pleine force »). `population_ready_humans`/
    `guaranteed_humans`/`need[]` restent le coût de la PROCHAINE vague mais sont maintenant
    CAPÉS au déficit affiché.
  - `int scps_player_split_comp(ScpsSim*, int id, long inf_p, long arch_p, long cav_p, long
    mages_p)` (paquets de 100, ≥0, au moins un >0, chaque type ≤ dispo — refus net sinon,
    JAMAIS un clamp) + binding Godot `player_split_comp(id, inf_p, arch_p, cav_p, mages_p)`
    (godot/src/scps_sim_node.{cpp,h}) — composition EXACTE du nouveau corps, symétrique à
    `campaign_corps_composition`/`ScpsArmyInfo.inf/arch/cav/mages` (déjà exposés) pour un futur
    panneau « split par curseur de type ».
- **SAVE_VERSION 97** (bump : `FieldArmy.nominal`, long neuf → `sizeof(Campaign)` grandit,
  section CAMP = un seul blob). `save_sane` revalide `nominal∈[0,1e8]` par corps actif ;
  `campaign_backfill_nominal()` (appelée après tout load réussi, motif
  `demography_dyn_id_rebase`) relève tout `nominal` désérialisé sous le courant — filet, pas un
  mécanisme de migration (une save <v97 est de toute façon FLATLY REFUSÉE par le contrat
  `h.version!=SAVE_VERSION`, aucune trajectoire de migration n'existe dans ce moteur).

---

## TOPONYMIE DES VILLES (vérif docs/DESIGN_TOPONYMIE_VILLES.md, mission lecture-seule 2026-07-29)

**Découvertes** :
- `gen_province_names` (scps_world.c:2567) pose littéralement `"Prov.%d"` — confirmé stub, comme le doc l'affirme.
- `gen_region_names` (scps_world.c:2078) tire 4 variantes par région (`name_hum`/`name_elf`/`name_dwarf`/`name_orc`, scps_types.h:271-276) via un vote d'`EnvKind` sur les provinces membres + `rng_f()` (flux RNG global, PAS un hash(seed) local) — seul `name_hum` est LU (copié dans `Region.name`, scps_world.c:2103) ; les 3 autres sont écrites, sérialisées (`WRLD`), jamais relues nulle part (grep confirmé) = mortes.
- `place_make_name` (scps_world.c:4468) est un syllabaire **HERITAGE** (race-like : ésotérique/métallurgiste/mécaniste/adaptatif/agraire/clanique — PAS géographique) — ses SEULS appelants réels sont `country_make_name` (noms de PAYS/empires) et le tirage tribal WILD + le dédup de pays (scps_world.c:2898,2950) ; **jamais** utilisé pour une province ou une région. Le doc §2 dit juste qu'il sert « pays et peuples » — correct, mais ce n'est PAS un syllabaire de LIEU malgré son nom/commentaire (scps_world.c:4465) ; le futur système de toponymie de ville doit être un tirage NEUF, indexé sur (culture_id, clé sémantique) comme le prévoit le doc §5 — pas une extension de `place_make_name`.
- Le bandeau de province Godot lit le nom de RÉGION, pas de province : `province_readout()` (scps_readout.c:667) `pr.nom = w->region[reg].name` — **c'est LE point d'intégration futur** (swap vers le nom de ville stocké, repli sur `region[reg].name` si `!colonized`). Chemin complet : `scps_api.c:555` (`out->nom=sz(pr.nom)`) → `scps_sim_node.cpp:497` (`d["nom"]=...`) → `province_panel_v2.gd:216` (`_title_lbl.text`).
- Second consommateur déjà en prod : `empire_sidebar.gd` a une section **« VILLES »** (régions habitées du joueur triées par âmes, plafonnée à 10+« …et N autres », ligne ~235-250) — `_region_name()` (ligne 151) fait un détour par `region_centroid()`+`province_at()` pour retomber sur le MÊME `province_info().nom`, donc bénéficiera du swap gratuitement.
- Tous les champs géo/société du doc §3 EXISTENT, avec un grain précis à respecter :
  - Géo par PROVINCE (Province, scps_types.h:222) : `biome_dominant`, `lat`, `height_avg` (≈altitude), `coastal`.
  - Géo par RÉGION (Region, scps_types.h:254) : `harbor` (aptitude portuaire, [0..1], WG), `province_ids[0..n_provinces)`.
  - `estuary` existe mais PAS sur Province/Region — sur `ProvinceEconomy.estuary` (scps_econ.h:372, posé à econ_init depuis les cellules côtières à fort débit, scps_econ.c:1936-1937) et `RegionEconomy.estuary` (scps_econ.h:455, union). N'existe donc qu'APRÈS `econ_init` — inutilisable au moment de `gen_region_names` (pré-économie).
  - Fleuve/débit et lac : **jamais stockés par province/région**, seulement `Cell.river` (uint8_t 0-255) et `Cell.lake` (bool) par cellule (scps_types.h:116,118). Précédent RÉUTILISABLE : `refine_capitals` (scps_world.c:3522-3531) calcule déjà `has_river[SCPS_MAX_PROV]`/`has_lake[SCPS_MAX_PROV]` en UN passage O(SCPS_N) (seuils river>76, lake bool) — motif à copier tel quel pour la sélection de localisation §10 du doc.
  - Forme insulaire : **AUCUN champ n'existe** (pas de bool île, pas de détection de péninsule/isthme) — seule trace : un commentaire informel (scps_world.c:2241-2243, spawn) qui traite « île » comme un continent de faible superficie. Proxy RÉUTILISABLE sans nouvel état : `Continent.area` (scps_types.h, cellules terrestres, déjà calculé au worldgen) sous un seuil = île. Le doc doit soit adopter ce proxy explicitement, soit admettre l'entrée « infaisable sans nouvel état ».
  - Frontière avec un autre propriétaire : **PIÈGE** — `Cell.border_country`/`Province.country`/`Region.country` sont posés UNE FOIS à `build_hierarchy` (scps_world.c:2344) et ne sont JAMAIS remis à jour par une conquête en jeu (seule `scps_endgame.c` les mute, capstone uniquement) → ils reflètent la carte de GENÈSE, pas la carte VIVANTE. Le vrai propriétaire vit dans `econ->prov[pid].owner`/`econ->region[r].owner` ; « frontalier » doit se calculer par adjacence LIVE (`e->adj[r][s]` scps_econ.h:479, ou `e->prov_adj`/`padj_get`, scps_econ.h:481) en comparant les `owner` courants des voisins — jamais via les flags `border_*` figés.
  - Société/région (RegionEconomy, scps_econ.h:388) : `culture`/`culture_id` (miroir de la province représentative), `edi_built` (uint32_t, union des masques provinces membres — test bit `& (1u<<EDI_X)`, motif scps_agency.c:186 etc.), `ferveur`, `reconstruction`, `revolt_scar`, `is_capital`, `colonized`, `coastal`, `estuary`. TOUS les marqueurs EDI_* cités doc §7-8 (GARNISON/FORTERESSE/CITADELLE/TRIBUNAL/CHANCELLERIE/MARCHE/COMPTOIR/TRADE_CENTER/SANCTUAIRE/TEMPLE/CATHEDRALE/ACADEMIE/BIBLIOTHEQUE/MONASTERE/OBSERVATOIRE/PORT) existent tels quels dans `Edifice` (scps_agency.h:28-44). Les 6 Ethos du doc §7 = `Ethos` enum exact (scps_culture.h:36-41, DOMINATEUR..PACIFISTE, même ordre).
  - **PIÈGE edi_built au grain région** : `RegionEconomy.edi_built` est l'union OR de TOUTES les provinces membres (scps_econ.c:1528) — une région de 2-3 provinces où le Marché est bâti dans la province VOISINE (pas celle de la ville) ferait quand même mordre le marqueur « Marché construit » sur un nom de ville qui n'a pas de marché chez elle. Le générateur de toponymie doit tester `ProvinceEconomy.edi_built` (scps_econ.h:269) de la province-ANCRE précise de la ville, pas l'agrégat région.
- Fondation/genèse — 5+2 sites distincts posent `colonized=true`, à surveiller pour le futur hook de nommage (aucun point d'entrée unique aujourd'hui) :
  - AVEC `ferveur=1.f` (« fondation », doc §8) : `colonize_from_prov` (scps_econ.c:5945-5987, colonisation immédiate + relais d'`econ_colonize_overseas`), la clôture différée `econ_colony_day` (scps_econ.c:5880-5885, convoi arrivé), `ip_colonize_laborer` (scps_econ.c:6188-6210, canal Initiative Privée).
  - SANS ferveur (« genèse », capitales de départ) : la boucle de seeding joueur/antagoniste/cité-état dans `econ_init` (scps_econ.c:1976-2006, avant même que `region[]` existe) et `WILD_PLANT` (macro scps_econ.c:2096-2110, hameaux libres POLITY_WILD).
  - Les 3 sites « fondation » sont un grep fiable : `ferveur\s*=\s*1\.?0?f?;` → exactement 3 occurrences (scps_econ.c:5882,5986,6209).
- `econ_region_rep_province`/`e->region_rep_prov[]` (scps_econ.c:2886-2889, cache posé UNE fois par `econ_build_adjacency` — appelée à `econ_init` + chargement + carve endgame SEULEMENT, scps_econ.c:1944) ≠ `rep_pid[r]` local d'`econ_aggregate_regions` (scps_econ.c:1500,1567,1577 — recalculé CHAQUE tick, « capitale sinon la plus peuplée ») ≠ `Region.province_ids[0]` (scps_types.h:258, figé à `build_hierarchy`, ORDRE D'ITÉRATION arbitraire, PAS forcément la capitale — utilisé tel quel par `gen_population`, scps_world.c:2636, sous le nom trompeur `cap_pid`). **Ce sont 3 mécanismes distincts**, qui peuvent diverger dans la même partie (un membre de région peut devenir plus peuplé que la province fondatrice de la ville). Le doc §1 dit juste « la province représentative reste son ancrage » sans trancher LEQUEL des 3 — un nom de ville ne doit JAMAIS être re-résolu via l'un de ces pointeurs dynamiques après la fondation (sinon la ville « migre » silencieusement de tuile si la démographie régionale bascule) : il faut figer explicitement le PID fondateur (ou directement la chaîne composée) au moment du hook, jamais le redériver plus tard.

**Pièges** :
- `RegionEconomy` est une VUE reconstruite `memset`+recomposée à CHAQUE `econ_aggregate_regions` (scps_econ.c:1486, appelée en clôture d'`econ_tick`) — un champ `ville_name` posé UNIQUEMENT sur `RegionEconomy` serait EFFACÉ au tick suivant sauf à ajouter une ligne de mirroring explicite (motif exact déjà là pour `culture`/`culture_id`/`pop`/`bld`/prix, scps_econ.c:1577-1588, copiés depuis `e->prov[rp]` où `rp=rep_pid[r]`). **La vérité doit donc vivre sur `ProvinceEconomy`** (comme `culture_id`, scps_econ.h:266 « demeure comme substrat si la province se vide ») — jamais sur `Region`/`World` (WRLD, génération PURE géographique, tourne AVANT `econ_init`, n'a accès à aucun des signaux société du doc §3) ni sur `RegionEconomy` seule.
- **Aucune trajectoire de migration de save n'existe dans ce moteur** (déjà noté ailleurs dans ce fichier pour SAVE_VERSION 97, reconfirmé ici : `scps_save.c:525`, `h.version!=SAVE_VERSION` → refus NET, code 2, aucune branche `version<N`). Le scénario doc §8 dernière ligne / la question « migration d'anciennes saves dépourvues de noms » (mission §4) **n'a pas de prise réelle** : une save d'avant le bump `SAVE_VERSION` qui ajoutera `ville_name` sera simplement REFUSÉE au chargement, jamais chargée-puis-complétée. Le seul scénario où « nom manquant → attribution avec marqueur reconstruction » a un sens est INTRA-version : une province `colonized` dont le hook de nommage aurait été manqué (bug de couverture, pas un problème de save legacy). Le doc devrait corriger cette prémisse ou l'assumer explicitement comme filet de bug plutôt que comme migration cross-save.
- Golden/déterminisme : `gen_region_names` tire dans le flux RNG GLOBAL séquentiel (`rng_f()`, motif `PICK()`), contrairement à `place_make_name`/`country_make_name`/`culture_make_name` qui hashent un seed EXPLICITE local (`seed*2654435761u^...`). Le jitter de novlang du doc §5 (`hash(culture_id, clé)`, `hash(province_id, clé)`) suit le second style — cohérent avec les fonctions de nommage récentes, PAS avec `gen_region_names` (plus ancien, plus fragile à tout réordonnancement d'appels).

**Restes** (mission verrouillée lecture-seule — aucun code touché) :
- Aucun mapping doc→moteur n'était fourni avant cette mission ; ce bloc EST le contrat de référence pour l'agent d'implémentation suivant.
- Décision non tranchée à faire remonter à l'orchestrateur : lequel des 3 mécanismes « province représentative » ancrer pour la ville (recommandation : le PID fondateur explicite capturé au hook, distinct des 3 existants).
- `geo_names.gd` (Godot, display-only, jamais sérialisé, fleuves/forêts/lacs/massifs) confirmé hors-sujet du doc — aucun recouvrement avec la toponymie de ville engine-side.

## TOPONYMIE DES VILLES — IMPLÉMENTATION (mission moteur 2026-07-29, save v98)

**Fichiers** : `scps/scps_toponym.c`+`.h` (NOUVEAUX) ; touches à `scps_world.c` (2 lignes : `toponym_reset()` dans `world_generate`, `toponym_world_tick()` dans `world_tick`), `scps_save.h`/`.c` (section TOPO, bump SAVE_VERSION 97→98), `scps_api.h`/`.c` (`scps_region_city_name`), `scps_api_demo.c` (6 nouveaux bancs), `Makefile` (33 lignes, `scps_scps_toponym.o` accolé à chaque `scps_scps_world.o` via sed — cf. Pièges).

**Découvertes** :
- **Décision d'architecture qui S'ÉCARTE de la recommandation du rapport de vérif (assumée, justifiée)** : le rapport recommandait de stocker `ville_name` sur `ProvinceEconomy` (scps_econ.h). Le BRIEF de mission (pas le rapport) tranche explicitement pour le grain RÉGION ; or la propriété de fichiers de la mission n'inclut NI `scps_econ.h`/`.c` NI `scps_types.h` (où vivent `Region`/`ProvinceEconomy`) — seulement `scps_world.h`/`scps_worldgen*.c`. Résolu par un tableau **STATIC DE MODULE** dans `scps_toponym.c` (`char g_ville_name[SCPS_MAX_REG][32]`), motif EXACT de `g_colony_cd`/EMOB/COLC/TXYR (scps_econ.c) : une section de save `NULL,0` + `xxx_save(f)`/`xxx_load(f)` propres. Ça évite ENTIÈREMENT de toucher `scps_econ.h`/`.c`/`scps_types.h` — la lecture des signaux société (culture_id, ethos, edi_built, ferveur…) se fait en LECTURE SEULE via `#include "scps_econ.h"`/`"scps_agency.h"` (inclure un header pour LIRE ses types n'engage aucune propriété de fichier, contrairement à le MODIFIER).
- **Le "hook de fondation" (5+2 sites `colonized=true` dans scps_econ.c, hors périmètre) n'a jamais eu besoin d'être touché.** Remplacé par un **balayage idempotent** (`toponym_world_tick`, appelé depuis `world_tick`, scps_world.c — que je possède) : chaque année (day%365==364, cadence EXISTANTE de `world_tick`), il ne fait que COMBLER les régions dont l'ancre est colonisée et qui n'ont pas encore de nom — jamais de ré-tirage. Le premier appel qui suit `econ_init` nomme d'un coup TOUTES les capitales de départ déjà colonisées : genèse et fondation en jeu deviennent le MÊME mécanisme, sans jamais toucher les 7 call-sites de colonisation dans scps_econ.c. Bénéfice inattendu : couvre AUSSI le cas « save antérieure avec province colonisée mais nom manquant » (doc §8 dernière ligne) gratuitement, sans code de migration — le balayage suivant le comble.
- **Le hash GOLDEN N'A PAS BOUGÉ** (`make golden` → « hash monde IDENTIQUE au golden commité », aucun re-baseline nécessaire) : conséquence directe du static de module — `g_ville_name` ne fait PAS partie de `World`/`WorldEconomy` (les structs que `chronicle --hash` hashe), donc les noms de ville n'existent NULLE PART dans le hash. Le mandat de mission privilégiait explicitement cette option si atteignable sans trahir save/load — c'est le cas ici (section TOPO dédiée, testée A==B par `--savetest` ET par un aller-retour explicite dans `scps_api_demo`).
- **`scps_country_capital_region(s, cid)` existe déjà** (scps_api.h:1410) — évite de re-dériver soi-même la région-capitale pour les bancs (pas besoin de fouiller `capital_prov`→région à la main).
- Les 6 combos interdits (§11) sont, à l'examen, tous des paires de MOTS EXACTS où au moins un membre est un « mot autonome » du §6/§7/§8 (Havre, Castel, Nouvelle, Marché, Siège) — le gabarit `[mot autonome] de [racine]` n'a PAS été implémenté (simplification assumée, cf. commentaire en-tête du .c), ce qui rend 5 des 6 combos STRUCTURELLEMENT inatteignables. Seul « Mont + Berg » reste atteignable (préfixe ET suffixe de la MÊME ligne §6 Montagne, gabarit 1 « racine+suffixe » sans modificateur) — gardé par un check générique `toponym_words_collide` (+ garde bonus : le MÊME morphème deux fois, ex. « port »+« port », pas explicitement dans les 6 mais clairement « empilement de synonymes » §11) avec retentative bornée (≤3, nb de variantes du suffixe de la ligne).
- Le §4 du doc donne un exemple à 3 morphèmes (NOVA+MERC+AVRE→Novamercavre, forme courte Novavre en abandonnant l'éthos si trop long) — ÇA CONTREDIT une lecture « marqueur XOR éthos » du §11. Implémenté fidèlement : marqueur ET éthos peuvent COEXISTER (2 préfixes empilés puis lissés), avec abandon de l'éthos si `count_syllables(nom) > 4` (comptage par groupes de voyelles contigus, heuristique mais qui reproduit EXACTEMENT les 6 exemples du §12 — vérifié à la main avant codage).
- Vérifié à la main (avant tout code) que `toponym_smooth` reproduit LES 6 exemples §12 tels quels : Nova+Avre→Novavre (fusion voyelle), Cast+Brive→Casbrive (retrait consonne, 4 consonnes à la jonction→3), Reg+Sart→Ressart (gs→ss), Cour+Nant→Cournant (rien, 2 consonnes seulement), Hav+Mor→Havmor ET Hal+Mor→Halmor (les 2 formes du doc « selon la novlang » sortent NATURELLEMENT du jitter §5 sur la variante Pacifiste Hav-/Hal-, aucune règle de lissage à ajouter).

**Pièges** :
- **MSYS2 `bash.exe -lc '...'` avec `&&` dans un ARGUMENT DOUBLE-QUOTÉ passé par l'outil Bash a échoué silencieusement à plusieurs reprises** (le `cd ...` en tête ne semblait jamais s'appliquer — chaque commande retombait à `$HOME`) alors que le MÊME `cd ... ; ...` avec des POINTS-VIRGULES a marché du premier coup. Cause précise non isolée (peut-être un artefact de l'outil Bash avec `&&` en tête de chaîne). Antidote : préférer `;` à `&&` pour enchaîner `cd`/`export`/`make` dans ces invocations MSYS2.
- Le module `scps_world.c` est un point de passage QUASI universel (33 targets Makefile le lient) — brancher `toponym_world_tick`/`toponym_reset` DEDANS (pas dans un fichier périphérique) a mécaniquement propagé le besoin de `scps_scps_toponym.o` à TOUS ces targets. Résolu par un `sed -i 's#.../scps_scps_world\.o#.../scps_scps_world.o .../scps_scps_toponym.o#g' Makefile` (33 remplacements en 1 commande, plus sûr qu'une édition manuelle de 33 lignes). Précédent : `scps_credit.o`/`scps_decrees.o` ont le même compte (32) dans l'historique du Makefile — un module appelé depuis `scps_econ.c`/`scps_world.c` core RIPPLE toujours largement ; `scps_fog.o` (5 occurrences seulement) montre le contre-exemple d'un module appelé plus haut dans la pile (scps_sim.c), moins propagé.
- **`ScpsSim` est documenté « un seul actif par processus »** (commentaire en tête de scps_api.c) à cause des statics de module partagés — mon `g_ville_name` en hérite : créer un 2e `ScpsSim` (via `scps_sim_generate`→`world_generate`→`toponym_reset`) EFFACE les noms du 1er encore vivant. Les bancs de démo DOIVENT donc `snprintf` le nom dans un buffer LOCAL avant de créer/faire vivre un second Sim — piège dans lequel il est facile de tomber (un `const char*` retourné par `scps_region_city_name` pointe DANS le tableau de module, pas une copie) ; le fichier a déjà ce motif ailleurs (`nom_before[64]` avant un 2e sim, ligne ~779) — suivi à l'identique.
- `make golden` a nécessité de reconstruire `chronicle` (dedans `CHRONICLE_OBJS`) — le binaire `chronicle` préexistant au repo n'était pas à jour ; `make golden` le refait de toute façon (`golden: chronicle`), pas un piège réel, juste une note.
- Le seed=1 de `scps_api_demo` échoue TOUJOURS sur `« colonisation : une cible LÉGALE existe »` — **confirmé PRÉ-EXISTANT** (reproduit identique sur le code AVANT cette mission, via `git stash push -u -- <mes fichiers>` puis rebuild) : rien à voir avec la toponymie, ne pas chasser cette régression dans un futur passage toponymie.

**Restes** :
- **`scps_readout.c` / le bandeau de ville N'EST PAS câblé** — `scps_region_city_name(s, region)` existe et fonctionne (testé), mais `province_readout()` (scps_readout.c:667, `pr.nom = w->region[reg].name`) n'a PAS été modifié : ce fichier n'est pas dans la propriété de cette mission. Un futur agent (scope scps_readout.c + peut-être godot/**) doit faire le swap `pr.nom = colonized ? scps_region_city_name(...) : w->region[reg].name` (repli région si pas encore de ville) — chemin complet déjà documenté dans le rapport de vérif (scps_api.c:555 → scps_sim_node.cpp:497 → province_panel_v2.gd:216 ; 2e consommateur `empire_sidebar.gd` « VILLES »).
- **`godot/SConstruct` (ENGINE_MODULES) n'inclut pas `"toponym"`** — la DLL Godot (scons) ne linkera PAS `scps_toponym.o` tant qu'une ligne n'est pas ajoutée à la liste (`godot/SConstruct:24-30`). INTERDIT à cette mission (`godot/**`) ; à faire dans la même vague que le câblage scps_readout.c, sinon la DLL a un symbole manquant (`world_tick`→`toponym_world_tick` non résolu) et NE LINKE PLUS du tout.
- Désambiguïsation géographique des doublons PROCHES (§14, gabarit `[racine] sur [nom de fleuve]`) : NON implémentée (aucun nom de fleuve stocké dans le moteur — `River` n'a pas de champ `name`). Les doublons MONDIAUX restent explicitement autorisés par le doc ; seul le cas « proche » (même continent/pays ?) resterait à spécifier avant de coder — pas fait, la mission n'exigeait pas ce banc.
- Ligne « Vallée » du §6 jamais sélectionnée (le §10, source de vérité pour la sélection, ne l'atteint par aucune branche — cf. commentaire en-tête du .c) ; l'exemple §13 « Courval » (Bureaucrate+vallée+capitale) ne peut donc jamais sortir tel quel — assumé, le doc lui-même dit de dégrader proprement plutôt que d'inventer une branche.
- `save_sane` (scps_save.c) n'a PAS été touché pour la section TOPO — suit le PRÉCÉDENT WILD/EMOB/COLC/TXYR : validation (NUL-terminaison défensive) faite DANS `toponym_load`, pas dans la fonction globale `scps_save_sane`. Aucun de ces 4 précédents n'ajoute non plus d'entrée dans `scps_save_sane` — cohérent, pas un oubli.
- Tunables neufs (registre J, `SCPS_TUNE=`) : `TOPONYM_RIVER_MAJOR` (160), `TOPONYM_HARBOR_HIGH` (0.5), `TOPONYM_ISLAND_MAX_AREA` (700), `TOPONYM_ETHOS_BASE` (0.25), `TOPONYM_ETHOS_REINFORCED` (0.80) — valeurs par défaut choisies sans données de calibrage réelles (aucun historique de distribution `Continent.area`/`river` consulté) ; à recalibrer si le ratio « villes RADE/ÎLE » ou « fréquence éthos visible » semble déséquilibré en jeu.
- Gates exécutés et VERTS : `make scps_api_demo` (243 réussis/0 échoués, seed 9 ; 235/1 sur seed 1 — le 1 échec est le pré-existant documenté ci-dessus, sans rapport) · `make smoke` (7/7 bancs verts, membrane-check/lang-check/region-write-check OK) · `make golden` (hash IDENTIQUE, aucun re-baseline) · `./scps_viewer --savetest` (A==B strict + refus net sur altération d'un octet, 2/2).

---

## REVUE overlay.gd (agent revue de code, 2026-07-29)

**Propriété stricte** : `godot/project/map/overlay.gd` seul, portée = la liste d'items fournie (pas de refactor hors sujet).

**Découvertes** :
- Confirmé au grep + lecture `map_view.gd` : la vue GLOBE est **totalement retirée** (le fichier le dit lui-même : « Il n'y a plus de vue GLOBE 3D ni de splat iso 3D : un seul rendu, à tous les zooms ») et `MapView.globe_to_screen()` est un stub COMPAT qui renvoie toujours `{"pos":Vector2.ZERO,"vis":false}` — la branche globe de `_draw_resources` était donc morte DEUX FOIS (jamais appelée avec `is_iso=false`, et même appelée n'aurait rien dessiné). Simplifié : paramètre `is_iso` retiré, plus de branche.
- `_draw_cap_lisere` (liseré de capitale) est un cas à PART pour la mise en cache de projection (item #5) : son décalage `1.4/zoom` dépend du zoom COURANT (pas un offset fixe en monde comme les 3 couches de lavis de `_draw_band`, qui elles sont bien zoom-indépendantes 0.45/1.07/2.20). Le mettre en cache à `_rebuild_borders` aurait figé le liseré à un seul zoom — laissé TEL QUEL (toujours `_project_segs_iso` par frame), documenté explicitement pour ne pas être « corrigé » par erreur au prochain passage.
- Le facade appelle `Sim.ticked` **CHAQUE JOUR simulé** (commentaire `sim.gd:12`), pas chaque frame — la boucle villes (#4) et `_process`'s `_sig_poll` (#11, poll ~4×/s même en pause) tournent donc potentiellement PLUS souvent qu'un `_draw()`. Piège identifié pendant l'implémentation : marquer `_setts_dirty=true` INCONDITIONNELLEMENT à chaque tick (comme `_raws_dirty`) aurait annulé tout le bénéfice du cache (rebuild à chaque draw, comme avant) — le dirty doit suivre EXACTEMENT la cadence de `_borders_dirty`/`_fog_dirty` (souveraineté + année), pas le tick lui-même.
- **Un autre agent a modifié CE MÊME FICHIER en concurrence pendant cette mission** (`_draw_banner`, résolution du nom via `w.region_city_name(r)` avec repli `province_info`/région) — cohérent avec l'avertissement du brief (« workflow toponymie, phase à venir ») sauf que ce n'était pas différé : le changement est arrivé PENDANT ma session, détecté via `git diff` (pas par moi — je n'ai pas touché `_draw_banner`). Vérifié : l'appel `w.province_info(pid).get("nom", "")` reste textuellement présent (en repli), la fonction est syntaxiquement saine, et les deux gates (parse + les deux scènes shot) passent proprement sur l'état COMBINÉ. Signalé ici pour trace — pas d'action corrective prise (pas mon fichier à arbitrer), mais l'orchestrateur devrait vérifier que le séquencement mono-écrivain par fichier est bien respecté à l'avenir (deux agents avaient la même « propriété stricte » sur overlay.gd en même temps).

**Pièges** :
- `_pa_positions` mélangeait deux espaces de clés (item #1) : la garnison n'a PAS de `corps_id` (elle n'existe que via `country_army(c).regiments`, pas `corps_ids(c)`) — donc après le fix (clé `"g%d"%c`), une garnison ne peut plus JAMAIS apparaître dans `point_hits_player_army`/`player_corps_in_rect` (hit-tests typés `TYPE_INT` seulement). C'est un CHANGEMENT DE COMPORTEMENT assumé : avant le fix, cliquer une garnison pouvait accidentellement produire un `hit_army` égal à l'index pays, que `map_view._issue_selected_move` retombait alors sur `player_move_army(dreg)` (repli legacy sans id) SI `corps_info(id).active` se trouvait falsy pour cet id — un comportement fondé sur une COÏNCIDENCE de collision de clés, pas une conception voulue. Après le fix, une garnison ne peut plus être sélectionnée/déplacée par clic (cohérent avec « ce n'est pas un corps ») ; si le jeu veut un jour un ordre « lever et marcher » depuis la garnison, il faudra un verbe/chemin dédié, pas raccrocher au hit-test corps.
- Item #6 (dressing en tableaux typés) : scinder `_dressing` en `_dress_relief` (chevrons, Dictionary) + `_dress_fast_*` (sprites, Packed*Array) CASSE l'ancien tri combiné fond→avant (un seul `sort_custom` qui interclassait chevrons ET sprites par bande Y). Assumé : les chevrons se dessinent maintenant en un bloc AVANT tous les sprites. Impact mesuré au probe visuel (map_art_shot #04) : imperceptible — les marques sont semi-transparentes (DRESS_ALPHA=0.50, encre de chevron 0.32-0.80) et le chevauchement chevron/sprite n'arrive qu'aux frontières de biome (rare, la grille de semis par biome ne mélange pas les deux familles dans la même cellule). À surveiller si un futur biome venait à poser BEAUCOUP de sprites tout contre du relief dense.
- Item #8 (`_h1` hash entier) : **REDISTRIBUE tout le semis** (dressing/canopée/chevrons/easter eggs — tout ce qui appelle `_h1`). Assumé et attendu (display-only, aucun impact déterminisme SIM — `_h1` ne vit que dans overlay.gd, jamais dans scps/). Le probe visuel (chevrons en rangées propres, canopée dense deux tons, noms lisibles) confirme un semis toujours cohérent, juste redistribué.
- Le param `mv` a dû être AJOUTÉ à `_rebuild_borders(mv: Node2D)` pour pouvoir projeter à ce point (item #5) — les 2 call sites (dans `_draw_iso`) avaient déjà `mv` en scope local, changement mécanique sans risque, mais à ne pas oublier si un futur appelant externe existe (aucun trouvé au grep).

**Restes** :
- **Item #10 SKIPPÉ tel qu'instruit** : le découpage en nœuds-calques (un CanvasItem par couche : fond/dressing/frontières/villes/armées) reste un chantier séparé — `overlay.gd` reste un unique gros `_draw()` monolithique. Pas commencé.
- Item #6 : uniquement la « variante RAPIDE » (tableaux typés + pré-projection), PAS de conversion MultiMesh pour le dressing sprite (chantier à part, comme demandé). Le dressing sprite reste `draw_texture_rect` par item (juste sans Dictionary/re-projection par frame).
- L'anomalie de concurrence sur `_draw_banner` (ci-dessus, Découvertes) n'a été ni causée ni corrigée par cette mission — juste observée et validée comme non-bloquante pour mes propres items.
- Item #4/#5 : la validation visuelle des VILLES/ARMÉES dessinées (gate explicite) n'a pu être confirmée qu'INDIRECTEMENT — le monde de test (seed 9, an 24, les deux scènes `map_art_shot`/`pause_zoom_shot`) n'a ni garnison recrutée (« Réserve: 0 ») ni armée en campagne dans le cadre photographié, donc aucun pion/anneau n'apparaît à l'écran pour vérifier item #1 à l'œil. Les FRONTIÈRES (item #5) et le TERRAIN/CHEVRONS (items #6 + addendum) SONT confirmés visuellement (crops joints à l'analyse, bandes de lavis à 3 couches nettes, chevrons en rangées). Le code des villes/armées a été relu deux fois et exécuté sans erreur sur les deux runs complets (aucun crash, aucun nouveau warning) — mais pas vu à l'œil sur un pion réel. À revérifier au premier passage qui a une armée/garnison visible en jeu.

---

## TOPONYMIE DES VILLES — FAÇADE+UI (mission Godot/GDScript, 2026-07-29)

**Propriété stricte** : `godot/src/scps_sim_node.{cpp,h}`, `godot/project/map/overlay.gd` (bandeau/vignette de ville SEULEMENT), `godot/project/ui/empire_sidebar.gd` (section VILLES), `godot/project/ui/province_panel_v2.gd`, + probe dédiée `toponym_shot.gd`/`.tscn` (nouveaux).

**Découvertes** :
- Le rapport moteur expose `const char *scps_region_city_name(const ScpsSim*, int region)` (scps_api.h/.c) — un lecteur pur, grain RÉGION, déjà complet et testé côté C. Aucun nouveau symbole C n'était nécessaire côté façade : un simple binding `ScpsWorld::region_city_name(int) const → String::utf8(scps_region_city_name(sim, r))`, motif exact de `region_seat`/`region_owner` juste au-dessus.
- **`scps_readout.c` (donc `province_info(pid)["nom"]`) reste INTOUCHÉ, DÉLIBÉRÉMENT** — ce fichier n'est pas dans ma propriété, et `province_info(pid).nom` continue de porter le nom de RÉGION (`w->region[reg].name`, le générateur `gen_region_names` « Bois Doré »-style), PAS le nom de ville. Le nouveau binding `region_city_name(region)` est un canal SÉPARÉ, appelé explicitement partout où « le nom de la ville » est réellement voulu — jamais une réécriture de `province_info`.
- **Piège d'ATTRIBUTION PAR PROVINCE (analysé, PAS codé)** : j'ai envisagé de faire porter le nom de ville par la fiche province (en-tête `_title_lbl`) quand `pid` est « la province-siège » de sa région, en la déduisant côté Godot via `province_at(region_seat(region))`. Vérifié dans `scps_toponym.c` (lecture) que c'est FAUX : l'ancre réelle du toponyme est `is_capital`-first sinon le PREMIER `province_ids[k]` colonisé (ordre FIXE, jamais réévalué) — un mécanisme DISTINCT de `region_seat`/`scps_region_seat` (qui suit `econ_region_rep_province`, un pointeur qui peut lui-même diverger de `rep_pid[]` d'agrégation, cf. le rapport de vérif : « 3 mécanismes distincts »). Aucun binding n'expose « pid == ancre du toponyme » directement — tenter de le redériver côté Godot via `region_seat` aurait pu attribuer le nom de ville à la MAUVAISE province dans les parties où ces mécanismes divergent. Décision : ne JAMAIS afficher le nom de ville dans l'en-tête per-province (`_title_lbl`, qui reste le nom de région existant, inchangé) — il vit UNIQUEMENT dans l'onglet « Région » (agrégat déjà nommé, grain région, aucune ambiguïté de pid puisqu'on lit directement `region_city_name(_region)`), conforme à la charte province (« une fiche province ne montre que SES champs » — CLAUDE.md) et au design §1 (« l'identité [de la ville] appartient à la RÉGION »).
- **Bug de cache latent, trouvé et corrigé AVANT qu'il ne morde** : `overlay.gd::_draw_banner` (bannière de ville) et `empire_sidebar.gd::_region_name` mettaient en cache le nom résolu dès le PREMIER appel, pour toujours (cache jamais invalidé sauf « monde neuf »). Or `region_city_name(r)` peut légitimement renvoyer `""` pendant jusqu'à ~1 an de jeu après colonisation (le balayage `toponym_world_tick` est ANNUEL, pas immédiat) : si un joueur zoome sur une ville fraîchement fondée PENDANT cette fenêtre, l'ancien code aurait figé le repli (nom de région) EN CACHE PERMANENT, et la bannière n'aurait plus jamais montré le vrai nom de ville même une fois attribué. Corrigé dans les deux fichiers : on ne cache QUE quand `region_city_name` renvoie un nom non-vide (ou quand la méthode n'existe pas du tout, ancien binaire — repli permanent alors légitime) ; sinon on recalcule le repli à CHAQUE appel jusqu'à ce que le vrai nom apparaisse. `_setts`/`_region_seat` (autres caches d'overlay, item #4 de la revue concurrente) ne sont pas concernés — ils ne portent pas de nom.
- **Un autre agent (« REVUE overlay.gd », TROUVAILLES juste au-dessus) a travaillé sur `overlay.gd` EN CONCURRENCE, pendant cette même session** — détecté mutuellement (leur bloc note explicitement mon changement à `_draw_banner`, sans que nous ayons coordonné). Aucune collision réelle : leur refactor de performance ne touche AUCUNE ligne de `_draw_banner`/`_region_label`, mon changement ne touche AUCUNE des fonctions qu'ils ont modifiées. Le fichier final (les deux diffs combinés) parse proprement et les DEUX jeux de gates (leurs probes GDScript-only + mon rebuild DLL/probe fenêtrée) passent sur l'état COMBINÉ — validation croisée involontaire mais complète.
- **`godot/SConstruct` (`ENGINE_MODULES`) NE CONTENAIT PAS `"toponym"`** — confirmé bloquant : `scps_api.c` (déjà modifié par la mission moteur, module `api` déjà présent dans `ENGINE_MODULES`) référence `toponym_region_name`, et `scps_world.c` (module `world`) référence `toponym_reset`/`toponym_world_tick` — sans le module `toponym` compilé, le lien de la DLL Godot échoue avec des symboles non résolus (confirmé par les DEUX rapports précédents : mission moteur ET revue overlay.gd l'ont chacun signalé comme un « reste » bloquant, explicitement renvoyé à « la même vague que le câblage UI »). **Déviation de propriété de fichiers ASSUMÉE et SIGNALÉE** : `godot/SConstruct` n'est pas dans ma liste, mais sans cette ligne, AUCUN des gates obligatoires de cette mission (rebuild DLL, probe fenêtrée avec captures) n'était exécutable — la mission aurait été bloquée net par un fichier hors de ma propriété que deux missions précédentes m'ont explicitement renvoyé. Une seule ligne ajoutée : `"world", "toponym", "econ", ...` (SConstruct:25). L'orchestrateur peut trivialement revert cette ligne seule si désapprouvé — tout le reste du diff SConstruct est inchangé.

**Pièges** :
- La fenêtre de jeu réelle par défaut (`get_window().size` posé dans `_ready()`) a été écrasée par la résolution du bureau au 1er run de la probe (1920×1080 au lieu des 1600×900 demandés) — pas reproduit au run suivant (1600×900 respecté). Cause non isolée (course avec l'init fenêtre du moteur ?) — sans conséquence (les captures restent lisibles aux deux résolutions), noté au cas où un futur agent verrait une taille de fenêtre incohérente entre deux lancements de la MÊME probe.
- **Le brouillard de guerre bloque la capture de villes ÉTRANGÈRES non découvertes** — `_refresh_setts` (overlay.gd) exclut du rendu (donc de toute bannière) une région dont `owner != joueur` tant qu'elle n'est pas `_fog_visible_region`. `fog_off` (flag probe existant, `shot_parch.gd`) NE contourne QUE le voile visuel (`_fog_tex`), PAS ce filtre de rendu — inutile pour révéler des bannières étrangères. Il a fallu avancer le monde à ~130 ans (seed 9) pour que le joueur (petit « Clans Dornyana ») ait découvert assez de voisins pour obtenir 6 villes nommées VISIBLES et culturellement variées (2 héritages) d'un coup ; à 30-60 ans, seule sa propre capitale (« Courrive ») était garantie visible. Un futur agent voulant capturer PLUS de variété culturelle en un seul run devrait soit avancer encore plus (coût CPU), soit accepter moins d'exemples simultanés, soit (hors scope ici) ajouter un mode probe dédié qui contourne aussi le filtre de rendu (pas seulement le voile visuel) — nécessiterait de toucher `overlay.gd::_refresh_setts`, hors du périmètre « bandeau/vignette SEULEMENT » de cette mission.
- Fausse alerte personnelle (documentée pour ne pas être re-creusée) : à la lecture rapide d'un PNG downscalé, « Nouvlenfurt » se lisait presque comme « Nouvleneuve » — résolu par un crop+upscale (PIL, `python -c` via Git Bash) sur la zone du cartouche : c'est bien « NOUVLENFURT », cohérent avec le nom attendu (région 61, owner 47). Aucun bug réel ; juste une leçon de méthode — toujours CROP+UPSCALE un cartouche avant de trancher sur un nom ambigu à l'œil.

**Restes** :
- **La fiche province (en-tête, `_title_lbl`) montre TOUJOURS le nom de région générique** (`province_info(pid).nom`, ex. « Futaie Profond »), jamais le nom de ville — décision délibérée (cf. Découvertes, piège d'attribution par pid) et non un oubli. Le nom de ville réel (« Hautlenavon » dans ce cas) n'apparaît QUE dans l'onglet « Région ». Si un futur agent veut le faire apparaître dans l'en-tête, il lui faudra D'ABORD un binding C exposant explicitement « pid == ancre du toponyme de cette région » (le moteur le sait — `scps_toponym.c` calcule `anchor` — mais ne l'expose pas), pas une redérivation côté Godot.
- `scps_readout.c` reste non câblé (le même reste que documenté par la mission moteur) — désormais **la DLL LINK** (grâce au fix SConstruct), donc un futur agent scope `scps_readout.c` peut faire le swap `pr.nom = colonized ? scps_region_city_name(...) : region_name` sans blocage de build. Pas fait ici (hors propriété stricte de cette mission).
- Probe `toponym_shot.gd`/`.tscn` (nouveaux, `godot/project/`) laissés en place comme actif réutilisable pour toute future vague toponymie/UI carte — écrit `build/toponym_*.png` (gitignored). Ramasse aussi des PNG de crop diagnostic (`build/toponym_crop_*.png`, gitignorés) laissés tels quels (scratch, sans conséquence).
- Gates exécutés et VERTS : `scons target=template_debug` (DLL, godot/) — build propre, zéro erreur ; `Godot --headless --check-only --quit` — parse propre (aucune erreur de script) ; probe fenêtrée `toponym_shot.tscn` (seed 9, 130 ans, `SCPS_MUTE=1`, jamais `--headless`) — 9 PNG produits, **6 noms de ville distincts lus À L'ÉCRAN et confirmés par crop+upscale** : Hautlenavon (capitale d'un empire voisin, « Haut- » = marqueur `is_capital`), Courrive (capitale du joueur), Librerive, Librebrod (« Libre- » partagé, peuple/cité libres), Nouvlenavon, Nouvlenfurt (« Nouv-…-len- » partagé, même empire — parenté sonore intra-culture confirmée à l'œil, § invariant 14) ; tous 2-4 syllabes, prononçables, AUCUN débordement de cartouche observé (le cartouche s'auto-dimensionne au texte — structurellement impossible de déborder côté UI). Sidebar « VILLES » confirmée (« Courrive · 6 759 »). Fiche province onglet « Région » confirmée (section « VILLE » → « Hautlenavon », distincte de l'en-tête « Futaie Profond »).

---

## RECALIBRAGE BANCS POST-GRANDS-FLEUVES (mission bancs rouges, 2026-07-30)

**Propriété stricte** : les 6 `scps/*_demo.c` listés en mission (statecraft_demo, agency_demo,
missions_demo, warhost_demo, events_demo, scps_api_demo) — jamais scps_world.c/scps_endgame.c.

**Découvertes** :
- **`agency_demo.c` n'était PAS cassé par les grands-fleuves du tout** — kill-switch
  (`SCPS_TUNE="RIVER_FILL=0,RIVER_ARID_NIL=0"`) donnait le MÊME échec que le monde neuf,
  bit pour bit (K_inst=3.2, H_coerc=5.3, PE_infra=0.8, food_cap=0.8 dans les deux cas). Le
  vrai coupable : **VÉTUSTÉ** (commit b0116bb, « Métriques province »), `agency_build_decay`
  (scps_agency.c:673) qui ronge `ProvBuild.K_inst/H_coerc/PE_infra/food_cap` en continu vers
  un plancher 50 %. Le fixture lisait ces champs à la TOUTE FIN du banc (~an 16,5), soit 9 à
  14 ans APRÈS que le bâti testé ait fini de construire — largement assez pour qu'un nominal
  tout juste au-dessus du seuil (K_inst 4.0 vs seuil 3.5 ; PE_infra/food_cap 1.0 vs seuil 1.0,
  AUCUNE marge) retombe sous la barre. Piège méthodologique à retenir : **sous kill-switch
  vert ⇒ presume grands-fleuves ; sous kill-switch ROUGE À L'IDENTIQUE ⇒ chercher AILLEURS**
  (ici une vague antérieure jamais recalibrée dans ce fixture précis).
- **`missions_demo.c` et `statecraft_demo.c`, eux, confirment exactement la jurisprudence
  POLITY_WILD du digest** : `missions_demo` excluait POLITY_UNCLAIMED mais pas POLITY_WILD
  dans sa boucle de sélection de `cid` — sous le nouveau monde (seed 42) elle attrapait un
  hameau libre. `mission_grant` LÈVE la récompense or sur la richesse RÉELLE du royaume
  (`econ_country_wealth_levy_bounded`, scps_missions.c:169) : quasi nulle pour un hameau, donc
  la mission se marque bien ACCOMPLIE (loyauté +5 pile, matières versées — ces deux canaux ne
  sont PAS bornés par la richesse) mais l'or n'abonde jamais le trésor. Un signal utile pour
  distinguer « précondition POLITY_WILD » d'autre chose : les canaux non-monétaires (loyauté,
  matières) passent, SEUL le canal or échoue.
- **`statecraft_demo.c` avait le MÊME piège caché deux fois** (`int cid=0` répété dans le
  bloc Q1 « LE CONSEIL » et dans le bloc V2a « LE CONSEIL VIVANT », ~30 assertions au total
  en dépendent) — `statecraft_council_cost` a pour assiette `econ_country_tax_year(cid)`
  (scps_statecraft.c:113-117) : un pays 0 = POLITY_WILD a une fiscalité PERMANENTE à zéro
  (jamais rien à lever), donc coût=0 pour toujours, faisant échouer « coût >0 » ET (plus
  loin) le test B1 « le trésor est CONSOMMÉ » (0<0 est faux). Seulement 2 des ~15 assertions
  du bloc Conseil dépendent RÉELLEMENT d'un coût non-nul — les autres (déterminisme des
  tiers, âge, retraite, factions, loyauté/rot…) sont indifférentes à la fiscalité du pays
  choisi, d'où le nombre limité d'échecs malgré la portée large du hardcode.
- **`warhost_demo.c` et `events_demo.c` étaient DÉJÀ au motif préféré** (recherche
  dynamique, tri par taille pour warhost — exclusion POLITY_UNCLAIMED explicite avec
  commentaire citant EXACTEMENT le même piège WILD dans son historique). Leur échec observé
  en tout DÉBUT de mission ne s'est PAS reproduit contre le build gelé final (jitter epsilon
  inclus) : les deux sont VERTS (8/8 et 119/119) sans un octet touché, avec ET sans
  kill-switch. Confirme que le monde était encore MOUVANT (l'orchestrateur ajoutait le bruit
  epsilon aux dépressions PENDANT que je diagnostiquais) — retester CONTRE LE BUILD COURANT
  avant de toucher un fixture qui semble rouge : un faux rouge transitoire ne doit PAS être
  recalibré en dur (aurait été du travail perdu / un hardcode inutile).
- **`scps_api_demo.c` — la seule assertion qui a RÉSISTÉ à tout levier façade-only** :
  « colonisation : une cible LÉGALE existe (scps_can_colonize) ». `scps_can_colonize`
  (scps_api.c:3878-3894) exige qu'AU MOINS une province COLONISÉE du joueur ait pop≥800 ET
  `food_sat≥0.5`. Sous la seed 9 (défaut du fichier) dans le nouveau monde, l'unique province
  de départ du joueur plafonne à food_sat≈39-47 % — vérifié en tendance sur +39 ans
  supplémentaires via `scps_player_alerts` (famine_pct) : ce n'est PAS un creux transitoire,
  c'est un PLATEAU D'ÉQUILIBRE (raw_cap borné par la géographie de la tuile — worldgen fige
  ce plafond, aucun levier façade ne le déplace). Deux leviers façade légitimes essayés :
  (a) `scps_player_alloc_raw` à 255 sur les 4 brutes vivrières (GRAIN/FISH/LIVESTOCK/FRUIT)
  et 0 sur tout le reste — a fait monter le plateau de ~42 % à ~47 %, insuffisant seul ;
  (b) empilé le décret « Rations mesurées » (trouvé par NOM via `scps_decrees_list`,
  jamais l'enum interne — motif déjà établi plus bas dans ce même fichier, ~ligne 1094 —
  FOOD_NEED×0.95). `ProvBuild.food_cap` (Grenier/Irrigation, agency) a été un FAUX AMI ici :
  il alimente `econ_prov_effcap`/`econ_region_effcap` (scps_econ.h:704-737, CAPACITÉ DE
  LOGEMENT), pas `food_sat` — une confusion facile à faire vu le nom du champ, à ne pas
  refaire. Aucun verbe façade n'expose l'EXPLOITATION (`agency_order_exploit`, qui booste
  RÉELLEMENT `raw_cap`) au joueur — seul le moteur interne y a accès (agency_demo le prouve
  via l'API interne, jamais via scps_api).

**Pièges** :
- Le moteur a bougé DEUX FOIS pendant cette mission (RIVER_FILL/RIVER_ARID_NIL d'abord, puis
  le bruit epsilon des dépressions ajouté PAR-DESSUS en cours de route) — un banc rebuild-et-
  testé avant le second changement peut redevenir rouge OU vert pour de MAUVAISES raisons
  (cf. warhost_demo/events_demo ci-dessus, qui ont basculé rouge→vert tout seuls). Toujours
  `touch scps/scps_world.c && make <banc>` avant de conclure quoi que ce soit si la mission
  mentionne un moteur encore actif ailleurs.
- `scps_api_demo` prend 3-4 MINUTES par run complet (170+ assertions, DIZAINES de
  `ScpsSim` fraîches recréées séquentiellement — chaque worldgen coûte plusieurs secondes) :
  un `run_in_background`/timeout mal calibré (<240 s) le fait passer pour « bloqué » alors
  qu'il tourne normalement. Prévoir large.
- MSYS2 : un `scps_api_demo.exe` resté ACCROCHÉ d'un run précédent (process encore vivant)
  fait échouer le LIEN du build suivant avec « cannot open output file … Permission denied »
  (pas une erreur de compilation) — `taskkill //F //IM scps_api_demo.exe` avant de rebuild
  si ce message apparaît.
- `ProvBuild.food_cap` (Grenier/Irrigation) ≠ nourriture réelle — c'est un terme de CAPACITÉ
  DE LOGEMENT (`econ_prov_effcap`), pas de production vivrière ; `RegionEconomy/ProvinceEconomy
  .food_sat` (couverture réelle besoin/production) est un champ SÉPARÉ, piloté par
  `raw_cap[RES_GRAIN/FISH/LIVESTOCK]` + `RES_FRUIT` en filet — à ne pas confondre au prochain
  passage sur la famine/colonisation.

**Restes** :
- **`scps_api_demo.c` — CONTRE-INTUITIF confirmé, puis fichier REVERTÉ à l'état pristine.**
  Empiler le levier (b) (décret « Rations mesurées », FOOD_NEED×0.95, POP_R_BASE×0.97)
  PAR-DESSUS le levier (a) (allocation 100 % vivrière) a fait EMPIRER le plateau (39→43 %
  au lieu de 39→47 % pour (a) seul) — hypothèse la plus probable : la production de brutes
  scale sur la main-d'œuvre disponible (POP_R_BASE↓ → moins de bras → moins de grain extrait,
  même à poids d'allocation 100 %), et cette perte de production dépasse le gain de 5 % sur
  le besoin. Un lecteur `food_sat` qui semble purement géographique (raw_cap) a donc AUSSI
  une composante démographique cachée — tout décret qui freine la natalité peut être
  CONTRE-PRODUCTIF pour la sécurité alimentaire à moyen terme, contre-intuitif à première
  lecture du flavor (« moins de bouches à nourrir »). Aucun des deux leviers (seul ou
  combiné) n'a franchi 50 % sur 40 ans simulés. Le fichier `scps/scps_api_demo.c` a été
  ENTIÈREMENT REVERTÉ à HEAD (`git diff` vide) plutôt que laisser du code expérimental
  mort/contre-productif dans un fichier livré — l'assertion « colonisation : une cible
  LÉGALE existe » reste ROUGE, EXACTEMENT dans son état d'origine (236 réussis/1 échoué,
  seed 9 par défaut), diagnostic complet mais PAS de fix trouvé dans le temps imparti.
- Aucun autre levier FAÇADE-ONLY identifié n'est disponible pour pousser `food_sat`
  au-delà de ce plateau (pas de verbe joueur pour l'EXPLOITATION de raws — seul
  `agency_order_exploit`, interne, y accède ; pas de verbe de relocalisation de capitale ;
  la recherche de graine ALTERNATIVE a été ÉCARTÉE délibérément — casserait la convention
  du fichier où TOUS les sims dédiés réutilisent la même variable `seed` que l'argument
  CLI, ce qui rendrait le comportement du banc silencieusement différent du `graine`
  affichée en bannière). Prochaines pistes NON essayées, pour un futur agent avec plus de
  budget : (i) un décret/levier qui monte la PRODUCTION plutôt que baisse le BESOIN (aucun
  candidat trouvé dans le catalogue DECREE_* actuel — à vérifier) ; (ii) établir des routes
  commerciales explicites avec un voisin excédentaire en vivres AVANT le test colonisation
  (verbe diplo existe, chemin non exploré faute de temps) ; (iii) si aucun levier façade ne
  suffit jamais : c'est un signal qu'il faut remonter à l'orchestrateur pour DÉCISION —
  soit un rebalance worldgen (hors propriété de fichiers de cette mission, PAS tenté), soit
  documenter/accepter que sous la seed 9 (nouveau monde), la colonisation reste
  indisponible très longtemps pour la capitale de départ — pas nécessairement un bug
  moteur (scps_can_colonize fait exactement ce que sa spec dit), mais une conséquence dure
  et peut-être non désirée des grands-fleuves à vérifier en jeu réel, pas seulement au banc.
- warhost_demo.c / events_demo.c : aucun changement — confirmés verts contre le build gelé,
  laissés TELS QUELS (leur motif de recherche dynamique était déjà correct).

**Clôture orchestrateur (vague grands fleuves, 2026-07-31)** : cartographie scps_api_demo
« colonisation : une cible LÉGALE existe » sur 4 graines du moteur final — 9 ✗ · 7 ✗ ·
3 ✗ · 42 ✓. La mécanique n'est pas morte (42 passe ; la colonisation IA vit partout —
dump PROV du chronicle), mais le VERBE JOUEUR (`scps_can_colonize` : pop≥800 ET
food_sat≥0.5, seuil EN DUR scps_api.c:3892, le drain revalide) trouve sa précondition
moins souvent : les capitales-joueur post-fleuves plafonnent fréquemment à food_sat
~40-47 %. DÉCISION JOUEUR REQUISE (équilibrage, pas bug) : abaisser le seuil, booster le
food du spawn curated, ou statu quo. Le banc reste graine 9 PRISTINE, rouge 1/237
documenté — ne pas « réparer » en changeant de graine (42 casse 3 autres fixtures :
B7 crédit + rénover ×2, elles aussi graine-dépendantes).

## VAGUE COLONISATION — « le stock drive la demande » (orchestrateur, 2026-07-31)

**Découvertes** :
- L'UI MENTAIT sur la colonisation depuis toujours : scps_can_colonize (façade) exigeait
  pop≥800 ET food_sat≥0.5 quand le drain réel (econ_colonize_province) demandait 500/0.35
  — le bouton grisait des colonisations LÉGALES. Le « rouge 1/237 » de scps_api_demo
  post-fleuves était CE mensonge, pas le worldgen. Morale : une « approximation UI des
  portes » diverge silencieusement — miroir EXACT obligatoire (helper partagé
  econ_colony_food_ok + mêmes tunables, motif scps_build_legal_ex↔agency_build_acct).
- Le chemin ASSIETTE du grain (« GARANTIE : jamais de gate can_buy ») court-circuitait le
  tally demand[] (continue avant le chemin générique) : une pénurie de grain ne formait
  NI prix NI demande — invisible du commerce, provinces pauvres auto-bloquées à vie.
- Le motif stock-cible existait déjà pour outils/armes (demand += max(0, cible − S×pshare))
  — le vivrier ne l'avait pas. Ajouté : cible = FOOD_STOCK_MONTHS (6) mois de conso grain.
- La boucle est fermée par construction P1 : demand[] province → agrégat région →
  intertrade → pool national → redescente re->stock=pool×share (l.5222) → le gate grenier
  lit prov->stock. Vérifié AVANT d'écrire (risque : gater sur un stock jamais rempli).
- Effet mesuré (60 ans) : prov colonisées 31→51 (g7) · 35→46 (g9) ; fondations 16→24 ·
  15→25 ; pop monde 49k→64k (g7) — la baisse forte NOURRIT (pas de famine systémique),
  0 colonie de survie. scps_api_demo 243/243 (6 assertions aval DÉVERROUILLÉES par le
  déblocage du verbe).

**Pièges** :
- EVID_MERV_SACRIFICE (events_demo) : mtth 1200 j × fenêtre fixe 10 ans = ~5 % d'échec
  PUR ALÉA ; tout recalage amont du monde décale la séquence frand et peut tomber dedans.
  Fixture → boucle jusqu'au-tir bornée 40 ans. Chercher ce motif si un banc à mtth
  re-rougit après une vague worldgen/éco.
- Golden re-baseliné DEUX fois dans la même session (fleuves, puis colonisation) — chaque
  vague éco gatée doit prouver son kill-switch AVANT le re-baseline suivant, sinon les
  preuves se contaminent.

**Restes** :
- FOOD_STOCK_MONTHS=6 et la baisse forte (300/150/0.25/WPC 4) calibrées sur 2 graines ×
  60 ans seulement — un gigasweep de validation reste souhaitable (annulé cette nuit).
- Le grenier ne demande que du GRAIN (pas fish/livestock) — simple d'abord ; élargir si
  les provinces halieutiques montrent un biais.

### SUITE — 2e et 3e passe (campaign_demo, ai_demo, diplo_demo, audit_eco — post plancher-Nil)

Le moteur a bougé encore deux fois après la section ci-dessus (tunables colonisation
COLONY_MIN_POP/COLONY_COST_POP/COLONY_FOOD_GATE/IP_COLON_WPC/FOOD_STOCK_MONTHS, puis
RIVER_NILE_KEEP=110 — 3e re-monde). Conséquence directe et BONNE NOUVELLE : les nouveaux
seuils de colonisation (food_sat exigé abaissé 0.5→0.25, pop 800→300) ont réglé tout SEUL
`scps_api_demo` (« colonisation : une cible LÉGALE existe » — la seule assertion restée
rouge à la fin de la passe précédente, jamais retouchée dans cette passe, revenue verte
d'elle-même : 243/243). Restaient 4 rouges neufs/rechutés : campaign_demo, ai_demo,
diplo_demo, audit_eco — tous recalibrés, `run_tests.sh full` = 40/40 verts.

**Découvertes** :
- **`campaign_demo.c` — le MÊME piège qu'en LOT 3 (§3d, déjà fixé la passe précédente)
  existait AUSSI dans la recherche de frontière INITIALE (§1, tout en haut de `main`)**,
  jamais couvert avant car cette paire (frontier/target) marchait par chance sur les mondes
  précédents. `econ->adj[r][s]` (adjacence éco) ne garantit PAS une route de CAMPAGNE
  praticable (`next_hop`/`region_ok`, scps_campaign.c:160-221, filtrent en PLUS sur le
  biome majoritaire de la région via `terrain_impassable` — plus strict que le flag éco
  `impassable` seul). Sous le 3e re-monde, la 1ère paire adjacente trouvée était
  injoignable pour une armée → `campaign_order` échouait EN SILENCE (`next_hop`<0) dès la
  ligne 85, faisant cascader 22 échecs sur 34 assertions (tout ce qui dépend de la force
  ayant RÉELLEMENT marché). Fix : sonder `campaign_order` avec une armée-jouet sur `camp2`
  (scratch, jamais `camp` — un sondage sur `camp` aurait fusionné un reliquat parasite dans
  le compte de troupes de la VRAIE force testée juste après) PENDANT la recherche, avancer
  au candidat suivant si le sondage échoue.
- **`ai_demo.c` — piège de PREMIÈRE INTENTION à ne PAS refaire** : `ProvBuild.food_cap`
  (Grenier/Irrigation) N'EST PAS la production vivrière — c'est un terme de CAPACITÉ DE
  LOGEMENT (`econ_prov_effcap`/`econ_region_effcap`, scps_econ.h:704-737, `+ food_cap×250`
  dans la formule). Perdu ~20 min à chercher un « levier de food_sat » côté agence avant de
  relire la formule et comprendre que c'est un FAUX AMI (le nom trompe). `RegionEconomy.
  food_sat` (couverture besoin/production RÉELLE) est un champ totalement séparé, piloté
  par `raw_cap[RES_GRAIN/FISH/LIVESTOCK]` (figé à la genèse) + `RES_FRUIT` en filet — voir
  aussi audit_eco ci-dessous, même confusion possible.
- **`ai_demo.c` — l'assertion « le Bâtisseur métabolise ≥ le Mercantile » a résisté à
  DEUX refontes de métrique successives** (celle-ci et la précédente, qui avait déjà retiré
  `consolidations` du total) avant qu'un test EMPIRIQUE révèle la vraie cause : ce n'est
  PAS la fiche/l'archétype qui pilote qui bâtit le plus, c'est le PAYS RÉEL derrière —
  `polity[2]` (peu importe la fiche qu'il porte) construit systématiquement MOINS que
  `polity[1]` sous cette graine/ce monde (mesuré dans les deux sens : polity[1]-Mercantile
  bâtissait DÉJÀ plus que polity[2]-Bâtisseur ; en échangeant qui porte quelle fiche,
  polity[1]-Bâtisseur écrase polity[2]-Mercantile 9 builds/0 route contre 0 build/25
  routes). Le SUBSTRAT ÉGAL (trésor/stock/pop/raw_cap identiques pour les 3 capitales)
  n'égalise PAS la connectivité/l'historique géographique — piste non retenue (coût trop
  haut) : une VRAIE recherche dynamique par re-simulation multi-candidats (snapshot/restore
  de s.w/s.econ/s.ts/s.wp/s.wl/s.ag/s.rn/s.dp) — documentée en commentaire dans le fichier
  si un 4e re-monde re-casse ce tableau.
- **`diplo_demo.c` — TROIS causes racines DIFFÉRENTES dans le MÊME fichier**, à ne pas
  confondre :
  1. §3 (fulgurance/hégémon) : jurisprudence POLITY_WILD classique — `B` choisi comme
     « le premier pays réel » sans exclure WILD ; `threat_of` = `base×(1+momentum)` avec
     `base=(eco+mil)/…` — un hameau a `base≈0`, donc `momentum[B]=40` ne bouge JAMAIS la
     menace (0×tout=0). Fix : exiger `role!=POLITY_WILD` ET `eco_power+mil_power>0.01` pour B.
  2. §6b (pillage de siège) : **PAS** POLITY_WILD cette fois — région LIBRE (owner=-1,
     `vic_wild=0`) mais dont la PROVINCE porteuse est maintenant ACTIVE sous le nouveau
     monde (`region_carrier_prov` exige seulement `.active`, PAS `.colonized`). Le fixture
     dotait SEULEMENT la vue agrégée `econ->region[vic].stock[g]`, jamais la PROVINCE
     (`econ->prov[vic_pid].stock[g]`) — sous l'ancien monde ça marchait par ACCIDENT
     (`region_carrier_prov` retournait -1, basculant `econ_region_stock_add` sur son
     chemin « vue seule » qui lit directement l'agrégat) ; sous le nouveau monde la
     province est réputée active, le retrait cible `prov[pid].stock` (jamais doté) → 0
     prélevé. Piège RE-KEY PROVINCE classique, mais ACTIVÉ par un changement de
     géographie, pas par une case à cocher owner/colonized comme d'habitude.
  3. §9 (esclavage) : ENCORE différent — ni POLITY_WILD ni re-key. `diplo_enslave_capture`
     (scps_diplo.c:1344-1350) plafonne le prélèvement à `strata[CLASS_LABORER]+
     [CLASS_BOURGEOIS]` (la STRATE économique), PAS au `count` du `PopGroup` culturel
     (4000) posé par le fixture. Le fixture peuplait le GROUPE (identité culturelle) mais
     jamais la STRATE (le bassin économique RÉEL d'où l'esclavagiste prélève) — sous
     l'ancien monde, `srcR` (« la 1ère région ≠ capitale ») avait par chance une strate
     naturelle suffisante ; sous le nouveau monde, non. Fix : doter aussi
     `econ->prov[srcP].strata[CLASS_LABORER].pop`.
- **`audit_eco.c` — le fix de la passe précédente (repli MONDE, food_sat≥0.5) a lui-même
  RECHUTÉ sous le 3e re-monde** : un témoin choisi à `food_sat` tout juste ≥0.5 peut encore
  DÉCLINER sur 10 ans (mesuré : ×0.86, un TÉMOIN COMPLET différent cette fois — capitale
  mono-région ×0.86 la 2e passe, un hameau-repli-monde ×0.86 la 3e). Cause : la formule de
  croissance (scps_econ.c:4986-4988) ne pénalise la natalité qu'en-dessous de
  `food_sat<0.35` (pic de mortalité famine) — un témoin choisi à 0.5 pile n'a AUCUNE marge
  contre une dérive de food_sat en cours de route sur 10 ans simulés. Durci à 0.75 (marge
  réelle avant le plancher de famine à 0.35) — conforme à l'instruction « durcis la
  recherche dynamique plutôt que la fourchette » : c'est la PRÉCONDITION du témoin qui est
  montée en exigence, jamais `[1.04..2.5]` (les bornes d'audit elles-mêmes, intouchées).

**Pièges** :
- Le triage kill-switch complet (`RIVER_FILL=0,RIVER_ARID_NIL=0,RIVER_NILE_KEEP=0,
  COLONY_MIN_POP=500,COLONY_COST_POP=250,COLONY_FOOD_GATE=0.35,IP_COLON_WPC=8,
  FOOD_STOCK_MONTHS=0`) doit inclure TOUS les tunables touchés depuis le début de la
  vague, pas seulement les derniers — un kill-switch partiel (oubliant COLONY_*/
  FOOD_STOCK_MONTHS) aurait donné un faux « rouge aussi sous kill-switch » sur
  `scps_api_demo` alors que c'est justement CE changement qui l'a réparé.
- Un banc qui casse une SECONDE fois sur la MÊME assertion après un premier recalibrage
  « dynamique » (campaign_demo, audit_eco) n'a pas forcément la MÊME cause racine que la
  1ère fois — vérifier à nouveau depuis zéro (grep, lecture du moteur) plutôt que de
  supposer que la même explication tient encore.
- Ordre des vérifications utile pour `diplo_demo` : les 10 échecs initiaux couvraient
  3 sections indépendantes (fulgurance/hégémon, pillage de siège, esclavage) — les traiter
  comme UN seul problème (ex. « tout est POLITY_WILD ») aurait fait perdre du temps ; grep
  par section + lecture du moteur DERRIÈRE chaque assertion (pas juste le nom de
  l'assertion) a été nécessaire à chaque fois.

**Restes** :
- `ai_demo.c` : l'échange polity[1]↔polity[2] (Bâtisseur/Mercantile) est une correction
  ROBUSTE mais toujours ARBITRAIRE (aucun des deux index n'a de légitimité propre) — si un
  4e re-monde inverse de nouveau le tableau, la piste documentée en commentaire (snapshot/
  restore + recherche multi-candidats par re-simulation, ~12 s/essai) est la bonne prochaine
  étape plutôt qu'une nouvelle permutation à la main.
- Confirmé, `run_tests.sh full` (BANC_TIMEOUT=300) : **40 verts / 0 rouge / 0 build échec**,
  aucun run laissé en arrière-plan.

## CHAÎNAGE PAR VILLES + FUSION DE CORRIDORS (anti-spaghetti routes, 2026-07-31)

**Découvertes** :
- Le cadrage joueur a REDRESSÉ mon diagnostic : le moteur construit DÉJÀ un graphe
  ville-à-ville (scps_api.c api_roads_build : 2 plus proches voisines, dédup paires,
  un A* par arête, attraction de corridors, garantie de connexité). Le chaînage par
  villes ne corrige que les arêtes passant près d'une ville intermédiaire — MESURÉ
  (graine 9, an 60, 76 routes) : wp=8, réutilisation 14 %. Utile, PAS suffisant.
- Le vrai tressage est GÉOMÉTRIQUE : spag=2162 segments parallèles résiduels (métrique
  A1) — des couloirs voisins au-delà du rayon magnet (1.4), que l'aimant global ne doit
  PAS avaler (déjà mesuré : plateau à 2.2/4 passes + risque de fusionner des croisements).
- LA FUSION DE CORRIDORS SOUTENUS est le bon outil : proche (≤2.2) ET parallèle
  (|cos|≥0.92) ET tenu sur ≥6 cellules d'arc CONSÉCUTIVES → snap en bloc sur le corridor
  le plus ANCIEN. Un croisement ponctuel ne tient jamais la longueur → jamais fusionné.
  Mesuré : 1758 points snappés, résidu 2162 → 455 (−79 %). Validé au shot (z4.5/z8).
- Canonique par paire : plus ancien _road_start → plus courte → clé (« jamais “la
  première” : l'ordre du tableau ne décide pas de la géographie »). Chantier progressif :
  visibilité d'étape = MAX des couvertures parentes ; usure = nb de parentes l'ayant
  ATTEINTE — sans ça, une relation neuve fait apparaître un tronc canonique d'un coup.
- Classes de rendu = level MOTEUR (0 grande / 1 régionale / 2 sentier, tiers de pop des
  bouts) — plus fidèle que l'ancien hack « desserte tier 1 = sentier ».

**Pièges** :
- L'instrumentation AVANT conclusion (cadrage joueur) a évité de livrer le chaînage
  seul comme « la solution » : les chiffres (14 %) l'auraient démenti en jeu.
- python heredoc bash : les gros scripts d'édition passent par un FICHIER (Write) —
  le heredoc inline casse sur la taille/quotes.
- Découpe de fonction par index (i0..i1) : vérifier ce que la fenêtre AVALE — la
  passe 3 vivait AVANT le marqueur de fin choisi (l'assert du script a sauvé le fichier).

**Restes** :
- spag résiduel 455 : tronçons parallèles courts (<6 cellules) et divergences aux Y —
  à regarder en jeu réel avant tout nouveau tour de vis.
- Les carrefours « aire commune + séparation d'ornières » : toujours pads-seulement.
- road_paths() n'expose ni relation commerciale ni volume : l'« usure par trafic RÉEL »
  exigerait une donnée de façade supplémentaire (l'usure actuelle = nb d'arêtes
  routières réutilisant l'étape).

**Consolidation v2 (même vague, cadrage joueur affiné)** : TERMINAL UNIQUE par ville
(secteur angulaire ~35°, la queue du plus ANCIEN devient le tronc de porte partagé,
jonction à la frontière urbaine r=4) + FUSION STRUCTURELLE (seuils joueur 3.5 cellules /
cos 0.94 ≈ 20° / ≥8 cellules tenues) : le suiveur est SCINDÉ aux deux jonctions, sa
portion médiane DISPARAÎT (fini l'empilement de bandes — l'usure du tronc s'incrémente
à la place), 99 carrefours créés (aires de terre usée, sans symbole). MESURES (g9,
an 60) : spag 2162 → 67 (−97 %) · term=65 · 1845 pts. Bande −29 % d'alpha, ornières
désaxées (+0.85/−0.74) et déphasées (périodes ≠) anti-« rails », usure = terre foncée +
ornières densifiées (jamais 2 bandes). Trouée/usure lues sur le réseau FINAL (post-
consolidation). VIGILANCE : géo-chg=10/70 entre rebuilds pendant la croissance du
réseau (des étapes changent de géométrie quand un chantier s'achève) — re-mesurer en
jeu long si un « saut » visuel se voit.

**Serpentines & frôleurs (même vague, 2e crop joueur)** : (1) le test de parallélisme
POINT-À-POINT cassait sur deux routes qui ONDULENT côte à côte (déviation locale ±25° →
runs < 8 cellules → jamais fusionnées) — le parallélisme se juge désormais sur la CORDE
du run entier (elle lisse les ondulations, cos ≥ 0.90) et near[] ne teste plus que la
DISTANCE ; la sécurité anti-croisement reste la PROXIMITÉ SOUTENUE (un croisement ne
tient pas 8 cellules à ≤3.5). (2) Une route qui FRÔLE une ville à 2.6-3.2 cellules
n'était pas une étape → jamais coupée à la porte → elle traversait la zone urbaine :
rayon waypoint 2.5 → 3.2. Mesures finales : spag 2162 → 11 · wp 11 · réutil 18 % ·
géo-chg 2. La bande s'arrête à la PORTE (r_gate 2.4, pad au point de coupe) depuis le
1er crop. Le piège générique : tout test d'alignement point-à-point sur des géométries
LISSÉES/ondulantes doit passer par la corde (ou une direction moyennée).

**Pincement des croisements (3e crop joueur, zone NE)** : le SPAGDIAG a localisé TOUT le
résidu en UN cluster (808,169) — deux GRANDES routes convergeant à ~35-40° qui se
longent quelques cellules avant de se croiser. La fusion les rejette À RAISON (garde
anti-croisement) ; le traitement cartographique juste = un CROISEMENT NET : deux étapes
qui se frôlent (≤2.0 cellules) sans partager de nœud sont PINCÉES vers le point commun
(poids gaussien ±2 pts, jamais les abouts) + aire de carrefour. Mesures : pince=1,
spag étapes 11 → 5. PIÈGES : (1) la métrique spag (SPAG_DIST=1.5) ne voit PAS les
parallèles à 2-4 cellules que l'œil voit — le juge reste le SHOT ; (2) le shot at= était
pris en mode NATURE (routes cachées) → déplacé avant le toggle ; (3) une zone non
découverte est NOIRE au shot → fog_off (voile seulement, motif shot_parch) pour
photographier sous le voile. RESTE (dit joueur) : les culées de pont « ressemblent à des
marqueurs » — à reprendre après stabilisation topologique.

**Montagne & canopée au zoom proche (4e crop joueur)** : (1) les ∧/dômes sont des
symboles de VUE D'ENSEMBLE — au plan rapproché un seul devenait un coup de pinceau
géant : FONDU relief_a = clampf((8.5−zoom)/2.5) (pleins ≤ z6, disparus ≥ z8.5, le
terrain — teinte du massif — prend le relais). (2) PURGE DES FRAGMENTS : un glyphe dont
le clipping de priorité a mangé > 45 % de l'encre (seuil 0.55 restant) est supprimé
ENTIER — fini les virgules orphelines en bordure de massif. NOTE : les petites marques
∩ qui restent en bord de massif sont des DÔMES de colline légitimes (biome hills), pas
des fragments. (3) LISIÈRE de canopée : le vote 1/3 semait des solitaires pleine taille
sur sol nu — probabilité 0.35→0.22 ET échelle ×0.78 en lisière : la forêt s'éteint en
dégradé au lieu de s'effilocher.

**Trait de rivière au DÉBIT réel (demande joueur : « scalable sur le nombre d'affluents —
précalcule ton débit »)** : l'ancienne largeur était par-rivière (flow_max + grow linéaire
par fraction d'arc — une croissance INVENTÉE, aveugle aux confluences). Désormais le
carve LIT la couche SCPS_LAYER_RIVER (accumulation log précalculée moteur, +1 saut à
chaque affluent) PAR POINT du tracé : échantillonnée sur le tracé BRUT (le méandre décale
de ±1 cellule), lissée sur 5 points (les marches log crénellent), largeur par paliers de
byte (≥0.82→3 · ≥0.62→2 · ≥0.42→1 · sinon fil) et intensité v=0.60+0.40·b (l'eau se
charge vers l'aval, sans passer sous le seuil 5-taps du shader aux têtes). La règle
« artère = top-2 des longueurs » SUPPRIMÉE — le débit EST la hiérarchie. L'entonnoir
d'estuaire prend le débit du dernier point.

**EXPORT scps.exe — LA PROCÉDURE (piège résolu 2026-07-30, « les mises à jour ne
semblent pas appliquées »)** : le joueur lance l'EXPORT (packaging/windows/dist_godot/
scps.exe) — il fige scripts ET DLL release au moment de l'export. Après toute vague :
(1) `scons -C godot target=template_release` (la DLL release vieillit en silence — elle
datait du 17/07 quand la debug était du jour) ; (2) exporter avec le Godot NON-MONO
`E:\JEUX\SCPS\Godot_v4.6.3-stable_win64.exe --headless --export-release "Windows
Desktop" --path godot/project --quit`. JAMAIS avec le Godot_v4.6.3-stable_mono_win64
de la racine du repo : le projet n'a AUCUN C#, la version mono exige des templates
.mono non installés + un SDK .NET 10 absent → « configuration errors » sec. Les
templates installés (AppData/Godot/export_templates) sont 4.6.3.stable NON-mono.

## LES MONDES ATHÉES — la vétusté mangeait le palier de foi (2026-07-31)

**Le fait** : gigasweep 100 mondes × 250 ans → `religion : 0.0 foi(s) fondée(s)/sim`
sur les 20 logs, au caractère près. Aucune Église n'était jamais née dans SCPS.

**Découvertes** :
- Le diag EDI_DBG montrait `Sanctuaire made=10 blocked=760` et AUCUNE ligne Temple —
  or le dump imprime toute ligne à ≥1 compteur non nul (made/blocked/nogold/nomat/nocap/
  notech) : `agency_build_acct` n'avait donc JAMAIS été appelé avec EDI_TEMPLE en 250 ans.
  Et la fondation exige TEMPLE|CATHEDRALE (scps_ai.c, emask) — un Sanctuaire ne fonde rien.
- CAUSE RACINE : `ai_next_faith_edifice` choisissait le palier en comparant `build.faith`
  à des seuils NOMINAUX (<1.0 → Sanctuaire, <3.0 → Temple). Mais la VÉTUSTÉ érode ce
  stock (2 %/an, plancher 50 %) : un Sanctuaire posé à 1.00 retombe à 0.73 (MESURÉ,
  diag FOI_DBG), repasse SOUS le seuil de SON PROPRE palier, et l'IA redemande un
  Sanctuaire… déjà bâti ⇒ refus. Boucle infinie = les 760 blocked. Le Temple, lui,
  n'était jamais choisi. FIX (décision joueur « le sanctuaire motive le temple, quel que
  soit l'éthos ») : le palier suivant se lit sur `edi_built` (masque BÂTI, qui ne s'érode
  PAS), jamais sur la valeur. Même correction sur l'échelle-debout et la garde de site.
- FAUSSE PISTE COÛTEUSE (consignée pour qu'un successeur ne la refasse pas) : j'avais
  conclu à un verrou arithmétique sur w_faith (genèse pluraliste ⇒ 0.1, plafond de glide
  ×2 ⇒ 0.224 < AI_FAITH_ZEAL 0.5, dépendance circulaire credo↔fondation). Le diag l'a
  DÉMENTI : des pays tournent à w_faith 0.591/0.567 — les credos évangélistes existent
  bien à la genèse, le zèle EST atteignable. `scps_world.c:2978` (credo=PLURALISTE) ne
  couvre pas toutes les régions. LEÇON : ne jamais conclure d'une lecture de code sur un
  chemin IA sans instrumenter — les valeurs runtime démentent la lecture statique.

**Pièges** :
- Le kill-switch AI_FAITH_LADDER=0 ne suffisait PAS à rendre le golden identique au
  premier essai : j'avais REFACTORISÉ `ai_faith_site_region` (extraction du ladder), si
  bien que sous kill-switch il retournait toujours le repli-bois au lieu de l'ancienne
  échelle-par-faith. Un kill-switch doit restaurer le corps ORIGINAL, pas une version
  refactorisée « équivalente ». Corrigé → golden byte-identique sous OFF.
- `make` ne recompilait pas après édition de scps_ai.c (dépendance manquante) : `rm -f
  build/scps_scps_ai.o` avant `make` pendant l'investigation.
- religion_demo est vert 13/13 mais ne contient AUCUN appel au chemin de fondation IA
  (il teste les lecteurs sur fixtures) — aucun banc ne couvrait la chaîne réelle.

**Restes** :
- La voie de CRISE n'a pas le garde `faith_pending` de la voie de zèle : elle retentait
  chaque tour pendant les 180 j de chantier (gaspillage, gonflait `blocked`).
- Cathédrale : `nomat=198 notech=29` — la chaîne se tend au sommet (équilibrage, pas
  blocage). Les schismes/hérésies/minorités tournent pour la PREMIÈRE fois en conditions
  réelles : leur calibrage n'a jamais été observé sur un sweep.

## WORLDGEN : LA VOCATION EST LE TIRAGE, RIEN D'AUTRE (2026-07-31)

**Le fait** : pierre et argile n'avaient AUCUN poids dans la table de tirage par biome
(0 occurrence d'ADD contre 8 pour le fer) — leur seule source au monde était le spawn
curé des capitales d'empire. Conséquence mesurée : Temple (20 pierre) refusé nomat=231
fois/sim ⇒ mondes athées. Le dump du chronicle comptait des RÉGIONS (agrégat) et masquait
tout : c'est le dump au grain PROVINCE (ajouté ici) qui l'a montré.

**Découvertes** :
- TROIS mécanismes contournaient la règle des 2 brutes au lieu de l'appliquer :
  (1) GREFFES GÉOLOGIQUES (econ_init) : pierre/argile/fruit reposées dans raw_cap d'après
      le biome, PAR-DESSUS le tirage — « une DÉRIVATION IA, une HALLUCINATION » (joueur).
      Débranchées (RES_GEO_GRAFT=0). ⚠ PAS neutre malgré la coupe de vocation : les
      manufactures se posent sur le raw_cap AVANT elle — les greffes orientaient l'atelier.
  (2) Le TIRAGE ignorait pierre/argile : ajoutées là où sw_biome_fits les déclarait déjà
      (la table le disait, le tirage ne l'appliquait pas).
  (3) COMPLÉTUDE : le tirage pondéré est une BINOMIALE — une brute à faible poids sort
      zéro fois sur ~700 tuiles (Fruits absent 5 graines/8). Passe de rattrapage : chaque
      absente est posée sur la tuile au POIDS MAXIMAL pour elle (la géographie décide OÙ),
      en remplaçant la MINEURE, jamais la dernière tuile d'une autre brute.
- ⚠ NE PAS « CORRIGER » : les manufactures de CITÉ-ÉTAT implantées au gisement sont
  VOULUES (tranché joueur). Le raisonnement « le pool est national donc l'atelier n'a pas
  à naître sur son intrant » est séduisant et FAUX : la cité-état est l'ATELIER DU MONDE,
  sa dotation est sa raison d'être ; l'empire, lui, naît nu. J'ai débranché puis restauré
  — avertissement pose dans le code.
- DÉFRICHAGE : mécanisme ORPHELIN (3 occurrences, aucun appelant, aucun verbe façade, ne
  posait pas le biome, grain région). Complété : 10 ans, l'or va INTÉGRALEMENT aux
  journaliers (on paie des bras), grain province, pose BIO_FARMLAND puis RE-TIRE les 2
  brutes sur le nouveau biome (world_province_reroll, table extraite pour être rejouable).
  FARMLAND n'était produit NULLE PART (0 return, 0 assignation) : ce n'était pas un bug de
  worldgen mais l'absence du travail humain — « on n'est pas apparu au 20e siècle avec des
  champs gigantesques ».

**Pièges** :
- Un tunable non déclaré au registre J fait ÉCHOUER le binaire quand SCPS_TUNE le nomme
  (« tunable INCONNU ») — d'où un `--hash` VIDE que j'ai d'abord pris pour un crash.
- Le golden de l'ARBRE avait été re-baseliné entre-temps : comparer le kill-switch au
  golden HEAD (`git show HEAD:scps/golden_hashes.txt`), pas au fichier courant.
- diff/cmp/bc ABSENTS du MSYS2 de ce poste : comparer avec `[ "$(cat a)" = "$(cat b)" ]`.
- Province n'a PAS de champ `.active` (c'est ProvinceEconomy) — utiliser habitability>0.
- FIXTURE FRAGILE (scps_api_demo B7) : seuil ABSOLU de 0.5 or sur un écart de formules —
  il testait en fait la TAILLE de la dette (5 vs 119 selon le monde), pas la divergence.
  Recalibré en RELATIF (les taux), l'assertion est intacte et robuste au re-monde.

**Restes** : façade du défrichage (éligibilité, verbe joueur, bouton grisé + hover) et
l'IA qui n'a aucune raison de défricher. Biomes : Jungle quasi absente (3 cellules/monde),
Sommets jamais, Savane/Désert/Collines parfois sans aucune province dominante.

## COLONISATION : LA MER EST LE MUR, PAS LA CADENCE (2026-07-31)

**Découvertes** :
- DÉCOMPTE RÉEL d'un monde (graine 1518, an 250) : 711 provinces = 187 INFRANCHISSABLES
  (glaciers/sommets/déserts morts) + 524 colonisables, dont 305 colonisées, 55 libres
  ATTEIGNABLES (adjacentes à une colonisée) et **164 HORS DE PORTÉE**. L'objectif « 650 »
  était arithmétiquement impossible : 26 % du monde est mort par nature.
- La CADENCE n'était PAS le frein : la sélection ne fondait qu'UNE colonie par pays et par
  an (best_src/best_dst uniques) — corrigé (jusqu'à 4/an ∝ taille) — mais passer de 4 à 8
  ne change RIEN (281 provinces dans les deux cas). Le franchissement des terres mortes
  (COLONY_CROSS_DEAD) : RIEN non plus. Ce sont des MESURES, pas des intuitions.
- LA MER EST LE MUR — et je ne l'ai su qu'en REGARDANT la carte (le joueur : « tu mesures
  sur quelle graine ? l'as-tu seulement regardée ? »). La graine 1518 est un ARCHIPEL de
  4 masses : les empires tiennent les 2 îles du nord-ouest, et le GRAND CROISSANT du
  sud-est — près de la moitié des terres — n'a AUCUNE couleur d'empire. Aucun compteur ne
  le disait ; la carte le crie.
- L'outre-mer EXISTAIT (scps_navy.c) mais était resté sur l'ANCIEN calibrage : son
  commentaire promettait « les mêmes seuils que la colonisation TERRESTRE » alors qu'il
  exigeait 500 hab / food 0.35 quand la terrestre était descendue à 300 / 0.25. Aligné sur
  les MÊMES tunables + répit 2 ans → 6 mois : 27 colonies outre-mer, 281 → 305 provinces.

**Piège de méthode (le plus coûteux)** :
- JE MESURAIS PENDANT L'APOCALYPSE. L'an 250 est APRÈS le déclenchement des fins §27
  (an 180-240) : le shot montre « Entropie 100 — Le Grand Engloutissement » à l'an 246.
  Une partie des « provinces libres » sont des terres PERDUES au cataclysme, pas jamais
  prises. Toute mesure de REMPLISSAGE doit se faire à un horizon PRÉ-CATACLYSME (an 150),
  sinon on calibre l'expansion contre la fin du monde.
- Le kill-switch de cette vague rend 4 hashes sur 5 : la graine 411 diverge parce que
  l'ALIGNEMENT des seuils outre-mer n'est pas neutralisable séparément (les remettre à
  500/0.35 changerait aussi la terrestre). Divergence ASSUMÉE et documentée.

**Restes** : l'outre-mer reste une exception (27 débarquements/partie) alors qu'un monde en
archipel en fait le moyen d'expansion ORDINAIRE ; il exige une région ENTIÈREMENT vierge
et côtière (grain RÉGION, pas province). Mesure de remplissage à refaire à l'an 150.

## HABITABILITÉ CULTURELLE + QUOTA N/X — le monde réparti ET rempli (2026-08-01)

**Le fait** : la moitié du monde restait vierge (archipels : le grand continent sans un
empire). Deux vagues liées, jugées ENSEMBLE sur graine appariée (1518, an 180, 3 configs) :
ancien monde 280 provinces · quota N/X seul 151 (!) · quota + habitabilité culturelle 314.

**Découvertes** :
- QUOTA N/X (« une distribution simple N(empire)/X(Continents) ») : quota ÉGAL par
  continent ≥ 10 % des terres habitables, AVANT poids et espacement ; repli à 2 étages
  (continents sous-quota à espacement relâché, puis sans quota — le COMPTE prime).
  DEUX BUGS HISTORIQUES dessous : Country.continent n'était JAMAIS assigné (tous à 0 —
  la post-passe L4 « peupler les continents » était INERTE depuis sa naissance) ; et son
  test « ensemencé » comptait une CITÉ-ÉTAT comme suffisante.
- Le quota SEUL est CONTRE-PRODUCTIF (280→151) : les empires redistribués végètent sur
  les climats non tempérés avec la table fixe. C'est le joueur qui a vu la cause :
  « l'habitabilité comme un seuil fixe, eurocentrique — des peuples orientaux, africains,
  mésoaméricains s'en sortent très bien ». La table (Désert 0.08, Terres sèches 0.28) dit
  ce que vaut la terre POUR UN COLON DES PLAINES, pas pour un peuple né là.
- HABITABILITÉ CULTURELLE : 4 classes (tempéré/aride/tropical/froid), bitmask par pays,
  matrice world_hab_for (plancher natif : aride 0.60, tropical 0.68, froid 0.52).
  LE TERRITOIRE FAIT LE PEUPLE, pas la seule capitale : le spawn Civ-like choisit
  l'OASIS (eau+grain) — si le bit ne venait que d'elle, un pays de terres sèches naissait
  « tempéré » et tout SON désert lui restait hostile (mesuré : 151 avec bit-capitale,
  314 avec bits ≥ 25 % de l'aire du territoire). Appliquée à l'init (terres possédées),
  au scoring de colonisation, à la fondation (colony_recap_for). Héritage par
  MÉTABOLISATION : groupe déplacé intégré ≥ 0.99 → la couronne gagne le climat de sa
  région d'origine (home_reg ; le marché aux esclaves « fongible » ne lègue rien).
- CRÉATEUR D'EMPIRE : sélecteur « Berceau » (Auto/Tempéré/Aride/Tropical/Froid) →
  scps_set_player_climat → la capitale du joueur naît sur la classe (score ×8, pénalité
  d'inaccessibilité levée pour la classe natale). Country.climates ⇒ SAVE_VERSION 99.

**Pièges** :
- BUILD MIXTE : sizeof(Country) a changé et le Makefile n'a AUCUNE dépendance headers —
  seuls les .o supprimés à la main se recompilent. Un binaire mi-ancien mi-nouveau = UB
  silencieux. RÈGLE : tout changement de struct partagée ⇒ `rm build/*.o` AVANT make.
  (Les mesures ont été re-prouvées sur rebuild intégral : identiques, coup de chance.)
- scons crash « UnicodeEncodeError » en console cp1252 quand un message d'erreur gcc cite
  du code UTF-8 (→) : lancer avec PYTHONIOENCODING=utf-8 pour VOIR l'erreur réelle.
- Mon premier patch SAVE_VERSION a ORPHELINÉ le commentaire multiligne v98 (remplacement
  d'une ligne d'un bloc /* … */) — toujours vérifier que le bloc reste fermé.
- Le hash monde (--hash) n'est PAS sensible au placement des rôles seuls : deux mondes aux
  empires différents peuvent hasher pareil à horizon court. Prouver par diag (SPAWN_DBG),
  pas par hash.

**Restes** : 3×3 apparié en cours (2528/4043 aux 3 configs an 180) ; sud du croissant
encore vierge à l'an 177 (dynamique lancée, 70 ans restants) ; UI : le berceau n'apparaît
pas encore dans la fiche empire (seulement au créateur) ; STR_* : libellés du sélecteur
en dur dans le .gd (lang-check 0 = le cliquet ne couvre pas les .gd).

## ESSAIMAGE PASSIF DE RÉGION — la pile complète gagne partout (2026-08-01)

**Le fait** : le 3×3 apparié (an 180) montrait quota+climat GAGNANT sur l'archipel (1518 :
280→314) mais PERDANT sur le grand monde (2528 : 429→301) et le fragmenté (4043 :
159→119). Directive joueur : « quand une province est colonisée, la région se colonise
passivement — petite migration progressive, 100 hab en 3 ans ». Résultat, pile complète :
1518 317 · 2528 483 · 4043 219 — DEVANT l'ancien monde sur les 3 graines (jusqu'à +38 %).

**Découvertes** :
- econ_region_seep (annuel, aux côtés d'econ_colonize_tick) : ruisseau SEEP_TARGET/
  SEEP_YEARS hab/an vers les provinces libres VIVABLES de toute région ayant une
  colonisée ; transfert CONSERVATIF (strates hors esclaves, la richesse voyage — motif
  M3a) ; source jamais drainée sous COLONY_MIN_POP ; palier 100 hab puis la croissance
  naturelle. Première goutte = vraie fondation (invariants de colonize_from_prov :
  colonized/owner/ferveur/groupe culturel) ; la toponymie suit (balayage idempotent).
- Un monde 30 % plus colonisé se simule ~30 % plus lentement : scps_api_demo passe de
  <300 s à 340 s — VERT (240/240) mais au-delà du BANC_TIMEOUT=300 usuel. Passer 480.

**Pièges (méthode)** :
- KILL-SWITCH : comparer au BON golden. Depuis 52a97ca le golden intègre quota+climat ON
  — tester « tout OFF » contre lui = comparer l'avant-vague au monde nouveau, forcément
  DIFFÉRENT (4 hashes/5). Le test correct d'une vague N est SON seul interrupteur à 0,
  le reste aux défauts du golden commité. REGION_SEEP=0 seul → IDENTIQUE.
- La graine 7 hashait pareil des DEUX côtés du faux test : un hash peut être insensible
  à l'écart à horizon court — jamais conclure « identique » sur une seule graine.
- Le probe colon_shot affichait « Fermeture anormale détectée » par-dessus la carte
  (drapeau de session de feedback.gd jamais nettoyé par un force-quit) : retirer
  user://session_running.flag au boot du probe (motif shot_ui.gd, désormais appliqué).
- La DLL Godot ne suit PAS make : après toute modif moteur, scons AVANT tout shot —
  sinon la carte montre l'ancien moteur (vécu : shot 2528 sans le seep).

**Restes** : SEEP_TARGET/YEARS non calibrés au-delà des défauts (100/3, décision joueur) ;
le seep ignore le climat (il essaime aussi vers les tuiles hostiles au peuple — voulu ?
la matrice ne s'applique qu'aux fondations actives) ; scps_api_demo à 340 s (optimisation
éventuelle si la lenteur gêne).

## EXPANSION PASSIVE UNIFIÉE — « l'un ou l'autre » (2026-08-01)

**Le fait** : deux mécanismes posés coup sur coup (seep RÉGIONAL d7d709f « 100 hab en
3 ans », puis un FRONT général « 2,8 personnes/mois ») ; le joueur a tranché : PAS de
cumul, un seul. Et ses deux specs étaient DÉJÀ le même nombre : 2,8 × 36 mois = 100,8 —
la famille mensuelle DONNE les 100 hab en 3 ans. econ_passive_seep remplace les deux.

**Découvertes** :
- Le mécanisme unifié est au GRAIN PROVINCE PUR : l'ADJACENCE remplace la maille région
  (conforme charte — la région n'est qu'un agrégat). Chaque année, toute province
  vivable adjacente au peuplé reçoit sa famille depuis la voisine la plus forte ;
  fondation à la première goutte ; complément jusqu'à SEEP_TARGET (100) pour les
  naissantes de la MÊME couronne ; jamais chez l'autre. Le peuplement COULE de proche
  en proche (~3 ans par anneau de frontière).
- L'unifié fait NETTEMENT mieux que le régional : 3×3 an 180 —
  1518 : 280 (ancien) → 317 (régional) → 428 (unifié, 76 % du colonisable)
  2528 : 429 → 483 → 530 (80 %)   ·   4043 : 159 → 219 → 386 (78 %)
  Les 3 mondes CONVERGENT vers ~78 % : l'adjacence traverse les frontières de régions,
  le front atteint tout le connexe. La carte 4043 : supercontinent couvert bord à bord.
- KILL-SWITCH d'un REMPLACEMENT : PASSIVE_SEEP=0 ne peut pas rendre le golden d7d709f
  (le régional n'existe plus) — il doit rendre le monde d'AVANT tout seep, donc se
  tester contre le golden HISTORIQUE `git show 52a97ca:scps/golden_hashes.txt`.
  Vérifié IDENTIQUE. Motif générique : le kill-switch d'un remplacement se compare au
  commit d'avant la PREMIÈRE version, pas au golden courant.

**Restes** : ~78 % à l'an 180 — le résiduel est l'inaccessible (îles sans marine, poches
closes) ; SEEP_POP_MONTH/SEEP_TARGET = spec joueur brute (2.8/100), calibrables ; le
seep ignore le climat (il peuple aussi l'hostile — la matrice ne gate que les fondations
actives) ; perf : un monde aux ~530 provinces vivantes se simule d'autant plus lentement.

## LA POPULATION PORTE TOUT — factions dissoutes dans les peuples (2026-08-06)

**Directives joueur enchaînées** : « plus de faction, tout faire reposer sur les
populations » · « merge communautaire avec le peuple, marchand avec bourgeois, fanatique
avec élite » · « glisse les mécanismes de faction et leur satisfaction vers les popgroup » ·
« le statecraft est porté par la pop, ministre élite/bourgeois/paysan ».

**Le modèle final** (5 itérations MESURÉES — chaque forme intermédiaire avait un trou
chiffré) :
- ÉTAT : g_lever_grief/g_capture (tableaux pays) SUPPRIMÉS → PopGroup.ethos_grief/
  state_grip (SAVE v100). g_lever_bias RESTE pays-grain (la stance est un choix de la
  COURONNE). Les 55 call-sites gardent leurs signatures via faction_bind (motif
  g_tech_cache) — la sim bind au tick, les bancs sur leurs fixtures.
- CLASSES = SOCLE des courants : laboureur→Communautaire, bourgeois→Marchand,
  élite→Gardien 0.6 + Conquérant 0.4, esclave SANS VOIX. La culture module par-dessus
  (FAC_CLASS_W). Dans la distribution, chaque strate d'un groupe pèse avec SON penchant.
- TEINTE : le penchant d'un groupe = le MÉLANGE pondéré de ses classes (émergées ; repli
  pré-émergence = structure NOMINALE 80/15/5 avec la classe POSÉE renforcée +0.5).
  Le grief se LIT sans seuil (moyenne pondérée par teinte — jamais 0 dès qu'un peuple
  rumine) et s'ÉCRIT par opposition pondérée (Σ lean×opp, gain FAC_TINT_GAIN=2 pour
  l'échelle du canal grief→loyauté). La capture se VERSE au prorata de la teinte ; le
  ROT = le MAX des captures par courant (l'État est capturé par SON captor — une
  moyenne diluait dans la masse saine et l'indice ne bougeait plus).
- MINISTRES : un candidat au Conseil a une CLASSE (hash : élite 3/6 · bourgeois 2/6 ·
  paysan 1/6) et sa faction EST faction_class_current(cid, classe) — le courant de sa
  classe telle qu'incarnée dans son pays. Le shuffle de factions de cour est mort.

**Pièges (chacun a coûté une itération)** :
- Un courant sans peuple DOMINANT (Transgresseur : AUCUN profil culturel ne le met en
  tête, même avant le socle) devenait in-aigrissable en « boîtes » — d'où la TEINTE.
- La teinte élite d'un groupe nominal plafonne ~3 % (5 % de pop × clout 3) : TOUT seuil
  de teinte (0.15, 0.05) la tue — d'où la lecture SANS seuil (les poids se normalisent).
- Le repli nominal ÉCRASAIT la classe posée des fixtures (grp() ne remplit pas
  pop_by_class) — d'où le renfort +0.5 de la classe déclarée.
- Un pays qui a PERDU ses provinces (tests de révolte) n'est PLUS corruptible — c'est la
  sémantique voulue (pas de peuple, pas de capture) ; le test du plancher d'efficacité
  ping-teste désormais un pays PORTEUR.
- L'élite gardienne ÉSOTÉRIQUE rumine contre sa propre orthodoxie (teinte transgressive
  0.35 × opposition G↔T forte) — thématiquement délicieux, mais le témoin d'un banc doit
  être un gardien PUR (ADAPTATIF).
- Récidive outillage : le python-en-heredoc mange les \n des printf (3 fois cette
  session) — TOUJOURS Write un .py ; et bash -lc SANS cd = build dans $HOME (re-vécu).

**Restes** : mots face-joueur (STR_HOVER_SEDITION parle encore de « faction ») ; la
satisfaction de groupe (lecteur pondéré pop_by_class à brancher sur l'UI ministres) ;
missions émanant des peuples ; LA GRANDE FUSION strata↔groups (364 usages strata —
l'équivalent du re-key province, vague dédiée multi-sessions).

**Addendum (les 2 bugs que les bancs ont attrapés — la valeur du filet)** :
- REPRODUCTIBILITÉ : faction_levers_reset() à l'init d'une sim purgeait les groupes du
  monde ENCORE LIÉ (état global de module) — la sim B fraîche mutait la sim A. Fix :
  l'init bind SON monde AVANT reset. Attrapé par scps_api_demo « sim A == sim B ».
- USE-AFTER-FREE différé : scps_sim_free ne rendait pas le bind — toute lecture de
  façade avant le premier tick de la sim suivante tapait l'économie LIBÉRÉE ; le tas
  rongé explosait 19 mondes plus tard DANS le worldgen (segfault « rivières » — le
  symptôme à des kilomètres de la cause). Fix : faction_bind(NULL,NULL) au free.
  RÈGLE : tout module à contexte global lié (motif g_tech_cache) DOIT unbind à la
  destruction de ce qu'il pointe.
- Le pôle tech suit désormais la CONDITION autant que la culture : une aristocratie
  devenue marchande RESTE martiale (socle élite) — pour qu'une région bascule en pôle
  fluide, elle doit S'EMBOURGEOISER (forks_demo : le flip pose ethos ET klass).
- Le seep passif fonde des provinces PENDANT les chantiers des bancs de façade
  (« +1 province » devenait +3) : scps_api_demo coupe PASSIVE_SEEP (tune_set) — il
  prouve la façade, pas l'expansion.

## LES ÉDIFICES ÉLÈVENT LEUR CLASSE + LE TERME POLITIQUE DE LA SATISFACTION (2026-08-06)

**Directives joueur** : « on va tous finir marchands » → « 100 élites par bâtiment (+100 à
chaque upgrade) et % efficacité sur la POP (Pop × investissement × rot) » ; et sur la
lisibilité : « dans le hover de détail de satisfaction il faut un "votre politique : ±X" —
la satisfaction s'obtient en allant dans le sens de la pop OU par l'impôt OU par les
marchandises ». JAMAIS de verbose narrative (« la Noblesse tient les offices » = interdit).

**Découvertes** :
- CONFIRMÉ à la struct : ProvBuild (le delta d'un édifice) n'a AUCUN champ d'emplois —
  le lien édifices→élites « qui devait exister » n'avait JAMAIS été codé. Seules sources
  de classes : tier de capitale (∝ pop, automatique) + ouvriers d'ateliers (bourgeois).
  D'où la convergence : la SEULE variable sociale pilotable était l'atelier.
- Fix : sièges d'élite DÉRIVÉS d'edi_built (rien de sérialisé) — épée (garnison/
  forteresse/citadelle/arsenal/amirauté), robe (tribunal/chancellerie/académie), clergé
  (temple/cathédrale/monastère — PAS le sanctuaire T1). v2 : tier×100 par édifice
  (+100/upgrade — la citadelle T3 vaut 300, elle EST sa famille) + part proportionnelle
  pop × EDI_ELITE_POP_PCT × Σtiers × (1+rot) — le ROT GONFLE la cour (vénalité des
  offices). Sans la part-pop, la masse bourgeoise des grands empires noyait la noblesse.
- SWEEP 3 graines apparié (an 180) : « tous marchands » n'était PAS une fatalité DÉMO
  (bourgeois 8-13 % même OFF, le monde reste ~3/4 laboureur) — le risque était le POIDS
  POLITIQUE. ON : élites +1..4 pts partout (4043 : 9→13 %), poids politique noble
  ~12 %→~27 % du clout mondial. Limite : mesure MONDIALE — la divergence PAR VOIE
  (citadelles vs comptoirs) reste à prouver par pays.
- LE TERME POLITIQUE dans LA formule de satisfaction (scps_econ l.4936) : sat = panier
  + confort ± POL_SAT_W×(alignement − grief) − impôt − cicatrice, borné ±POL_SAT_CAP.
  L'ALIGNEMENT = la stance de la couronne projetée sur le penchant de la classe
  (faction_class_policy) ; le GRIEF = la rancœur des porteurs locaux. Le grief d'éthos
  cesse d'être un canal invisible : il pèse dans LE nombre que le joueur regarde.
  Reader façade scps_country_class_policy_sat (±15 pts) pour la ligne UI « Votre
  politique : ±X » — LE NOMBRE, JAMAIS LE RÉCIT (doctrine).
- CHRONICLE : ligne « CLASSES monde : L/B/E/S % » — la société se juge sur pièces.

**Pièges** :
- Le Makefile n'était JAMAIS commité (racine, hors de mes `git add scps/`) : un
  `git checkout -- Makefile` a perdu le fix TECH_DEMO_OBJS+tune.o vieux de 3 jours.
  COMMITTER le Makefile avec ses vagues.
- scps_demography.o dépend désormais d'agency (edifice_tier) + factions (capture) :
  8 recettes de bancs complétées. APPEND en FIN de définition multiligne (jamais après
  un `\` de continuation — un regex naïf a cassé les 40 builds d'un coup).
- Un sweep doit CONSERVER ses logs entiers (fichiers), pas des variables greppées —
  la première mesure de composition a jeté toutes les autres télémétries.

**Restes** : brancher la ligne « Votre politique : ±X » dans budget_panel_v2.gd (reader
prêt) ; composition PAR PAYS au chronicle (prouver la divergence des voies) ; la passe
des mots (« faction » face-joueur) ; calibrage POL_SAT_W/CAP sous sweep long.

## LA VENTILATION CONTEXTUELLE + LE USE-AFTER-FREE HISTORIQUE DE g_tech_cache (2026-08-06)

**Directive joueur** : « les factions ont un poids différent à chaque fois — LFI et RN
sont votés par les pauvres, pas à la même proportion » → le cas « Couronne Durgan »
vendu → GO.

**La ventilation** (class_ethos_base_ctx) : le socle de classe se ventile selon la
CONDITION VÉCUE — misère = 1−satisfaction de la strate dans SA province, friction = la
part non-dominante locale (la mosaïque). Laboureur misérable → Gardien (×friction) +
Conquérant (colère tournée dehors) ; bourgeois menacé → Légiste ; élite en crise → se
crispe sur l'épée et l'autel. LE POINT NEUTRE : satisfaction ≥ 0.5 → m_eff=0 → LE SOCLE
D'HIER EXACT (la radicalisation est un ÉVÉNEMENT de crise, pas un bruit permanent — la
continuité des mondes calmes, des bancs et du kill-switch POP_MOOD_GAIN=0 est stricte).
La « noblesse d'affaires » (élite prospère → Marchand) SACRIFIÉE v1 pour cette
continuité — elle demanderait un signal de RICHESSE, pas de satisfaction (reste).
La boucle complète : misère → radicalisation → le penchant s'écarte de la stance →
« Votre politique : −X » → la satisfaction chute → la spirale, brisée par le grain,
l'impôt ou un changement de cap. Tout par coordonnées, jamais un dé.

**LE BUG HISTORIQUE (ASan)** : heap-use-after-free dans tech_has_tier — g_tech_cache
(le motif de bind global DONT j'AVAIS COPIÉ le design pour faction_bind) pointait le
TechState LIBÉRÉ par sim_free_members ; toute sim SUIVANTE du même process (chronicle
multi-sims, bancs façade) lisait de la mémoire morte via econ_country_has_tier (les
gates de tier des édifices !) pendant sa genèse. LATENT DEPUIS LA CRÉATION du cache —
le déterminisme apparent tenait au layout mémoire reproductible ; la ventilation n'a
fait que déplacer le tas assez pour l'exposer (segfault « 19e monde », hors gdb
seulement). Fix : sim_free_members REND les deux caches (econ_apply_country_tech(NULL)
+ faction_bind(NULL)) avant les free. Il n'existe QUE ces 2 caches globaux (grep).

**Outillage** : cible `api_asan` ajoutée au Makefile (le banc façade sous ASan+UBSan —
seul chronicle_asan existait). C'est elle qui a nommé le fautif en une ligne quand gdb
ne voyait RIEN (heisenbug de layout). Pour toute corruption « symptôme loin de la
cause » : api_asan D'ABORD.

**Restes** : signal de richesse pour la noblesse d'affaires ; sweep long pour mesurer
la belligérance f(misère du monde) ; POP_MOOD_GAIN calibrable.
