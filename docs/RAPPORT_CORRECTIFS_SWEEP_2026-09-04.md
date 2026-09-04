# RAPPORT — CORRECTIFS DES VAGUES W1/W2 ET SWEEP DE VALIDATION 50 × 250

Branche `claude/vibrant-euler-1tgfp3`, commits 37680a3 → 64146e4 (2026-09-03/04). Save v108. Golden et golden-deep re-baselinés deux fois (fin W1 : 908a7cd ; fin W2 : 64146e4). Toutes les décisions joueur citées sont actées dans TROUVAILLES.md et la mémoire de session.

## 0. Décisions joueur qui ont cadré la vague

- « Corrige tout » sur les cinq rapports de calibrage (docs/CALIB_{INFLUENCE,ECONOMIE,TECH,ARMEE,POPULATION}_2026-09-03.md) ; « démêle le plat de spaghettis ».
- **Trésor et stock NATIONAUX** : la banque et le stock par province étaient un choix d'implémentation, pas une décision joueur (« C'est voulu par toi, pas par moi »).
- **Pas de cap** : jamais un plafond dur ; le frein est économique (coût, solde, revenu) ou une règle de conservation.
- Divin gaté sur une religion FONDÉE par le pays (fondation, schisme, hérésie).
- Sweep de validation 50 graines × 250 ans en fin de travail, lu par un Opus data analyst (lecture intégrale, scripts filtrants interdits) ; un Opus joueur pour les impressions par captures.

## 1. Les correctifs, commit par commit

### 1.1 Préparation (37680a3 → b9b026c)

| Commit | Domaine | Ce qui a changé | Preuve |
|---|---|---|---|
| 37680a3 | Chronicle | Télémétrie honnête : provinces vierges ≠ colonisées sans propriétaire ; doctrines actives = instantané + cumul ; plancher de volume sur les hubs ; « X libre » = sécession jouable | golden identique |
| ddd1b1c | Influence | Assiette sur les SIÈGES (`pop_by_class`) au lieu des strates par richesse (~1-3 % → ~13 % d'élites) ; 3 classes 0,002/0,0011/0,00011 (≈60/20/20) ; courants = boost de leur classe ; Divin = fidèles/6000 | kill-switch byte-identique, golden re-baseliné |
| 38523b6 | Guerre | Capitale orpheline recalée ; garde de budget valable sous le feu ; affectation `pop_by_class_in_army` rendue au bon registre | s512 : 17/23 empires à 0 rgt → 1/7 ; s777 : 342 → 43 rgt |
| b9b026c | Doctrines | Divin refusé tant qu'aucune religion n'a été fondée par le pays (DOCT_NO_FAITH, mot « Aucune religion fondée ») | golden identique (l'IA n'adoptait jamais Divin) |

### 1.2 Vague W1 (aa2e82e → 908a7cd) — cinq agents opus en worktrees

| Commit | Domaine | Ce qui a changé | Mesure avant → après |
|---|---|---|---|
| aa2e82e | Influence/foi (W1-E) | Foi d'État écrite sur TOUTES les provinces de la région (unique écrivain de `PopGroup.faith`) ; Conseil : siège vide = rang I (monotone) ; émissaire 6×é, fabrication 12×é, pivot 10×é ; courant IA après l'an 40 | plafond Divin 0,617e-4 → 1,667e-4 ; influence médiane 3134 → 390, saturation an 96 disparue |
| 95fcd3e | Population (W1-B) | Invariant Σ sièges = âmes rétabli (9 sites) ; émergence des classes sur toutes les provinces ; tier unifié ; essaimage au prorata ; plancher provincial ; ledger P11 | provinces figées 75 % → 45 % ; strates 89/8/2 → 85/11/3 |
| 9d0d2a4 | Tech (W1-C) | Ruines rebranchées sur la matière arcane (3 nœuds morts à vie) ; `TECH_COST_N_EXP` 0,50 → 0,65 ; barre d'héritage tier 3 en plafond de profondeur ; scores IA tech ×400 ; clé METAB_TIER12 réelle | leader 49 → 40 techs à 120 ans, max d'arbre 74 % → 62 %, combos 12 → 3 |
| a2d234d | Armée (W1-F) | 9 tunables au registre ; corps au front compté dans le pool ; Milice soldée ; Hallebardier → CONSCRIPTION, cav. lourde → ORGANISATION ; levée au grain province × `ARMY_POOL_FRAC` 0,20 ; décrochage 0,35 | rgt/limite max 2,02× → 1,03× ; efficacité milice 112 → 7,2 |
| 908a7cd | Trésor/stock national (W1-A) + frein | `nat_treasury[cid]`, `nat_stock[cid][res]` ; provinces/régions sans trésor ni stock ; 3 miroirs supprimés ; pool « rassemblé puis redistribué » supprimé (−1516/+1662, 49 fichiers) ; fuite −2596 or/an attrapée par l'invariant M3c ; 21 bancs re-keyés ; CLAUDE.md corrigé. **Interaction A×F** isolée par 3 arbres intermédiaires → frein économique `WH_DESERT_RATE` 0,5 + `WH_PAY_REVENUE_FRAC` 0,35 | fusion brute : 62 rgt / limite 25, trésor 0, taxes 1,7 → avec frein : 20/24 rgt, trésor 17 k, taxes 149/90 |

Rejetés au nom de « pas de cap » : plafond `TECH_PROD_CAP` (rapport tech P1), levée plafonnée à 2× la limite de force (rapport armée P1).

### 1.3 Vague W2 (db71f24 → 64146e4)

| Commit | Domaine | Ce qui a changé | Mesure avant → après |
|---|---|---|---|
| db71f24 | Front Godot (W2-5) | Champ stock provincial retiré ; prix diplo réels × é dans le binding et les hovers ; audit UI qui refuse un stock provincial | probes 04b/06/09/03 regardées |
| f5dc044 | Chronicle/sweep (W2-4) | Flux décomposé recoupé au trésor national (ligne « recoupement I0 ») ; lignes rgt/limite, solde/revenu, frein de levée, prix du grain, foi d'État, porte Divin ; script de sweep v2 (« prix » → « indice », vraie colonne grain, nouvelles colonnes) | golden identique ; test 2×2×60 recalculé colonne par colonne |
| 8cfe747 | Population F2 (W2-2) | Les âmes des groupes suivent enfin les strates (`demography_group_growth_sync`) ; 4 sites d'invariant réveillés et fermés | âmes/strates 23,6 % → 99,8 % ; sièges d'élite 30 % → 11 % ; assiette 79/12/9 → 56/22/22 ; provinces figées 44 % → 11 % |
| dd7f64d | Économie (W2-1) | Prix effondrés (plancher par province sommé > trésor national) → `PL_SINK_MONTHS` 3 ; `COURT_MONTHS` 60 (cour/admin/encadrement revivent ; P1 4000→1200 réfuté : clé partagée avec le crédit) ; matière maison facturée (`BUILD_OWN_MATERIAL_PRICE` 1) ; `labor` 6,0 sur 7 luxes ; l'IA paie ses manufactures | grain 0,00 → 1,06 ; taxes 13 → 546 or/mois ; manufactures 403 → 3306 ; friche 50 → 27 ; satisfaction 37/52/35 → 50/84/64 |
| 1ba07b9 | Armée (W2-3) | `BT_DECROCHE` 0,35 → 0,26 (sonde 10 runs) | décrochages 40 % → 22/19 %, prises 27 → 41/37 |
| cf477ec | Opus joueur | Partie graine 7 par probe, 24 captures, 22 frictions, 18 bugs (docs/RAPPORT_JOUEUR_2026-09-04.md) | — |
| 74acdea | Banc API (W2-6) | Pas de régression de la file de décisions : `tune_set(PASSIVE_SEEP=0)` global gelait le monde du banc ; horizon 200 → 60 ans, assertion franche | banc 47 min → 5 min, 250/250 |
| 64146e4 | Golden | Re-baseline de fin de vague W2 | 42 bancs, membrane, determinism, ASan, savetest, fuzztest, lang-check 127 |

W2-7 (façade/UI des bugs vus par le joueur : prix des stocks nationaux à 0, grand livre vide, allocation affichée, ids moteur, rail illisible) : en cours au moment de la rédaction ; sa fusion s'ajoute en §1.4.

### 1.4 Fusions postérieures au lancement du sweep

| Domaine | Ce qui a changé | Preuve |
|---|---|---|
| Façade/UI (W2-7, bugs du rapport joueur) | Prix des stocks nationaux = prix FACTURÉ (le lecteur rendait le prix nu 0,004-0,10 : « 0,00 » à l'écran) ; grand livre : la façade mémorise l'or au RAZ et rend « Autres mouvements » ⇒ Σ postes = delta de trésor, champ `month` unique ; `scps_region_label` (toponyme) et `region_name` dans chaque événement (fin des « région 21 / Prov.6 ») ; `scps_manuf_name` (fin des manufactures « ? », la copie du binding avait 24 entrées pour 30) ; rail droit : bandeau clair sur les deux parités (JOURNAL, VILLES, GUERRES lisibles) ; libellés coupés sur espace ; panneau Doctrines : « Influence N · +N/mois » + « Prochaine idée : X — N d'influence » ; boutons « Édifices… / Manufactures… » ; « tier » → « palier ». L'« allocation 90 % → 100 % » n'était pas un bug (la probe poussait un seul puits) | 5 assertions ajoutées au banc API (Σ postes = solde ±1 sur tous les pays, prix ≥ plancher = devis×marge, aucune manufacture « ? », chaque région nommée) ; 24 captures rejouées (godot/project/shots_player_w27/) |

Restes W2-7 : pacte accepté sans trace (`diplo_context` ne rapporte aucun engagement) ; panneau Armée hors pile Échap ; Annales qui ne retiennent que « un âge a commencé » ; **`econ_country_tax_class_month` rend 0** alors que FX_TAX vaut ~14 or/mois (tout le fiscal tombe dans « Autres mouvements » — probablement resté région-grain après le re-key) ; registre FX_* sans bucket pour l'achat d'État.

## 2. Ce qui reste ouvert (décisions joueur)

- ~~Marbrive structurellement mort~~ : **réfuté par le sweep** (63 déclenchements sur 27 sims × 250 ans) — le constat de W2-6 valait une graine sur 60 ans ; il reste rare avant l'an 60, pas mort. Rien à corriger ; le banc events_demo pourrait tester l'atteinte par le monde plutôt que forcer la fixture.
- **Abandon de doctrine par l'IA** : Divin reste à 0 adoption (slots pris avant l'an 40) ; seul levier restant.
- **Exposant tech** : 0,65 laisse le leader vers 78-80 % de l'arbre à l'an 200 (cible 40-60 %) ; 0,78 casserait « wide récompensé » (×2 sur l'empire moyen). Recommandation : renchérir les tiers 4-5 en vague séparée.
- **« Hors registre » ~1 700 or/mois/empire** dans le recoupement I0 : des dépenses que le registre de flux ne voit pas (chantiers matière maison, semis IA, colonisation ?).
- Population P7 (esclavage régional), P10 (foi sommée), P8 (nommer les deux réalités de classe).
- Décrochage : « le décroché se replie comme le déroute » mesuré sans gain, retiré ; P4 milice plancher exige un apparié.
- Friche pid-ordonnée ; `SINK_MONTHS` ; colonisation et marine toujours gratuites.
- Desseins : aucun panneau Godot ne consomme `dessein_info` (pivot bindé, nulle part affiché).

## 3. Sweep de validation 50 graines × 250 ans

Protocole : `tools/sweep_doct_ai.sh` v2, apparié témoin `AI_DOCT=0` vs essai, 100 sims, 8 jobs, dossier `sweep_valid_W1W2_50x250/`. Lecture intégrale par un Opus data analyst.

Coupé par le joueur après 5 h de simulation (« 5 h de sim continue suffisent ») : **13 paires complètes** (graines 1 2 3 7 11 60 90 512 777 1009 2026 3333 4243) + un témoin orphelin (graine 5) ; 27 journaux lus intégralement par l'analyste (docs/SWEEP_VALID_W1W2_2026-09-04.md, 7 sections, 16 anomalies citées fichier:ligne). Aucun ASSERT, invariant monnaie max 201 % pour un seuil de 370 %, 6 âges dans 27/27.

### 3.1 Verdict global (témoin AI_DOCT=0 vs essai, et vs le sweep 10×200 d'avant les vagues)
- L'arbre de doctrines est joué jusqu'à l'an 250 (96 adoptions cumulées médianes, influence médiane 250) SANS casser les agrégats : pays 38 → 33, guerres 245 → 219, masse monétaire +2,5 %, satisfaction des journaliers +3 points.
- La monnaie et le grain sont plus bas qu'avant les vagues : indice ÷6, trésor ÷2,7, grain médian 0,23 pour une base 1,00.

### 3.2 Correctifs TENUS (les plus nets)
1. Population (W1-B + W2-2) : « écart +0 = 0,0 % » et « 0 groupe hors invariant » dans 27/27 ; âmes/strates 97,8-100,4 %.
2. Décrochage (W2-3) 0,26 : 18,5-19,7 % médians, 0 nul, 27/27.
3. Juge martial : 62 % → 86 % pondéré.
4. Courant IA différé (W1-E) : Aristocratie 13,0 % → 1,6 % des adoptions, Populaire 55 % des courants ; Technologie ×16, Connaissances ×11,5 ; Faustien sort du code mort.
5. Arbre HÉRITÉ (§27, W1-C) enfin visible (jusqu'à 11 empires à 92 %).

### 3.3 Anomalies graves
1. **Le recoupement I0 ne se recoupe jamais** : toujours négatif, médiane −1 519 / −1 891 or/mois/empire, pire −4 078 — signe constant, donc une dépense structurelle hors registre, pas des buckets épars.
2. **Queue de la levée hors frein** : armée/limite jusqu'à 432 %, solde/revenu jusqu'à 19 066 % — le frein W1 n'a pas de prise sur la queue de distribution.
3. **Armée fantôme réfutée à l'envers** : le premier empire du monde à 0 régiment avec 184 577 or et 127 329 armes lourdes en stock (essai_s11:698).
4. **Prix du grain 0,000 exact dans 5 sims sur 27** (le plancher indexé sur le trésor retombe à 0 sur certains mondes).
5. **Marbrive n'est pas mort** : 63 déclenchements sur 27 sims (le constat de W2-6 valait un an-60 sur une graine).

### 3.4 Propositions de l'analyste (par impact)
- P1 : bucket `FX_AUTRES` par différence puis ventilation (print-only) — sans quoi aucune mesure monétaire ne peut servir de gate.
- P3 : imprimer la RAISON du refus de levée (armes, or, pool, budget) avant de toucher `ARMY_POOL_FRAC` — l'empire riche à 0 régiment doit s'expliquer.
- P2 : plancher du prix du grain découplé du trésor — décision joueur (limite de « pas de cap » : un plancher n'est pas un plafond, mais c'est un nombre neuf).

### 3.5 Limites
13 paires sur 50, une sim par cellule (aucun bruit intra-graine), horizon unique de 250 ans face à des cibles calibrées à 120 : la moitié des écarts peut être un effet d'horizon. Les 13 graines arrivées sont les plus rapides, donc les mondes les plus petits. La commande de reprise (mêmes graines, script v2) permet de compléter à 50.
