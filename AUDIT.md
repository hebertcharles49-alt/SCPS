# AUDIT — SCPS (moteur de grande stratégie C99)

> État vérifiable. Source de vérité = les bancs (`make test`), jamais ce fichier.
> Daté : 2026-06-18 (entête rafraîchi ; les entrées datées plus bas gardent leur
> compte d'époque). Build : gcc 13 · -O2 -Wall -Wextra -std=c99 + durcissement
> (`-fstack-protector-strong -D_FORTIFY_SOURCE=2`). SAVE_VERSION = **31** (le
> format a fusionné les deux lignées : endgame §27 + assets/armée du collègue ;
> puis v30 ROSTER militaire 12→22, v31 empreinte tunables — cf. correctifs d'audit).

---

## (a) Résultats MESURÉS (`make test`, K3 appliqué)

**37 bancs VERTS / 37** — `make test` les bâtit, les lance, compte les BILAN
(snapshot 2026-06-18 ; le compte AUTORITAIRE est la sortie de `make test`, pas
cette liste — gardée à jour pour le coup d'œil) :

core 35/35 · monde_reel 10/10 · readout 27/27 · species 9/9 · tech 22/22 ·
faith 14/14 · intertrade 25/25 · routes 4/4 · save_io 14/14 · statecraft 27/27 ·
pop 14/14 · army 48/48 · demography 19/19 · demography_integ 6/6 · revolt 23/23 ·
social 10/10 · agency 16/16 · campaign 19/19 · factions 35/35 · econ_tax 8/8 ·
econ_culture 6/6 · econ_arcane 6/6 · econ_production 4/4 · labor 37/37 ·
missions 8/8 · diplo 49/49 · warhost 4/4 · events 41/41 · structural 16/16 ·
ai 23/23 · forks 35/35 · prosperity OK (sans format BILAN) · credit 16/16 ·
cap 5/5 · endgame 76/76 · **audit_eco 4/4** · **lang 26/26**.

**0 banc rouge.** `audit_eco` et `lang_demo` sont désormais DANS le harnais
(`tools/run_tests.sh`) — le rouge de `make audit` (longtemps invisible) serait
capté. `make smoke` = sous-ensemble rapide ; `make full-test` = bancs +
déterminisme + ASan. Statut mesuré sur ce build, pas supposé.

## (a-ter) Refonte ÉCO + pipelines IA + hameaux libres + diplo subjectif (2026-06-25)

Quatre arcs livrés et MESURÉS (40/40 bancs · determinism STABLE & golden re-baseliné
vert · 0 warning) :
- **Refonte ÉCO (A0-A5)** : extraction LABOR-BOUND `out = ouvriers × YIELD × geo_eff ×
  prix` (table ancrée grain 800/poisson 400/bois 50…), bouche ANNUELLE calibrée
  (`FOOD_NEED`, sinon needs_met s'effondre), nourriture du SPAWN (capitale = socle de
  grain, SEULE règle vivrière de worldgen). Calibré seed 9 : pop 38.4k (= baseline 38.8k),
  needs_met 0.52, remplissage 63 %. `EXTRACT_GEO_REF` 4.5 · `EXTRACT_LABOR_SHARE` 0.65.
- **Pipeline IA éco** : `econ_country_forecast` (runway/shortfall/déficit structurel +
  food_runway, l'offre suit la pop jusqu'au potentiel → seuls les déficits STRUCTURELS
  arment) → priorités émergentes (prix×stress×manque) → colonisation needs-aware (capacité
  + steer urgence) + anti-spirale food + relocate projeté + grenier stock-safe. SAVE non
  bumpé (AiView = cache de tick).
- **Hameaux libres (POLITY_WILD)** : N hameaux épars/jouable (BFS), pop ≈750±, culture +
  éthos DISTINCTS du voisin, AUCUNE religion, raw food forcée, PASSIFS (ai_on=false).
  Ralliement CULTUREL au voisin (contact pacifique soutenu) ; absorption militaire par
  conquête. seed 9 : 4 semés, 4 ralliés/100 ans.
- **Pipeline diplo (étages 1-2)** : `ai_province_value` SUBJECTIVE (prix objectif + BESOIN
  Σ raw_cap×stress(runway)×prix) → `ai_pick_rival` convoite (AI_COVET_W), `diplo_settle`
  butin needs-driven (l'affamé exige le grenier). Banc INVARIANT anti-modificateur
  (diplo_demo 51/51 : affamé 123 vs repu 30). **À VENIR** : étage 3 (vassalité sur la
  durée — acquérir/tenir/intégrer/annexer avec soft-scar) NON implémenté.

Tous SAVE non bumpés (aucune struct sérialisée n'a grandi). RE-BASELINE du hash 12 ans
(golden mis à jour, diff revu).

---

## (a-bis) Passe de STABILISATION (2026-06-18)

Suite à l'audit technique du 18 juin (point bloquant : `make audit` ROUGE 2/4).

- **`make audit` au VERT (4/4)** — deux bornes étaient rouges depuis longtemps,
  jamais captées (audit_eco n'était pas dans le harnais). Les DEUX échecs sont
  des artefacts du BANC, pas des bugs moteur :
  - **POP (E0.1)** : le banc colonisait une vierge de FRONTIÈRE comme hameau
    témoin. Sur un monde N1 (carte nue) développé, la frontière est pauvre
    (cap_pop ≤ 200) : une colonie de ≈250 âmes y naît DÉJÀ au-dessus du plafond
    (cap_factor=0, pop gelée) → la borne mesurait le mauvais régime. Fix : le
    témoin est la région du joueur (HORS capitale) avec le PLUS de marge sous
    son eff_cap. → rég 55, ×1.19 ∈ [1.1 .. 2.5].
  - **ACCESSION (E1 §9)** : le banc ne lance ni l'intertrade ni le marché de
    départ (`agency_seed_capital_markets`) — l'import de matériaux depuis les
    cités-états (qui bootstrappe un empire NU dans le vrai jeu) n'opère pas ici,
    donc la capitale ne voyait jamais de PIERRE. **Diagnostic RES_STONE** : ce
    n'est PAS un bug de marché — l'extraction est demand-driven et sans Centre/
    cache mondial dans le banc, il n'y a pas de consommateur ni de source ; le
    vrai jeu importe via `CS_TRADE_POOL` (N1, accession 3e empire an 43, prouvée
    en chronique). Fix de banc : amorcer la capitale d'un socle bois/pierre/
    argile (+120) ; le bootstrap import reste couvert par `intertrade_demo`.
- **Harnais durci** : `audit_eco` + `lang_demo` rejoignent `run_tests.sh`
  (35 → 37 bancs) ; `timeout` par banc ; split `make smoke` / `make full-test`.
- **Aucune entrée moteur touchée** : `make test` 37/37, déterminisme STABLE,
  aucun hash bougé. Re-baseline NULLE (purs fixes de banc + doc + harnais).

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo « Bâtisseur +K » : RÉSOLU (L6/K5.b)** : la racine n'était pas la profondeur de
  chaîne mais le crédit de largeur NON pondéré — la même cascade foi→savoir→K payait tout
  le monde d'aller voir ailleurs. Fix : la largeur suit l'éthos (Dominateur/Honneur → H
  tant que la poigne est basse · Mercantile → le réseau, toujours · Bureaucrate → K pur,
  jamais le détour savoir ; la foi de crise reste universelle). → 23/23 sur 4 graines.

---

## (c 5) Arc P-bis — la paix DÉCLARÉE & le prix juste (2026-06-14)

- **P2 — l'occupation devient territoire** : la cession de l'occupé existait (diplo_settle)
  mais un PLAFOND DE BUDGET la bloquait (4 occupations, 0 transférée). Itérations :
  (a) retrait du plafond → 46 transferts (EXCESSIF) ; (b) **prix de province log-compressé**
  (cumul bâti+pop, hard-cap 80, un hameau de 100 âmes ≈ 5) → le budget §5 borne de nouveau,
  mais sur une échelle saine ; (c) **PAIX DÉCLARÉE** (règle lisible) : score ≥ 50 ET ≥ 1
  région tenue → décisive (on encaisse l'occupé) ; OU **10 ans** → paix blanche. Le « ET
  occupé » laisse les SIÈGES aboutir (le score-batailles touche 50 avant qu'une place tombe).
  Tunables `AI_WAR_DECISIVE` (50) / `AI_WAR_EXHAUST` (10 ans).
- **Calibrage du timeout** (chronique 1×60 seed 7) : 5 ans → 0 occ ; 8 → 4 occ/0 transf ;
  12 → 9/11 ; **10 ans retenu → 10 occ · 4 transferts · 2 sécessions** (MODÉRÉ : la carte
  change sans tout emporter). UI : le popup diplo affiche « Paix : score X/50 ou X/10 ans ».
- make test **32/32** · 0 warning · lang-check 64 · pas de bump SAVE (pure logique).
- **P3 — ratio poursuite/choc recalibré** (cible révisée **[0,8 ; 1,8]**, SANS cavalerie) :
  `BT_DMG_K` rendu tunable + grille (registre J). Le ratio est très SENSIBLE à `BT_DMG_K`
  (dégât de moral/jour → durée de bataille → morts de choc, quantifiés en paquets) :
  0.054→0.5× · **0.057→1.3×** · 0.060→1.6× · 0.063→0.5× (graine 7, 1×60). `CUREE_CAP`
  sans effet (la base P borne avant le cap). **Point figé : `SCPS_TUNE="BT_DMG_K=0.057"`**
  (devenu le défaut) → ratio **1.3×**, 0 décrochage, conquête P2-bis intacte (4 transferts,
  2 sécessions). NOTE : sans cavalerie la poursuite d'infanterie est bornée — À RECALIBRER
  vers [2,5] à l'arc UNITÉS MONTÉES (charge + poursuite ×2-3). make test 32/32.

## (c 6) Q6 — le DOUBLEMENT démographique 48k→96k (2026-06-14)

- **PIVOT (révision joueur) — la capacité VIENT DU DÉVELOPPEMENT.** Le 1er jet (apex
  statique par rôle, ci-dessous) atteignait 96k mais l'an-100 plafonnait à ~75 % d'un
  apex figé (logistique) → on a monté l'apex pour traverser 96k, au prix d'une variance
  guerrière. RÉVISÉ : `eff_cap = ½·cap_pop` (terre nue) **+ LOGEMENTS bâtis** (manufactures
  SEULES, `+HOUSE_MANUF`/niveau, plafond ½·cap_pop) **+ grenier** (nourriture). `cap_pop` =
  la taille PLEINE nourrie. **Bâtir double la région ½→plein** : la pop SUIT le bâti (seed 9
  monte 48→56→69→75→80→89k, monotone). Graine **UNIFORME** à l'an-0 (divergence ENSUITE).
  Readout viewer = miroir de l'eff_cap (les logements montent quand on bâtit). Tunables :
  `EMPIRE_CAP` 10300 / `CITY_CAP` 5150 (taille nourrie) · `HOUSE_MANUF` 100. make test
  **32/32** (les 2 contrôles `ai_demo` sensibles au monde — aggression/routes réalisées —
  rendus robustes : la conquête du Dominateur peut hériter des routes, hors appétit marchand).
  **À VENIR (#5)** : « le commerce nourrit le marché » (cités-états = hubs alimentaires)
  exige de DÉCOUPLER la nourriture de `cap_pop` (∝ fertilité) — increment distinct.
- _(1er jet, apex statique — conservé pour mémoire :)_
- **Cible (joueur)** : monde âgé (`world_age=0.7`, la Pangée FEND), **6 empires + 12
  cités-états**, an-0 = **48 000** hab, **doublement vers ~96k à l'an 100** — PUR
  (aucun taux de croissance touché ; la guerre REDISTRIBUE la capacité, ne la crée ni
  ne la détruit). Tolérance **±5 %**, l'an-100 est l'instantané du DOUBLEMENT (la pop est
  un jeune en pleine croissance, pas un monde saturé).
- **Capacité d'accueil par RÔLE** (`econ_init` Passe 2, tunables registre J) : la cible
  VIT dans les polités RÉELLES, pas la friche — `EMPIRE_CAP` 10800 / `CITY_CAP` 5400
  (apex) / friche 200. **Fuite COLMATÉE** : `cty_cap` ne somme que les régions VIVABLES
  (zone morte tranchée en Passe 1 — ≥35 % d'aire à habitabilité nulle OU moyenne < 12 % —
  et RÉUTILISÉE en Passe 3) ⇒ la cible se répartit en plein sur l'actif, `cap_pop_sum`
  = Σ cibles EXACT.
- **Graine DÉCOUPLÉE du cap** : `SEED_POP` (48000) réparti au prorata du cap_pop des
  régions de polité ⇒ an-0 = 48k PILE (fini le déficit « empire né sur terre morte ») et
  la pop AMORCÉE sous son plafond CROÎT vers l'apex. La capitale (région la plus riche,
  tuile nourricière) reçoit mécaniquement la plus grosse part.
- **Dispatch CONSERVATIF** (`colonize_from`) : les colons détachés ARRIVENT (graine =
  prélevé, plancher `COLONY_SEED_POP`) — plus de saignée du convoi terrestre (la mer garde
  son surcoût ×2 via `econ_colonize_overseas`). Le déclin de colonisation disparaît.
- **Mesuré** (an-100, graines 7/42/9/99, graine an-0 = 48k PILE) : **106/88/110/98k →
  moyenne ≈ 100k** à `EMPIRE_CAP=11000` ; recentré à **10800** (≈ 97k). `food_sat ≈ 0.95`
  (la NOURRITURE ne borne pas — le socle vivrier suit cap_pop). Variance assumée : mondes
  fendus (7/42) remplissent ~15 % de moins que cohésifs (9/99) — c'est la guerre/géo, pas
  un bug (« interaction is the point »).
- **Tension reconnue** : sous taux de croissance FIGÉ, un doublement logistique depuis la
  ½-capacité ne peut être À LA FOIS robuste (cap = 96k strict) ET pile 96k à l'an-100 — on
  a tranché POUR l'an-100 (apex > 96k, le monde traverse 96k en montant). ⚠ Pop & TAILLE
  des pays ≫ qu'avant → l'équilibre guerre/diplo (prix de province, budget §5 ∝ pop) se
  RELIT à cette échelle (bancs diplo/ai/campaign re-baselinés).
- Diag env : `SCPS_CAPDIAG` — an-0 (cap_pop_sum, graine, comptes de rôles) + an-100
  (remplissage colonisé/actif, food_sat). À RETIRER ou garder comme `SCPS_CSV` (preuve).

## (c 7) V3 — libérer le maritime régional, l'interaction VIRTUELLE (2026-06-14)

Racine MESURÉE (instrumentation `routes_order`, retirée depuis) : sur seed 7, **0 route
maritime** non par une gâche Centre/pacte (le code n'en a aucune sur le régional depuis M1)
mais parce que le monde re-baseliné (Q6/L4) a écarté les côtes — **145 paires de ports
étrangers testées, 0 sous le plafond de 60 j** (la plus proche à **72 j**), 91 cross-bassin,
54 entre 60-120 j. Le plafond de mer (rejet dur) était l'unique verrou.

Règle joueur (donnée en cours d'arc) : **deux ports + deux marchés + (pacte OU même empire)
= commerce, l'interaction est VIRTUELLE**. Donc :
- `routes_order` maritime ne **rejette plus sur la distance** : deux ports = lien. La distance
  ne MODULE que le rendement (`routes_advance` : `yield ∝ 1/(1+sea_days/40)`) — le « rendement
  dégressif sur les jours de mer » demandé ; hors-portée du calcul ⇒ distance virtuelle = borne.
- `navy_best_coast` (neuf) : la rade s'ouvre sur la meilleure côte (capitale côtière, sinon la
  + peuplée) ⇒ les empires à capitale enclavée participent ; l'IA navale (chronicle + viewer)
  bâtit le port là et trace depuis CE port (plus la seule capitale).
- `hub_map_build` : passerelle de mer ROBUSTE (toute côte branchée par terre fait porte — plus
  d'UN Centre côtier unique).

Preuve (1×100 ans) : routes maritimes **6 / 18 / 20** (seeds 7/11/19, ex-**0 / 3 / 0**) ;
commerce terrestre seed 7 **4493 → 4866** (ne régresse pas, monte). `make test` **32/32**,
ASan+UBSan muets, déterminisme `HASH 7 322534ab` reproductible, viewer 0-warning, lang-check
64, SAVE non bumpé. Sobriété **3 routes/pays** ⇒ seed 7 (genèse = **2 empires**) plafonne à 6 :
pour viser ~10-15 il faudrait enrôler les cités-états (marchés) OU relever la sobriété — laissé
au choix. Colonies outre-mer (cross-continent) 0/5/0 : f(côte vierge d'un AUTRE continent) +
paix (transports libres), émergent — non forcé.

## (c 8) S1 — le commerce ouvre l'archétype, mais le vrai verrou est la RECHERCHE (2026-06-14)

Le diagnostic donné (« la porte d'archétype n'est franchie que par la gouvernance ») est
INCOMPLET — l'instrumentation (retirée) le montre : post-GR4 les héritages sont ÉPARS, l'empire
en gouverne PLUSIEURS, donc l'accès est DÉJÀ large (`accès[444440]`, depth_max 4/SECRET). Le
vrai verrou est en AVAL : les signatures d'archétype sont toutes **tier-3 (~2300 pts)** et l'IA
gloutonne (`ai_pick_tech` saute l'inabordable, prend le moins cher) ne PAIE JAMAIS le tier-3 —
seule la foreuse passait, par sa LOGIQUE D'ÉPARGNE dédiée. D'où « 0/6 archétypes » éternel
(même pour SON propre héritage).

Deux ajouts (`scps_ai.c`) : (1) **le chemin commercial** — `ai_archetype_depth(+rn)` : une route
OUVERTE où l'empire est partie et dont l'autre bout PORTE un archétype creuse la profondeur (la
MER pèse fort, somme sur entités distinctes ; registre J). C'est la PORTE d'un archétype qu'on NE
gouverne PAS (Venise ← Grèce). (2) **la greffe culturelle** — `ai_research_step` fait ÉPARGNER un
empire investisseur (mercantile/bâtisseur) pour la signature accessible la moins chère (même
ressort que la foreuse), bornée à ≤ 2 greffes, NON-faustienne (le faustien = S3). C'est CE
ressort, pas la porte, qui décolle le compteur.

Preuve (1×100) : nœuds profonds seed 7/9/11/19 = **2/2/6/2** (ex-0). `make test` **32/32**, ASan
muet, viewer 0-warning, lang-check 64, **déterminisme `HASH 7 322534ab` (IDENTIQUE à V3 à 8 ans
— S1 ne perturbe pas le début ; la greffe mûrit au siècle)**. Profondeur d'arbre (41 %),
spécialisation (déjà 6 Société en V3, pas une régression S1) et commerce INCHANGÉS — la greffe
REMPLACE du tout-venant. SAVE non bumpé. À VENIR : S2 (cristallisation culturelle par contact),
S3 (frein faustien réconcilié → la COMBINAISON forge runique × arcane, encore 0).

## (c 9) S2 — la cristallisation culturelle suit le contact (réveil de culture_syncretize) (2026-06-14)

Le diagnostic donné (« culture_can_syncretize exige des cultures CO-GOUVERNÉES ») est INCOMPLET :
`culture_syncretize` (le mutant hybride) n'était appelé NULLE PART dans la sim — seul `culture_demo`
le touchait (DORMANT). La fusion VIVE du moteur, `assimilation_tick`, ne tire que vers la dominante
LOCALE (intra-province = co-gouvernées) ; il n'y avait donc AUCUN chemin par le contact inter-pays.

`demography_contact_tick` (neuf, annuel — chronicle + viewer) le réveille : une région en contact
commercial soutenu (route OUVERTE, à la paix) avec un AUTRE pays dérive sa culture dominante vers la
sienne, via la PILE DE DÉRIVE (durable, comme l'assimilation — pas une écriture du cache, qui était
écrasée chaque mois), la MER portant plus loin (×2), jugée par la porte métabolique INCHANGÉE. Au
franchissement de FUSE_EPS, l'hybride cristallise dans l'ORIGINE (substrat durable) + la dérive
culture remise à plat (négation du delta accumulé). Pas d'état neuf sérialisé ⇒ SAVE non bumpé.

Preuve (1×100) : cristallisations seed 7/9/11/19 = 5/1/0/4 (ex-0 ; seed 11 : partenaires trop
lointains — D∞ > FUSE_EPS en 100 ans). make test 32/32 (les 2 bancs démographie gagnent diplo/routes
au lien), ASan muet, viewer 0-warning, lang-check 64, déterminisme HASH 7 de6e2229 reproductible.
Mondes stables (6 pays, 0 absorbés, commerce ~inchangé) ; S1 tient (archétypes > 0 partout). La
COMBINAISON forge runique × arcane reste 0 → S3 (frein faustien).

## (c 10) S3 — le frein faustien réconcilié → l'emblème s'allume (forge runique × arcane) (2026-06-14)

Diagnostic affiné (instrumentation FAUSTDIAG, retirée) : même S1 ouvrant les DEUX archétypes (NAIN+ELFE
accessibles chez TOUS), l'emblème (TECH_FORGE_RUNES, faustien) restait à 0 — non par le frein (l'appétit
d'un culte/pluraliste ×1.4 + la bénédiction le franchit déjà), mais par DEUX murs : (1) la PROFONDEUR —
l'emblème est tier-3 derrière la Poudrière, jamais atteinte par l'IA gloutonne (le mur de S1) ; (2)
l'absence de Transgresseur — w_faustian plafonne ~0.18, l'appétit ~0.84, aucun empire n'est assez fou.

Deux gestes (scps_ai.c) : (1) AI_TECH_FAUSTIAN 2.5→1.2 (la rencontre appétit/frein que demandait le
brief) ; (2) la QUÊTE de l'emblème — l'empire le PLUS faustien-enclin du monde (ai_faustian_appetite ≥
0.80, écarté des marchands/bâtisseurs par le filtre S1) BEELINE la chaîne (ai_step_toward : Poudrière →
Forge à runes) en épargnant à chaque pas. On NE touche PAS la foreuse (AI_FOREUSE_HUNGER intact).

Preuve (1×100) : combo forge runique × arcane seed 7/9/11/19 = 3/1/2/1 (ex-0 sur toutes) ; faustiens
déverrouillés > 0. La charge → Brèche la garde COÛTEUSE mais BORNÉE : mondes stables (6-8 pays, 0
absorbé), âges éveillés inchangés (2, pas de Brèche en masse). make test 32/32, ASan muet, viewer
0-warning, lang-check 64. SAVE non bumpé. L'arc Syncrétisme est bouclé : le commerce réveille
l'archétype (S1), cristallise la culture (S2), et allume l'emblème faustien (S3).

## (c 11) N1 — la carte nue : worldgen ne pose plus rien, SAUF les cités-états (2026-06-16)

Bascule de genèse : `econ_init` ne pose plus ni socles de matière, ni manufactures, ni niveaux
pour les EMPIRES — chaque région garde sa VOCATION (`REGION_RAW_KEEP`=3 brutes dominantes + vivrier
+ stratégiques rares, la traîne tombe), la carte naît NUE, l'IA/agency bâtissent dans le temps.

Trois compléments pour que la mise à nu reste vivable :
1. **Cités-états EXEMPTÉES** — elles tiennent le marché mondial (#5) : socles + voiles arcanes +
   manufactures au gisement + niveaux préservés (le carve-out porte sur `w->country[cid].role`).
2. **Pool tradable cité-état** (`CS_TRADE_POOL`=1000 bois/fer/argile/pierre sur la région-pivot) :
   le marché mondial le revend aux empires nus → ils IMPORTENT de quoi bâtir. A/B seed 9 : accession
   du 3e empire an 66 → 43, vrac aval 0.00 → 1.60, flux −4.7 → +15.4 or/mois.
3. **Marché de départ** (`agency_seed_capital_markets`) : chaque empire naît avec un `EDI_MARCHE`
   GRATUIT sur sa capitale (semé comme les Centres, chronicle/viewer). Effet seed 9 : hub tenu
   1/4 → 2/4, commerce via cités-états 62 % → 55 % (les empires négocient davantage chez eux).

Diagnostic clé (la question « tech / volonté IA / ressources ? ») : la pose autonome (§NF,
`econ_build_tick:783‑799`) est gatée par les RESSOURCES — la manufacture ne s'implante que si le
royaume sait la NOURRIR (intrant au pool OU stock importé ≥ `NF_STOCK_MIN`) + une pénurie (prix
≥ 1.8× base, automatique) + pop ≥ 80. La tech ne gate que 5 manufactures avancées ; la « volonté »
d'IA est la voie parallèle payée en or (`ai_build_manufacture`). Le charbon de la chaîne à feu/arcane
reste au joueur (la charbonnière) — hors pool, par choix.

Bancs recâblés (les fixtures dépendaient de bâtiments posés d'office) : `ai_demo` « Bâtisseur +K »
robustifié (monde nu ⇒ digestion permanente, K → `builds_other` ; on garde « métabolise le plus »,
`w_build` STRICT) · `social_demo` brasserie isolée (`owner=-1`, sinon diluée P1 sur les sœurs nues) ·
`econ_arcane_demo` forge nourrie en charbon (in2). make test **34/34**, lang-check 64, déterminisme
stable, ASan/UBSan muets. SAVE non bumpé (l'accès marché des empires — `hub_map` vers le Centre le
plus proche — est inchangé ; `re->stock` reste le store).

## (c 12) WG — worldgen-graphe : l'aptitude portuaire & les détroits émergents (2026-06-17)

Deux couches de GENÈSE lues par le moteur (membrane respectée : on lit des coordonnées du monde,
on n'assigne aucun bonus plat).

**(1) `Region.harbor` — l'aptitude portuaire (la FORME du littoral).** À la genèse,
`compute_harbor_suitability` (scps_world.c, après courants) pose sur CHAQUE région une coordonnée
[0..1] qui lit trois traits du trait de côte à l'avant-port : l'**ABRI** (part de terre dans
l'anneau 5×5 du recoin de mer le mieux enserré — une baie protège, un cap expose), la **PROFONDEUR**
du mouillage (cabotage/eaux peu profondes = rade franche ; un COURANT vif au pied = rade brutale),
la **LONGUEUR** de côte exploitable. Mélange `0.50·abri + 0.30·profondeur + 0.20·longueur`.
**`navy_best_coast`** (scps_navy.c) la LIT pour asseoir sa rade : score = `harbor + 0.35·pop/popmax
+ (capitale-côte ? 0.40 : 0)` — l'avantage de SIÈGE n'est plus absolu, une baie franche l'emporte
sur un cap capital exposé. Preuve (`SCPS_HARBORDIAG`) : seed 1 pays 20 ouvre sa rade région 43
(harbor 0.75) plutôt que sa capitale-côte région 98 (harbor **0.00**, un cap nu) ; seed 42 pays 8
prend région 8 (harbor 0.79) contre sa capitale-côte région 16 (harbor 0.00). « La FORME l'emporte
sur le siège. »

**(2) Les DÉTROITS émergents (chokepoints) — péage-au-tenant + valeur de blocus.**
`compute_chokepoints` (scps_world.c) scanne les cellules de mer : un goulet = un chenal mince (≤ 12
cellules, terre des DEUX côtés sur un axe) flanqué de CONTINENTS distincts (à défaut, deux régions
sur un chenal ≤ 6, robustesse quasi-pangée), dédupliqué par grappe (18 cell.), borné à 24. Le
**TENANT** = la région côtière la plus proche du goulet ; la **valeur de BLOCUS** [0..1] croît avec
l'étroitesse. Table DÉRIVÉE par seed (cache, hors sauvegarde — comme les ancres de mer). API
`world_chokepoints` / `world_route_chokepoint` (le goulet sur le segment des deux ancres, t∈]0.08,
0.92[) / `world_chokepoint_holder`. À la création d'une route MARITIME (`routes_order`), on pose
`TradeRoute.choke_region`/`choke_block` (le détroit franchi, géographie statique). En commerce
(`intertrade_tick`), si la route franchit un goulet TENU par un **TIERS** (ni l'un ni l'autre des
deux bouts), à la paix, hors embargo, son propriétaire **SKIME** `IT_CHOKE_TOLL·(0.4+0.6·blocus)`
de la valeur transportée — un **transfert exportateur→tenant** (l'importateur ne paie pas plus, le
verrou prélève ; conservation préservée). FX_TOLL_RECV au tenant.

**⚠ RE-BASELINE (le monde change : qui tient quel détroit, qui ouvre quelle rade).** La rade
choisie par `harbor` (vs l'ancienne « capitale-côte sinon la + peuplée ») change quelle région
bâtit le port ⇒ les routes maritimes diffèrent ⇒ le commerce diffère. Le péage de détroit
redistribue de l'or entre régions. Nouvelles valeurs de référence (chronicle, péage CUMULÉ/sim) :

Balayage chronicle 1 sim × **100 ans** (péage CUMULÉ sur la sim) :

| graine | goulets | tenus | routes mar. | franchissent | tiers/partie/vierge | routes taxées | péage cumulé | meilleur tenant |
|--------|---------|-------|-------------|--------------|---------------------|---------------|--------------|-----------------|
| 7      | 11      | 2     | 4           | 1            | 0/0/1               | 0             | 0            | —               |
| 9      | 11      | 5     | 6           | 1            | 0/1/0               | 0             | 0            | —               |
| 11     | 9       | 2     | 15          | 0            | 0/0/0               | 0             | 0            | —               |
| 19     | 9       | 4     | 6           | 1            | 0/0/1               | 0             | 0            | —               |
| **1**  | **17**  | **8** | **10**      | **4**        | **3/1/0**           | **3**         | **163 or**   | **pays 8 = 128 or** |

Le péage MORD sur les mondes FENDUS (seed 1/42 : empires de part et d'autre d'un détroit tenu par
un tiers — 3 routes taxées, pays 10 encaisse 16 or au verrou) ; sur un monde mono-continent à 2
empires (seed 7) la rare route qui franchit un goulet tombe sur un flanc VIERGE → 0 péage, et seed 11
(routes côtières courtes) n'en franchit aucun → 0 péage. Émergent, non forcé : la géographie décide.
Télémétrie neuve : ligne « détroits (WG) » (goulets/tenus/routes taxées/péage cumulé) + diags
`SCPS_CHOKEDIAG` (routes vs détroits : tiers/partie/vierge) et `SCPS_HARBORDIAG` (la rade choisie).

**Déterminisme (5 graines × 12 ans, `make determinism`).** STABLE (deux runs identiques convergent)
sur la NOUVELLE baseline. Le HASH BOUGE là où la rade par `harbor` ou le péage de tiers diverge en
12 ans : **HASH 7 `3f17e5cc` et 108 `36684fb2` INCHANGÉS** (worlds dont les choix de port/commerce
ne changent pas avant l'an 12) ; **209 `19585d19`→`f4368e2c`, 310 `ec502e7a`→`1be2fd49`,
411 `0e0eef86`→`77941c52`** re-baselinés.

**SAVE bumpé v25→v26** : `Region` GAGNE `harbor` (float) ⇒ `sizeof(World)` change ; `TradeRoute`
GAGNE `choke_region`/`choke_block` ⇒ `sizeof(RouteNetwork)` change → « ère antérieure » (< v26
refusé). `save_sane` revalide `harbor∈[0,1]` (lu par navy_best_coast pour SCORER la rade) et
`choke_region∈[-1,n_regions)` (lu par intertrade pour trouver le tenant). make : core 35/35,
monde_reel 10/10, routes 4/4, intertrade 25/25, lang-check 64, ASan/UBSan muets.
## (c 13) G2 — LE DIRECTEUR-AMPLITUDE : trauma → amplitude → présage (2026-06-17)

Un « directeur » NARRATIF déterministe par-dessus le directeur d'événements (§F), entièrement
dans `scps_events.{h,c}` (+ bump SAVE). Quatre ressorts, tous SÉRIALISÉS (dans `Director`, donc
le blob `EVNT` de la save) et tous DÉTERMINISTES (aucune fonction du hasard ; lecture de l'état
du monde) :
1. **`adapt_days` — l'INTÉGRATEUR DE TRAUMATISME** : un compteur de jours-de-tension qui MONTE
   sous les chocs et REDESCEND au calme. La charge est ∝ la **température directeur T** (qui
   agrège DÉJÀ guerre/famine/révolte/fracture/SI, §F1) — `+AMPL_TRAUMA_CHARGE·T` jours/an ; la
   décharge est une **demi-vie** (`AMPL_TRAUMA_HALF`=900 j). Borné `[0..AMPL_TRAUMA_MAX]`.
2. **`amplitude = f(adapt_days)`** — saturation douce `adapt/(adapt+SCALE)` ∈ [0..1] : HAUTE
   juste après un choc (le monde « vibre »), BASSE au ronron. C'est la **lecture UI/télémétrie**.
3. **`budget` ∝ POP·RICHESSE·TEMPS** — des points de mise en scène qu'un monde riche/peuplé/âgé
   accumule plus vite (`AMPL_BUDGET_POP`/1000 hab + `AMPL_BUDGET_GOLD`/1000 or, par an, plafond
   `AMPL_BUDGET_CAP`). La pop = Σ strata des régions possédées ; l'or = Σ `econ_country_gold`.
4. **la boucle TALE** — fait NOTABLE → MÉMOIRE durable → AUGURE. Un événement dirigé (§F) OU un
   âge (§4) inscrit une **MÉMOIRE** horodatée dans un anneau `mem[DIR_MEM_CAP=16]` ; plus tard,
   quand `amplitude ≥ AMPL_OMEN_AMPL` ET `budget ≥ AMPL_OMEN_COST`, le **plus VIEUX** fait encore
   en mémoire RESSURGIT en **PRÉSAGE** (`omens++`), qui **dépense** le budget et **consume** la
   trace (la mémoire a nourri le récit). Tout en jours de jeu.

Branchement : `director_tick` calcule T (déjà), puis fait avancer l'amplitude **À CHAQUE SCAN**
(annuel) — même au calme, sinon l'intégrateur ne décroîtrait jamais. La membrane est respectée :
ces champs sont une COUCHE NARRATIVE qui LIT le monde et n'y RÉ-ÉCRIT JAMAIS (le moteur ne lit pas
adapt_days/amplitude/budget) — d'où le point clé ci-dessous.

⚠ **RE-BASELINE — la SAVE seulement, PAS le gameplay.** `sizeof(EventsState)` change (le blob
`EVNT` s'élargit : `adapt_days/budget/amplitude/max_amplitude` + `mem[16]` + `mem_head` + `omens`)
⇒ **`SAVE_VERSION` 25→26**, les saves **<v26 sont REFUSÉS** (« ère antérieure »). MAIS le
**HASH chronique est IDENTIQUE à la base** (`HASH 7 3f17e5cc` AVANT==APRÈS, vérifié en bâtissant
la base HEAD à part) : `chronicle_sim_hash` ne hashe pas `EventsState`, et SURTOUT le directeur-
amplitude ne MUTE aucun état moteur. `make determinism` reste STABLE et INCHANGÉ — la re-baseline
est strictement le format de sauvegarde, pas le monde.

`save_sane` (v26) revalide le champ : `director_save_sane(ev, SCPS_MAX_COUNTRY²)` — `mem_head` borne
l'écriture de l'anneau (un index forgé déborderait), chaque `mem.kind` est `[0..DMEM_KIND_COUNT)`,
chaque `mem.subject` est `-1` (monde) ou `< SCPS_MAX_COUNTRY²` (l'encodage Amnistie a·MAX+b),
`omens ≥ 0`. La garde vit AVEC la struct (scps_events) ; viewer l'appelle. **Testée headless**
(`events_demo` §8 (e)) : `mem_head` fou (déborde/négatif), `kind` hors enum, `subject` hors borne
(+/−) sont TOUS refusés ; l'état sain est accepté et RE-accepté après restauration.

**PREUVE — l'amplitude monte au choc, retombe au calme** (`SCPS_AMPLDIAG=1 ./chronicle 9 1 80`,
fenêtre lisible) : pic **0.442** à l'an-24 (la crête du choc : T 0.62→0.54, 4 révoltés) puis
DÉCROISSANCE monotone sur le calme an-27→60 : **0.416 → 0.299** (T retombe 0.45→0.28 ; `adapt_days`
396→214 j). La courbe REMONTE à chaque nouveau choc (p.ex. an-70 T 0.48 ⇒ ampl 0.299→0.363). Sur
seed 9 / 150 ans : **8 présages** émis (★ marqués : an-28, 70, 71, 88, 104, 119, 133, 148). Synthèse
chronique neuve : « directeur-amplitude (G2) : pic moy · fin moy (le monde se calme) · N présage(s)
· l'amplitude a VIBRÉ dans X/Y sims ». Diag : `SCPS_AMPLDIAG` (la courbe année-par-année).

**Tunables** (registre J, §G2) : `AMPL_TRAUMA_CHARGE` 180 · `AMPL_TRAUMA_HALF` 900 · `AMPL_TRAUMA_MAX`
2000 · `AMPL_TRAUMA_SCALE` 500 · `AMPL_BUDGET_POP` 0.02 · `AMPL_BUDGET_GOLD` 0.01 · `AMPL_BUDGET_CAP`
400 · `AMPL_OMEN_COST` 60 · `AMPL_OMEN_AMPL` 0.35. `make test` **34/34** (`events_demo` 27→**41/41** :
+14 contrôles G2), lang-check **64** (aucun texte joueur — couche moteur ; la télémétrie reste FR par
politique), ASan/UBSan muets, déterminisme STABLE & INCHANGÉ, viewer 0-warning.

## (c quater) Arc P — « la guerre prend du terrain » (2026-06-13)

Racine : 217 batailles, **0 occupation** — après chaque bataille TOUT le monde
passait FA_IDLE (le vainqueur décrochait au lieu de presser), et même quand un
siège partait, le défenseur (toujours plus fort) gagnait le secours et le siège
ne tombait jamais. Mesuré au profileur : assaillant ~797 de réserve vs défenseur
~1290, le défenseur l'emportait 217/217.

- **P1 — le vainqueur PRESSE** : `bt_end` désigne le vainqueur (principal en lice,
  non brisé, moral le plus haut) et l'envoie ASSIÉGER la région contestée
  (`bt_press_siege`) au lieu de décrocher ; `bt_rout` ne remet plus le vainqueur
  en IDLE. Pas de presse hors guerre (paix éclatée).
- **P2 — le siège TOMBE** : un secours défait laisse la place sans espoir → elle
  capitule en `BT_RELIEF_FALL` (30 j, ≤ la fenêtre `BT_BRISEE_J`=45 j où l'armée
  ennemie gît brisée). Sans ça, le secours se reformait et resettait le siège à
  l'infini. `days_left` est sérialisé ⇒ sauver/recharger fidèle, **0 bump SAVE**.
  La chaîne siège→`taken_region`→`diplo_occupy`/`diplo_liberate`→transfert à la
  paix était intacte ; il manquait juste que le siège FINISSE.
- **P3 / calibrage (valeurs dirigées)** : bonus défensif LÉGER `BT_DEF_EDGE`=0.10
  (le secours doit pouvoir l'emporter) ; curée ALLÉGÉE (cap 0.22→0.12, socle 0.06 —
  les armées survivent, la guerre s'inscrit dans la durée) ; doctrine d'attaque
  `BT_ATK_RATIO`=1.2 (on n'assaille qu'avec 1,2× le défenseur — fin de l'usure à
  vide) ; décrochage l'EXCEPTION `BT_DECROCHE`=0.22 (la bataille se DÉCIDE).
- **Preuve (chronique 1×60 graine 7)** : occupations **14 posées** · provinces
  **transférées à la paix 18** · **2 sécessions** (la carte respire) · **0 %
  décrochage** (39 batailles, 39 déroutes) · 7 guerres. `make test` **32/32** ·
  0 warning · campaign_demo 19/19 (banc de ralliement re-calé sur la curée légère :
  le noyau survivant peut dépasser 60 %, le ralliement ne réduit jamais).
- **Réserve** : ratio poursuite/choc **0,7×** (cible brief [2,5]). Il était DÉJÀ
  sous la cible (1,7×) avant l'arc ; réduire la poursuite (consigne) fait dominer
  le choc, et sous `BT_CHOC_MORTS`≈0.006 les morts de choc se quantifient à 0 (pas
  de milieu lisse). Tension ASSUMÉE : le modèle « batailles décisives mais non
  annihilantes » (armées qui survivent → guerres qui durent → terrain qui change
  de main) prime sur l'aspiration « la poursuite domine » de l'ancien modèle.

## (c ter) N3.1 — Frontières hiérarchiques, zoom-stables (2026-06-12)

- **Fix 1 (flags symétriques)** : `compute_render_flags` compare aux QUATRE voisins
  (E,W,N,S) sur la PLEINE grille (hors-grille = id -1) — fini le trait peint « à
  l'intérieur » d'un seul côté de la couture (inversion sur relief) et la dernière
  rangée/colonne nues. Ces flags restent pour la sélection/vue ressources/minicarte ;
  la hiérarchie politique ne les emprunte plus.
- **Fix 2 (strokes espace écran)** : trois listes de SEGMENTS (arêtes en coins de
  cellule, niveau le plus FORT du joint : pays > région > province, riveraines ra/rb
  stockées), extraites au BAKE et rebâties au seul changement de souveraineté (photo
  owner-effectif + seed, memcmp par frame) — jamais par frame. Tracé batché
  `SDL_RenderGeometry` (un appel par niveau ; repli 3 lignes parallèles < 2.0.18) :
  province **2 px** 0xFF1A2230, région **3 px** 0xFF141A26 (vue Régions), pays
  **5 px** 0xFF0A0E16 — largeur ÉCRAN constante à tout zoom, bouts ronds (hexagone)
  aux extrémités ⇒ pas de trou aux jonctions ≥3. Contour de pays FERMÉ : pays↔pays,
  bord externe vers vierge/mer (côte politique) et bords de carte. Z-order strict :
  terrain → prov → région → pays (domine) → sélection dorée (strokes) → glyphes →
  étiquettes. `RenderParams.screen_strokes` coupe le bake 1-cellule sur la carte
  principale (minicarte/outils : bake historique inchangé).
- **Membrane** : ids seulement (région/province/owner), zéro flottant SCPS ; rien en
  SAVE (recalculé au chargement). Preuves : capture `--shot` graine 7 — contours pays
  5 px fermés (îles comprises), jonctions pleines, sélection au-dessus ; 32/32 bancs ·
  0 warning · lang-check 64.

## (c bis) Arcs L + M v1 (2026-06-12) — BOUCLÉS, preuves au banc

- **L6** ai_demo 23/23 (largeur pondérée par l'éthos ; correctif post-gate : la largeur du
  conquérant est l'ARSENAL — acheter des armes — pas la garnison qui blindait le monde et
  tuait l'opportunisme : 1×30 mesuré à 0 guerre, corrigé).
- **L1** l'interception (campaign_redirect ; défense mensuelle → sortie de garnison ;
  l'attaquant re-cible ; la campagne respire au mois) — graine 7 : 2 → **84 batailles**.
- **L2** le ralliement (40-60 %, 30-60 j, une fois/guerre, noyau survivant) — SAVE 14.
- **L3** calibré par grille (arc J) : BT_CHOC_MORTS 0.008 → **0.0045** ⇒ ratio
  poursuite/choc **2.6x ∈ [2,5]** (graine 7, ancienne genèse). Traçabilité : SCPS_TUNE="BT_CHOC_MORTS=0.0045".
  Leçons : CUREE_CAP ne mord pas (P<cap) ; <0.004 = falaise d'entiers (0 mort de choc).
- **L4-recal** (2026-06) genèse L4 (redistribution orphelins) invalide L3 pour graine 7 :
  0.0045 → ratio 88x. Regrille 0.0045→0.02 → point cible : **BT_CHOC_MORTS=0.006** ⇒ ratio
  **2.4x ∈ [2,5]** (graine 7, 1×30). Traçabilité : SCPS_TUNE="BT_CHOC_MORTS=0.006".
- **L4** la genèse peuple les continents (5 graines prouvées ; re-baseline notée CLAUDE.md).
- **L5** colonies outre-mer (portes ×2, Port+coque, ≤20 j de courants, traversée comptée).
- **M0-M7 v1** : design doc versé · ETHOS_FN verbatim · pôle par factions (Transgresseur
  orthogonal) · succession contextuelle + hystérésis 360 j + frères bloqués · +5 édifices
  (SAVE 15) · Alambic + essence purifiée + LE PUITS (charge 0.28→0.00 au banc) · score
  tech multiplicatif (souche×éthos×credo×matière) · 15 gabarits §25 (3 variantes/fork).
  forks_demo **34/34**. RESTE (hors v1 ou passe UI) : routage IA du savoir forké, journal
  des forks (bande UI), §23.3 interdit (cristallisation, navires, uint64).
- Transverse : make test **32/32** · 0 warning GCC+clang · ASan muet · déterminisme ·
  lang-check 64 · SAVE 13→16 (I/H : 13 · rally : 14 · forks : 15 · region_ids[32] : 16).
- **H3 — guerre trans-mer (2026-06)** : `countries_sea_adjacent` ajoutée dans scps_ai.c
  (TOUS les ports côtiers de a testés contre toutes les côtes de b, portée ≤ 400 j de
  courants = tout bassin atteignable) ; `ai_pick_rival` étendue (pénalité ×0.60 outre-mer).
  CB existants applicables sans ajout (CB_ECONOMIC, CB_SUBJUGATION, CB_RELIGIOUS,
  CB_TERRITORIAL par rancune). Armées via `campaign_order_sea` (§6 existant). `make test`
  `make test` 32/32 · 0 warning · overhead +4 % (62 s → 65 s, graine 42 / 40 ans) ·
  graine 7 mono-cont. : 2 guerres terrestres / 0 traversée (H3 inactif sans mer, CORRECT) ·
  graine 42 bi-cont. 16 pays / 40 ans : 2 guerres terrestres · 8 raids piraterie ·
  0 traversée (CB_ANTIPIRATERIE en construction ; guerre outre-mer attendue à 60-80 ans quand
  la rancune de piraterie dépasse le seuil CB — le chemin de code H3 est vérifié, les conditions
  de déclenchement prennent 40-80 ans pour se former dans un monde naissant).

---

## (c) Recommandations (ordre de correction) — arc K BOUCLÉ

K4a (events) ✓ → K4b (diplo) ✓ → K4c (warhost) ✓ → **K5** (racine IA = la spirale de
friche) ✓ → **K6** (coercitif : cas-test sur-extrême, pas un chaînon manquant) ✓ → K7
(hygiène : clang 0 warning, ASan muet) ✓. `make test` 30/31. La dette « Bâtisseur +K »
demande du CONTENU (chaîne K plus longue / bâti multi-régions) — hors périmètre « pas de
système neuf », laissée en l'état documenté (déjà signalée dans CLAUDE.md).

---

## (d) Correctifs DÉJÀ intégrés (arc K)

- **K1 — build/linkage** : `faction_name→tr()` (G4) rendait scps_factions.o dépendant de
  scps_lang.o ; 12 listes `_OBJS` linkaient factions sans lang → « undefined reference to
  tr ». Corrigé par liste (multi-ligne, sans double). `ifdef DEV/WIN` → `ifeq ($(VAR),1)`
  (le piège « DEV=false = dev »). Cible `scps` : message clair si SDL absent.
- **K2 — membrane** : `faction_name`/`edifice_name` migrés au readout (tr() y est
  légitime) ; scps_factions.c / scps_agency.c n'incluent plus scps_lang.h. AUDIT :
  aucun module moteur n'inclut scps_lang.h ; aucun tr() hors readout/viewer.
- **K3 — instrument** : `make test` (30 verts / 1 rouge, rc≠0 si un rouge) — la
  non-régression de tout l'arc.
- **K4a — events** : `events_fire_risk` retourne 0 dès `forest < 0.01` (le feu de forêt
  ne naît plus sans forêt). → 27/27.
- **K4b — diplo (cooldown)** : diagnostic CORRIGÉ — la racine n'était PAS la migration
  tune mais une garde `colonized` : le décrément de `pillage_cd` vivait DANS la boucle
  des régions colonisées, donc une région `colonized=0` ne le purgeait jamais. Décrément
  déplacé en tête de `econ_tick`, pour TOUTES les régions. → 49/49.
- **K4c — warhost (levée)** : diagnostic CORRIGÉ — la solde I1 draine bien le trésor,
  mais le recrutement puise dans un scratch ∝ pop, PAS dans le trésor : la paix
  n'était donc pas freinée par le coût, elle ACCUMULAIT (batch +2/an vers le plafond de
  pop) pendant que la guerre SATURAIT son pool en an 0 (recrutement tout-ou-rien). Un
  pays paisible à grand pool dépassait un pays en guerre à petit pool. Fix : la guerre
  MOBILISE vers le plafond de pop ; la paix DÉMOBILISE vers une GARNISON ∝ jauge
  (`WH_GARRISON_UNITS`) — au-dessus on dégraisse (`wh_shed`, la pop retourne au pool),
  en-dessous on complète à l'entretien. La solde I1 reste le COÛT qui, via la garde IG,
  abaisse la jauge → la garnison → démobilise PAR LE COÛT. → 4/4 (8 graines vérifiées).
- **K5 — IA gelée : diagnostic CORRIGÉ (PAS le crédit de largeur)** : l'instrumentation
  montre les archétypes NON aplatis mais GELÉS — les ponctions d'or arc I (admin/cour/
  encadrement/surtaxe IPM) vidaient le trésor d'une capitale-test isolée à 0 → la région
  tombait en FRICHE (prod 0.6×) → satisfaction effondrée → pression fiscale (I) montée →
  SI<5 → frein de consolidation BLOQUÉ à 1 (le Dominateur consolide au lieu de conquérir ;
  le Mercantile, à trésor 0, ne peut bâtir son grenier → verrou de famine, 0 route). Fix
  (scps_econ.c) : (1) RÉSERVE D'EXPLOITATION `SINK_FLOOR` — entretien & redépense d'État ne
  vident jamais sous ce plancher (un État garde de quoi bâtir/amorcer) ; (2) FRICHE = surbâti
  CATASTROPHIQUE seulement (entretien > TOUT le trésor), plus la falaise sur une thune fine ;
  (3) les ponctions anti-thésaurisation (admin/cour/encadrement/surtaxe IPM) ne mordent qu'AU-
  DESSUS du seuil de HOARDING (`COURT_FLOOR`) — elles saignent les gros trésors, pas le bas de
  laine qui finance les chantiers. → ai_demo 22/23, structural_demo 15/16 ; chronique 40 a :
  trésor moy ~15.9k (mieux que les 21-30k d'avant), flux +20/mois (bord de bande), 0 friche.
  Reste l'ÉGALITÉ « Bâtisseur +K » (dette de contenu, supra).
- **K6 — structural « la coercition dissout L » : diagnostic CORRIGÉ (PAS un chaînon
  manquant)** : le mécanisme de dissolution EXISTE et MARCHE — l'Âge des Lumières pose
  `age_lumiere_solvent` (+2) qui ronge la légitimité effective `L − solvant·H/10`. L'invisible
  venait du CAS-TEST : le coercitif était façonné H=9·L=2, plus extrême que tout régime réel
  (la σ-forme A1 est CALÉE sur monde_reel : Russie 9.6, Iran 9.2), si bien que sa fragilité
  SATURAIT déjà à 10.0 — la dissolution n'avait plus de marge pour monter. Le moteur est juste
  (σ-forme verrouillée par core_demo 9.990 + monde_reel) ; c'est l'ASSERTION qui était
  sur-extrême. Fix : le cas-test coercitif passe à L=6 (~9.2, l'Iran) — franchement fragile
  MAIS avec la marge où le solvant fait grimper la fragilité (9.2→9.8). → structural_demo 16/16.
- **K7 — hygiène** : clang `-Wall -Wextra` 0 warning sur tout `scps/*.c` (15 recettes sans
  repli initialisent explicitement `alt1` = zéro-init intentionnel ; 2 variables mortes
  retirées) ; GCC 0 warning ; ASan+UBSan muets sur `chronicle_asan` ; déterminisme
  byte-identique (md5 inchangé avant/après) ; `lang-check` OK (64, base) ; membrane propre.
- **Bilan arc K** : `make test` 30/31 — la SEULE rouge est « Bâtisseur +K » (égalité à 3,
  CONTENU manquant — chaîne K profonde de 3, bâti mono-région ; hors périmètre « pas de
  système neuf »). Preuves : 0 warning GCC+clang · ASan muet · déterminisme · lang-check ·
  membrane · gold band 40 a en bande (trésor ~15.9k, flux +20). Les bancs structural/ai
  gardent une sensibilité de graine résiduelle (worldgen) sur 1-2 assertions hors graine
  par défaut — la cible `make test` (graine 42) est verte.
