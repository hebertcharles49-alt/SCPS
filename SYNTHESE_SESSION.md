# SYNTHÈSE DE SESSION — handoff roulant (2026-07-17 soir — M15 + EXPORT : « Termine tout » FAIT)

## M15 FINITIONS + EXPORT scps.exe (HEAD 53d82aa + dist)
- **M15** (pre-m15, 5 commits) : F1 inflation — l'option A MESURÉE D'ABORD et DÉGRADANTE
  (−0.40 %/an) ⇒ option B retenue (INFLATION_CAP seul 1.6→2.0) : dérive moyenne **+0.17 %/an,
  positive pour la première fois**, bandes tenues · F2 ai_speculate_tick (le dernier site M0)
  converti en transferts conservés — résidu 209s3 halvé (+8.3k→+4.6k/an, un autre site porte
  le reste) · F3 ré-embarquement d'un corps actif (golden-neutre) · F4 choke-au-chemin-réel
  LIVRÉ GATÉ OFF — le chemin réel fait tomber le péage de détroit à 0 partout (les lanes
  évitent les goulets) : DÉCISION JOUEUR avant activation. Gates 39/39, DLL debug+release.
- **EXPORT** : packaging/windows/dist_godot/scps.exe (289 Mo, PCK embarqué, DLL release
  KERNEL32-only) — SMOKE TEST VERT en PATH System32 seul (worldgen graine 9 « archipel »
  complet, exit 0). Le jeu complet — 15 vagues monnaie + UI-MONNAIE + TECH/FINS/FAUSTIEN +
  ROUTES/ANTISPAG/MARITIME + savoir×2 — est ENTRE LES MAINS DU JOUEUR pour la première fois.
- DÉCISIONS JOUEUR EN ATTENTE : activer F4 (choke réel = tuer le péage de détroit) ou le
  laisser mort · le résidu +4.6k/an de 209s3 (l'autre site, non identifié) · les mécanismes
  morts de l'œil-neuf (coups d'État 1/100, esclavage 9/100, morts-poursuite 5.9×, cohésion
  gelée) · élargir la fenêtre d'exode de fin (13/100 sims seulement) · tech→rendement ?

## M13 + ROUTES + ANTISPAG + MARITIME (HEAD d135da2, save v96, 39/39)
- **M13** (pre-m13, 4 commits) : « si y'a personne, y'a pas de péage » — 3 sites région-grain
  gatés par econ_region_has_keeper (la PORTEUSE, pas region.colonized qui ne mordait jamais) ;
  colonisation TENUE (−3.1 % bidirectionnelle vs le −14/−34 % du re-routage M3i) ; les 3
  breaches du giga avaient DÉJÀ disparu sur pre-m13 (tués par TECH→M14) — résidu +9k/an
  sous seuil, suspect suivant : ai_speculate_tick · découvertes d'or 52→81 % (goulot =
  éligibilité 32 %, MTTH boost) · P3 inflation : proposition NON landée (CAP sature 8/18
  sims ; options A: MINT 0.75+CAP 2.0 / B: cap seul / C: amortir pf) — DÉCISION JOUEUR.
- **ROUTES** (pre-routes) : le pathfinding terrestre existait depuis juin (A* terrain-aware,
  corridors ×0.30, cache) — la vague a RÉPARÉ viewer_audit (qui mentait « OK » à vide) et
  les routes-en-mer (19→1, 18→1), purgé 30 lignes mortes, perf 25 ms COLD.
- **ANTISPAG** (pre-antispag) : spaghetti −46.5 % (3279→1753 paires) — magnétisme R=1.4/2
  passes + épaisseur ∝ multiplicité + dédup ponts ; la 1re métrique MENTAIT (66 % = bruit
  de phase) ; consensus de grille essayé-mesuré-PIRE-reverti ; résidu = faisceaux légitimes,
  plateau ~47 % (au-delà : matching arc-length, Restes).
- **MARITIME** (pre-maritime, 5 commits) : la mer savait tout mais ne montrait rien
  (world_sea_days = Dijkstra complet SANS chemin ; armées IA embarquaient déjà, le joueur
  n'avait AUCUN geste). N2 lanes port-à-port (A* mer-pas-lacs, cabotage rail 2e rangée,
  tampon exact, cache, sea_paths(), CPU 78 ms COLD/0.04 WARM) · N3 CMD_MOVE_ARMY replie
  sur l'embarquement (SEA_TRAVEL, golden-neutre) + reader sea_travel() · N4 portulan
  pointillés (dédup par bin — les tirets déphasés faisaient un trait plein ; la boucle de
  tirets qui HANGEAIT 11 min corrigée) · golden byte-identique AUX DÉFAUTS (zéro re-baseline)
  · DLL debug+release. Piège : biome SHALLOW des lacs ⇒ cell.sea≠0 n'exclut PAS les lacs.
- RESTES NOUVEAUX : ré-embarquement d'un corps actif (verbe manquant) · choke segment-droit
  vs chemin réel · P3 inflation à trancher · ai_speculate_tick (le dernier site M0) · EXPORT.

## M14 AUDIT-2 + SAVOIR ×2 (HEAD 76994fa, SAVE v96, make test 39/39 — PREMIÈRE)
- **SAVOIR ×2** (ee79945) : SAVOIR_W_* doublés (décision joueur, testé par override avant
  bascule) — médiane 37→48 techs, machines faustiennes ×1.7-2.8, zéro-tech 0, golden
  INCHANGÉ (aucune tech ne tombe en 12 ans).
- **M14** (10 commits, tag pre-m14) — audit externe n°2, 9/9 confirmés : B1 trésor
  négatif n'inverse plus les paiements (6 sites, grep généralisé) · B2 dette fantôme
  fermée (econ_region_treasury_add) + TOCTOU can_spend/spend · B3 amortissement sur
  dette périmée = 10.2 or DÉTRUITS/tick mesurés au banc → 0.004 résidu float · B4
  markup réservé au plafond · B5 CountryDebt ventilé to_elite/to_bourgeois (v96 —
  l'emprunt bourgeois ne rembourse plus les élites) · B6 une-seule-vérité COMPLÉTÉE
  (17 sites convertis, dont le don mercantile de fronde qui s'évaporait) · B7 échéance
  UI corrigée (~4.6× trop basse ; ScpsDebt.due, DLL debug+release rebuildées) · B8
  sliders restaurés [0.1, 2.0] (la spec joueur) · B9 setenv shim + stack + fixture ⇒
  **39/39 bancs Windows pour la première fois**. Sweep apparié : invariant 0/9 des
  deux côtés, banqueroutes −8 %, colonisation +16 %, dette trackée ×3.1 (la fantôme
  devenue visible — le but).
- RESTES : site WILD péages parqués (désigné depuis M11, toujours ouvert) · M13 (#119 —
  breaches giga ~3/100, découvertes or 47 %, inflation déflationniste à l'échelle) ·
  arbitrages joueur dormants (mécanismes morts œil-neuf, tech→rendement) · EXPORT.

## TECH + FINS + FAUSTIEN — 3 vagues moteur livrées après l'UI-MONNAIE (HEAD a8658f5)
- **TECH** (pre-tech, 6 commits) : le 22 % d'empires à zéro tech = des pays SANS IA
  (ai_on=0 — fragments de resplit cataclysme §27 jamais adoptés + course d'agrégat des
  sécessions). Fix : adoption mensuelle idempotente + héritage de l'arbre parent +
  f_satisfaction [0.5..1.25] dans econ_country_savoir (la formule joueur pop×classes×
  bâtiments×métabolisation existait déjà aux ¾ — SAVOIR_W_*). Zéro-tech 21.6→3.7 %
  (giga), médiane 17→27, ancre max 65 intacte. ⚠ re->tech (econ:4786) = VESTIGE (ne
  nourrit rien — a trompé 2 audits, documenté Piège). Verdict pre-monnaie : PRÉEXISTANT
  (38.6 % avant le chantier monnaie).
- **FINS** (pre-fins, 3 commits) : 4 fins gatées derrière ENTROPY_FIN=55 (bimodal —
  quasi aucun monde dans la bande) ; SANG découplé, ENTROPY_FIN 25, fallback 2→7 ⇒
  ratio 6:1→1.8:1 mais AUCUNE 44/100. Merveille : fondation HUMAN_PLAYER-gatée — jamais
  mesurable en headless (tous les chiffres « métab MAX x/6 » = plafond théorique).
- **FAUSTIEN** (pre-faustien, 5 commits) : décisions joueur — spawn fer céleste +48 %,
  FAUST_YIELD_MULT 2.0, ENTROPY_PER_USE 0.10 (chaque usage pousse vers la fin), foreuse
  = lot 7/7 au ratio 2:1 commun:précieux (+ redevance minière ENFIN branchée sur le
  panier → inflation émergente), corne +alcool, réplicateur +PMOD_MUTATION, RÉCHAUFFEMENT
  = BACKUP INCONDITIONNEL (« AUCUNE == 0 définitif » — gate prouvé 0/9), Merveille 400.
  RESTES : Laborer −2/−12 pts (prix de X3, non recalibré — À SURVEILLER), usage machines
  toujours dormant à n=9 (gate IA de construction non touché), inflation-foreuse câblée
  non mesurée à l'échelle.
- **RÈGLE JOUEUR NOUVELLE** : GIGA sweeps SUR DEMANDE SEULEMENT (CPU) — appariés 3×3 =
  standard, plafond ~30-40 sims/vague (mémoire scps-sweep-budget).
- DLL rebuildée après chaque vague. Backlog : M13 (#119 — fuite invariant 3/100,
  découvertes or 47 %, inflation déflationniste à l'échelle) · arbitrages joueur en
  attente (mécanismes morts œil-neuf, tech→rendement ?) · EXPORT scps.exe (toujours
  le plus gros backlog de test manuel).

## UI-MONNAIE — U1-U4 LIVRÉS (0f2f9de, DLL rebuild scons)
La seule tâche ouverte de la synthèse précédente (« UI-MONNAIE ») est FAITE : l'arc
M0→M12 est maintenant VISIBLE et JOUABLE côté Godot. Tag `pre-ui-monnaie` posé sur
1e18cb8. 6 commits (readers+binding · U1 onglet Monnaie · U2 prix en direct · U3
emprunt diplo · U4 journal · probe monnaie_shot). Détail complet : TROUVAILLES.md
§« CHANTIER UI-MONNAIE ».
- **U1** : onglet MONNAIE neuf dans `budget_panel_v2.gd` (Balance/Marché existaient,
  jamais câblés — page-stack ajoutée) — réserve, frappe+curseur, débase+curseur+
  avertissement, dette (total/ordres/créancier/taux/échéance), 3 boutons d'emprunt
  (confirmation UI-4), banqueroute volontaire (bouton rouge, conséquences en mots
  AVANT, confirmation UI-4), 3 curseurs fiscaux AVEC satisfaction en regard. Onglet
  Marché câblé (prix + tendance /mois, suivi client).
- **U2** : `province_panel_v2.gd` — hover « prix national N × M t/mois = P/mois » sur
  les raws ET manufactures (nouveau reader `scps_province_res_price`) ; `topbar.gd` —
  cellule ÉCONOMIE « Prix » (indice national, tendance au survol).
- **U3** : `country_actions.gd` — bouton « Demander un emprunt » (ACTIONS ÉCONOMIQUES),
  état en mots (accordé/refusé/aucune demande).
- **U4** : `alerts.gd` — 4 conditions monétaires (banqueroute/débase, soi + adversaires
  connus), motif édge-detection existant, zéro nouveau canal moteur.
- **6 nouveaux readers scps_api** (PURS, aucun verbe neuf — tous existaient déjà M8/M9,
  juste jamais bindés) : `scps_country_debt`, `scps_country_price_level`,
  `scps_world_price_index`, `scps_country_debase_frac`, `scps_country_bankruptcy_scar`,
  `scps_province_res_price`. Binding Godot complet (scps_sim_node.h/.cpp).
- Vérifié : `scons` propre, probe `monnaie_shot.gd` (6 PNG, zéro SCRIPT ERROR),
  `make lang-check` 0=0 (aucun fichier C-viewer touché — la doctrine STR_* ne couvre
  QUE le viewer console, pas le GDScript, précédent confirmé sur tout le codebase UI).
- **Piège découvert** : `--headless` HANG (pas juste noir) sur ce toolchain pour toute
  probe qui capture un PNG (`frame_post_draw` n'arrive jamais) — tourner SANS
  `--headless` (fenêtré, GPU réel répond, le process quitte proprement quand même).

## RESTES UI-MONNAIE (voir TROUVAILLES pour le détail)
- « Gros emprunts d'États voisins » (dernier item U4) NON fait — pas de seuil scale-
  invariant honnête sans un reader `debt_ceiling`/PIB-pays.
- Hover prix (U2) confirmé CORRECT par lecteur (console) mais pas confirmé BEAU à
  l'écran avec un vrai nombre non-nul (monde de test sans province à or/cuivre).
- Journal U4 (banqueroute/débase) jamais observé EN CONDITIONS RÉELLES (aucune
  banqueroute survenue dans le monde de test) — mécanisme à haute confiance, non
  visuellement prouvé en action.
- EXPORT scps.exe — toujours pas fait (backlog de test manuel au plus haut historique).

---

# (archive) SYNTHÈSE 2026-07-16 soir — MONNAIE ARC 2 COMPLET M5→M12 (save v95, HEAD b05dca6)
Journée du 16 : sept vagues moteur enchaînées, chacune taguée (pre-m5 … pre-m12).
- **M5** revenu propre + assiette (toll 50/50 · réserve genèse 100/100 · conso payée
  déjà vraie depuis M3b — le vrai trou : ration vitale garantie + élasticité richesse).
- **M7** inflation séculaire ÉMERGENTE (+0.90 %/an à 10 empires, INFLATION_CAP 1.6 +
  MINT_* 0.6 — déplafonner seul ne suffisait pas) + DÉCOUVERTE D'OR par remplacement
  de la ressource commune dominante (slot-libre = 0 % éligible) ; choc Potosí 2× monde.
- **M8** cercle vertueux (satisfaction→capacité fiscale · curseurs PAR ORDRE · IA
  fiscale 60 %) — mais banqueroutes ×2 : le contrôleur bradait tax_mult à 0.34.
- **DIAG matrice C1×C3** : verdict = le contrôleur (C3), pas le couplage (C1).
- **M10** PALIERS DE BESOINS pop-empire (le système actif_needs existant était mort-né
  — tier 4 dès tick 1 ; needs_met jugeait contre le panier mature) : 1 besoin/palier,
  universel, générique, gate l'ACHAT (correctif audit), plancher tax_mult 0.75 →
  banqueroutes 1183→583, revenu +26 %.
- **M11** AUDIT-SOL 4/4 confirmés : frappe à parité pleine (vendeur payé), trésor
  une-seule-vérité (contrat en tête d'econ_aggregate_regions, plus de treasury+= nu),
  INTÉRÊT FIXE à l'origination (1000→1050, jamais plus) + défaut réel (échéances
  impayées→streak→faillite), credit_demo 20→48. Dette monde fin 597k→66k.
- **M12** ÉQUILIBRE DE BASE : la ligne coupable = l'achat d'État (−340/mois an 1 vs
  +250 de revenu total) + amorçage genèse price_level=1 re-déclenché 12 ans/pays neuf.
  STATE_BUY_FRAC 0.60 (la taxe générale, INTOUCHABLE — règle joueur : si Laborer casse,
  baisser LEURS taxes + monter leurs biens, jamais le 0.60) + PL_GENESIS prudent.
  RÉSULTAT : dette an 2 = 0 (9/9 sims), an 12 −98.9 %, banqueroutes 795→28, emprunt de
  paix rare (+63→+10 or/pays-an), colonisation +33 %, Laborer 60-71 % tenue, invariant 0/9.
- Doctrine actée : l'emprunt automatique au négatif RESTE ; l'emprunt sert l'urgence,
  pas le fonctionnement — c'est le BUDGET qu'on répare, pas le tuyau.
- DLL Godot re-buildée après CHAQUE vague. Piège agents récurrent (4/6 vagues) :
  « j'attends le moniteur d'arrière-plan » — relancer avec boucle de poll BLOQUANTE.

## PROCHAIN PAS (au choix du joueur)
1. **UI-MONNAIE** (#114, la seule tâche ouverte) : réserve/dette/banqueroute/débase +
   curseurs fiscaux par ordre + verbes d'emprunt (éco+diplo) + PRIX EN DIRECT (« combien
   de tonnes vais-je produire, combien ça rapporte instant T » — readers déjà prêts).
2. **EXPORT scps.exe** — M5→M12 jamais touché par un humain ; le backlog de test manuel
   du projet au plus haut historique.
3. Restes moteur : convergence prix-métal→parité toujours molle (payer le vendeur n'a
   pas suffi — signal M3f) · pression fiscale >100 % Bourgeois/Élites certaines époques
   (borné par la richesse, levier INCOME_TAX_RATE_*) · banqueroutes à 28/9 sims — le
   défaut est-il devenu TROP rare ? (à juger en jeu) · site WILD graine 11 désigné.

---

# (archive) SYNTHÈSE 2026-07-15 — MONNAIE ARC 1 M0→M3i (save v93, HEAD c901317)
- **M3h LA DÉBASE** (4942ec9..d83a3dd, v93) : sur-frappe au-delà de la parité (curseur
  BUDGET_DEBASE), payée en K_inst rongé à la capitale (debase_kdrain sérialisé) + rot
  factions + évasion émergente (econ_debase_tax_factor — K ne pilotait AUCUN canal
  fiscal avant). IA : dernier recours avant banqueroute forcée — l'échelle
  emprunt→débase→banqueroute vécue DANS L'ORDRE (mesuré, graine 9 pays 39 : streak
  3-4 débase, 5 banqueroute, récupération K en ~5 ans). Deux régimes : crise courte
  rationnelle / chronique ruineux (rot saturé 0.85). Écart documenté : invariant 8/9,
  graine 110 breach 387-460 % (bruit pré-existant, pas M3h).
- **M3i L'IMPÔT SUR LE REVENU** (b409f33..c901337, v93 inchangé) : le forfait
  per-capita remplacé par la retenue à la source sur les pools 42/20/38 + intérêts
  créanciers ; taux × valeur produite × curseur × (1−évasion) ; capitale = référence
  fiscale nationale. Neutralité an 5 : 0.93-1.09. Structurel assumé : l'impôt-production
  encaisse les chocs (guerre/révolte) que le forfait-pop amortissait. Exonération
  vitale CONSERVÉE (mesurée : sans elle, Laborer 44 % < bande). Invariant AMÉLIORÉ :
  0 breach/9 (le breach 110 de M3h a disparu). Kill-switch prouvé ×2 (un bug de fuite
  trouvé/corrigé dans econ_income_tax_rate_capital).
- Fuites secondaires : péages porteuses (~250k/région) ESSAYÉ-REVERTI (casse la bande
  colonisation, lien non tracé avec le pool P1) · ai_speculate_tick documenté non
  tenté (exige un vrai acheteur/vendeur façon M3b ; invariant vert sans).
- DLL Godot re-buildée post-M3h ET post-M3i. Tags reverse : pre-m3 … pre-m3i.

## PROCHAIN PAS (au choix du joueur)
1. **EXPORT scps.exe** — l'UI unifiée + TOUT le chantier monnaie jamais touchés en
   vrai : le plus gros backlog de test manuel du projet.
2. **UI DE LA MONNAIE** — réserve/dette/banqueroute/débase invisibles face joueur ;
   CMD_BANKRUPTCY et le curseur débase n'ont pas de bouton.
3. **Hégémon affaibli** — signal non-monétaire récurrent, jamais diagnostiqué
   (candidat : cartes d'influence, mémoire scps-ref-4x-ai-thesis).

---

# (archive) SYNTHÈSE 2026-07-14 après-midi

## ÉTAT COURANT
- Branche `claude/vibrant-euler-1tgfp3`, **tree propre**, tout committé jusqu'à `9035a11`.
- **SAVE_VERSION 85** (BuildOrder.prov). `make test` : **39 verts / 0 rouge** (seul
  intertrade_demo en BUILD ÉCHEC = pré-existant Windows setenv). Golden : IDENTIQUE
  (aucune re-baseline aujourd'hui). Determinism stable.
- CLAUDE.md **99 lignes** (doctrines PROVINCE + UI en tête) · TROUVAILLES ~200 lignes.

## CE QUI VIENT DE SORTIR (2026-07-14, ~15 commits)
1. **Doctrine PROVINCE gravée + codée** : mémoires persistantes + CLAUDE.md compressé
   2133→99 ; tous les verbes éco joueur au PID direct (v85, `de25550`), reader
   `scps_province_alloc`, fiche province = les 2 raws de LA tuile.
2. **UI unifiée — squelette + peau** : fiche province (classes sans barres, logements/
   services, hover biome+habitabilité, bâti seul, croissance /mois), menu construction
   en CARTES (effet·ressources·Prochain palier), arbre tech Civ 6 (3 couloirs × tiers,
   scroll latéral, flavor popup seul), panneau armée (onglets Composition·Combat,
   combat temps réel + résultat), journal persistant du rail droit, palette VKit →
   parchemin (a7c9945), PORTS STRUCTURELS armée (`97f7d4a`) + construction (`03a7080`)
   + chrome tech. Fix critique `main.gd` (`eefd1c4`) : le thème de fenêtre n'écrase
   plus ParchTheme des panneaux v2 (piège invisible aux probes).
3. **Textes** : sphère/espèce retirés face joueur ; taux /mois réels partout ;
   croissance démo /mois (empire + province). Impôt province corrigé (per-capita
   mensuel, plus 113k/an).
4. **« À la tonne »** (`f6e55e2`) : intrants d'or des recettes ÷4 (joaillerie 0.2,
   parurier 0.25) — golden inchangé (la joaillerie éclot après la fenêtre 12 ans).
5. **Concept MONNAIE v4** (docs/MONNAIE_CONCEPT.md, NON implémenté — « discutons ») :
   frappe = seule création, NEUTRE EN VALEUR (1:1 au prix du marché, Gresham retiré),
   redevance minière en nature, usages physiques d'abord, M(t)=M(0)+frappe, crédit au
   noyau M3, prix par empire + contagion commerciale (M4 local non optionnel),
   M6 = centralisation fiscale + transport. Étapes M0-M6 avec gates.
6. **Bancs réparés** (`9035a11`) : agency_demo/ai_demo — introduits NON par S0 mais
   par d07fa3b (2 brutes strictes, la veille) — bisect prouvé ; fixtures adaptées,
   zéro assertion affaiblie.

## LEÇON DE PROCESS (à retenir)
Deux vagues moteur (d07fa3b ET de25550) ont validé golden/determinism/savetest SANS
`make test` complet → bancs rouges découverts avec un jour de retard. **Tout brief
d'agent moteur doit inclure `make test` complet dans ses gates.**

## RESTES
- Readers façade armée : `scps_corps_upkeep(id)` (entretien /mois), `scps_corps_doctrine`
  (bonus/malus au repos), pertes ventilées par corps.
- Verbes SOCIAUX encore région-grain (CMD_REPRESS/ASSIMILATE/PURGE) — transférer au
  patron S0.
- Arbre tech : routage des lignes de prérequis (spaghetti) ; intérieur encore VKit
  immediate-mode (port futur) ; oddité `unlocks` qui écho son propre nom.
- Bataille de choc vivante jamais capturée (probes n'ont produit que des sièges).
- Capture multi-corps jamais faite. Flash « ordre émis » sans fondu.
- **EXPORT scps.exe : pas fait** — toute la vague UI est committée et vérifiée par
  probes ; l'export est le pas naturel suivant (« au moment opportun »).
- MONNAIE : **M0+M1+M2 LIVRÉS** (soir du 14) — audit (cb1b506, M imprime ×360-×1280/250 ans),
  redevance MINT_ROYALTY 0.15 + réserve par pays (a9e3dab, SAVE v86), frappe neutre 1:1
  via enveloppe BUDGET_MINT (61091df), kill-switch PROUVÉ (MINT_ROYALTY=0 ⇒ golden pré-M1
  vert), sweep apparié sain (±5 pts, IPM 0.96-1.05, réserve an-250 ni nulle ni explosive).
  M3 (conservation — QUI VEND + crédit) / M4 (prix locaux) / M5-M6 : décisions séparées.
- Aussi livrés le 14 après-midi : tri scps_faith SUPPRIMÉ (religion survit) · wilds =
  cités-états indépendantes (capitale fantôme corrigée + défense réelle) · purge grain
  région (sociaux/esclavage/pop_transfer → pid) · entretien VISIBLE (cartes + friche).

## MONNAIE — état au soir du 14 (M3b-v2.1 LIVRÉ, golden VERT)
- M3a (fixes+instrument, v87) · ÉTALON BIMÉTALLIQUE v5 (parité or 8/t · cuivre 2.6/t,
  042f4cc) · M3b-v2 circuit d'État+prix libres (815ee1a, v88 : price_level[pays],
  caisse achète/revend, IPM neutralisé, exonération vitale) · M3b-v2.1 dispatch item 5
  COMPLET (6 familles, table du joueur : entretien 33/33/33 · péages→bourgeois ·
  conseil→élites · encadrement→clé · militaire→laborers · manuf-chantiers→gages ·
  events→classes de la province).
- MESURES (3 graines × 250 ans) : conso-destruction ≈ 0 (trou noir FERMÉ) · Laborer
  53/49/59 (bande atteinte ~an 150, convergence monotone, zéro collapse) · HÉGÉMON
  MORTEL RESTAURÉ par le dispatch (les classes pauvres rendaient le monde docile) ·
  friche saine · colonisation vivante.
- RESTE MAJEUR (documenté TROUVAILLES) : la **VA RÉSIDUELLE (47-105k/an) n'a pas
  fondu** — l'hypothèse « caisse→price_level » n'était pas le canal, c'est le revenu
  direct qui a agi ; la dérive de M reste 15-30k/an (vs 160-290k pré-M3). Le banc
  invariant M(t)=M(0)+frappe (M3c) ne passera PAS tant que la VA résiduelle vit.
  + gains d'événements (créations), FX_AUDIT/FX_BUILD partiels, crédit (M3c).

## MONNAIE — CHANTIER COMPLET au 15/07 matin (M0→M3e + M4-IP, save v90)
Arc complet en ~24 h : audit (M imprimait ×360-1280) → redevance+réserve → frappe →
étalon bimétallique (parité or 16/t · cuivre 5.2/t, v5+M3e) → circuit d'État à prix
libres (price_level≤1 par pays) → dispatch complet des dépenses (table joueur) →
crédit réel (péréquation→classes→cités-états, passif ventilé, rachats Fugger) →
initiative privée (colonisation du peuple, investissement bourgeois) → dette
soutenable (plafond 300 %, taux 2-5 % au levier, banqueroute −75 % scar) → re-
liquéfaction (royalty/share 0.35, FRAPPE LIBRE = le levier de volume, commerce ×5,
démonétisation des WILDS, fix invariant = l'écrasement de wealth à la re-fondation
coloniale, jusqu'à 260k détruits en silence par province).
MESURES FINALES (apparié 520d1cf vs HEAD, 3 graines×3×250) : invariant VERT 9/9 ·
pop +16.2 % moy (V1 : −21.9 %) · colonisation totale +23 % · minorité au plafond
(10-25 %) · dette/revenu respire · IPM 0.85-0.90 SANS inflation (price_level≤1) ·
banqueroutes vivantes (44-207/sim) · taux 3.0-4.4 %.
RESTES MONNAIE : conversion des sites M0 restants pour l'invariant SERRÉ (missions,
tributs, gains d'events, pillage-stock — chacun ~VA-sized) · convergence prix-métal→
parité (les achats mint ne poussent pas demand[]) · M6 centralisation+transport ·
**UI DE LA MONNAIE : rien n'est visible face joueur** (réserve/dette/banqueroute/
frappe libre — CMD_BANKRUPTCY n'a pas de bouton) · hégémon affaibli récurrent
(signal non-monétaire, à diagnostiquer).

## PROCHAIN PAS ATTENDU
Proposer : EXPORT scps.exe (l'UI unifiée + TOUTE la monnaie jamais touchées en vrai
— le plus gros backlog de test manuel du projet), OU l'UI de la monnaie (rendre
visible dette/réserve/banqueroute), OU la vague invariant-serré (sites M0 restants).

## MISE À JOUR 2026-07-15 — M3h LA DÉBASE (calibré-livré, v93)

L'étage 2 de l'échelle du désespoir est CÂBLÉ : emprunter (M3c/M3d) → DÉBASER →
banqueroute-saisie (M3g). Sur-frappe au-delà de la parité (value=parité×(1+débase),
DEBASE_MAX=1), cash réel compté FX_MINT ; le PRIX passe par les ENTRÉES du moteur :
K_inst rongé à la capitale (rémanence debase_kdrain, décrue lente), rot des
Marchands (faction_capture_add, nouvel écrivain continu du g_capture), tolérance
fiscale ↓ ∝ déficit K (câblage AJOUTÉ — K ne pilotait AUCUN canal fiscal avant).
Curseur joueur BUDGET_DEBASE (verbe générique + binding, panneau GDScript = Restes) ;
politique IA déterministe : streak≥2 au plafond → débase progressive → banqueroute
forcée à streak 5, jamais sous cicatrice. Kill-switch DEBASE_MAX=0 prouvé
byte-identique pré-M3h. Golden re-baseliné (1 graine/5), test 38/0/1, determinism +
deep STABLES, savetest v93 A==B, fuzz 8/8.
ÉCART DOCUMENTÉ : invariant M3c 8/9 — graine 110 (le monde le plus petit du sweep)
breach 387-460 % vs seuil 370 % (pre : pic 299 %) — AUCUN canal M3h non compté
(diagnostic complet en TROUVAILLES), c'est le bruit « autres » pré-existant re-tiré
par la bifurcation ; précédent M3d appliqué (documenté, seuil non élargi).
TROUVÉ EN CHASSE (pré-existants, candidats M3i) : spéculation IA jamais convertie
(M0 §1.6, création lente garantie) · péages région-grain PARQUÉS sur des provinces-
porteuses vides (~250k/province à l'an 250, conservé mais hors circulation).

## PROCHAIN PAS ATTENDU
M3i (l'invariant qui redescend) : convertir spéculation IA + parking des péages +
saisie-monétisation M3g — OU l'UI de la monnaie (dette/réserve/banqueroute/débase
visibles), OU l'EXPORT scps.exe (test manuel du plus gros backlog).
