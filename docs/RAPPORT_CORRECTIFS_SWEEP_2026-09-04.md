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

_(à compléter : W2-7)_

## 2. Ce qui reste ouvert (décisions joueur)

- **Marbrive structurellement mort** : ses trois conditions (agitation ≥ 55, instabilité bâtie ≥ 1, coercition ≥ 0,25) sont anti-corrélées, 0 occurrence sur 3,3 M région-jours ; le banc force la fixture. Piste : OU sur l'instabilité bâtie, ou seuil 0,3, seuils au registre.
- **Abandon de doctrine par l'IA** : Divin reste à 0 adoption (slots pris avant l'an 40) ; seul levier restant.
- **Exposant tech** : 0,65 laisse le leader vers 78-80 % de l'arbre à l'an 200 (cible 40-60 %) ; 0,78 casserait « wide récompensé » (×2 sur l'empire moyen). Recommandation : renchérir les tiers 4-5 en vague séparée.
- **« Hors registre » ~1 700 or/mois/empire** dans le recoupement I0 : des dépenses que le registre de flux ne voit pas (chantiers matière maison, semis IA, colonisation ?).
- Population P7 (esclavage régional), P10 (foi sommée), P8 (nommer les deux réalités de classe).
- Décrochage : « le décroché se replie comme le déroute » mesuré sans gain, retiré ; P4 milice plancher exige un apparié.
- Friche pid-ordonnée ; `SINK_MONTHS` ; colonisation et marine toujours gratuites.
- Desseins : aucun panneau Godot ne consomme `dessein_info` (pivot bindé, nulle part affiché).

## 3. Sweep de validation 50 graines × 250 ans

Protocole : `tools/sweep_doct_ai.sh` v2, apparié témoin `AI_DOCT=0` vs essai, 100 sims, 8 jobs, dossier `sweep_valid_W1W2_50x250/`. Lecture intégrale par un Opus data analyst.

_(section remplie à la sortie du sweep : résumé de l'analyste, tables, anomalies, verdict)_
