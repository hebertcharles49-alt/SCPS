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
