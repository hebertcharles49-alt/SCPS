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
