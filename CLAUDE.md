# SCPS — Codebase Instructions

**SCPS** est un jeu de civilisation en temps réel déterministe, codé en C99 (`scps/`), avec façade Godot 4 (`godot/project`). Le déterminisme du moteur est vérifié par des runs appariés et des bancs de sauvegarde. Le journal UI des ordres ne constitue pas, à lui seul, un fichier de replay complet : ne pas présenter le rejeu intégral d'une partie joueur comme acquis sans test dédié. Membrane : le front de jeu passe par les lecteurs de façade ; les outils développeur F10 constituent une surface explicite d'édition des coefficients.

## État vérifié — 2026-09-05

- Dernière suite complète après correction paix/siège : 50/51, timeout API à 420 s ; relance isolée à vérifier dans `runs/selection-2026-09-05/api-isolated.log`. Ne pas confondre avec les 51/51 de la passe cadence précédente.

- Les corrections et leurs limites sont dans `docs/RAPPORT_CORRECTIONS_VERBES_2026-09-05.md` et `docs/RAPPORT_FINITION_2026-09-05.md` ; besoins de finition et contradictions : `docs/BESOINS_FINITION_2026-09-05.md`.
- Export courant : `packaging/windows/dist_godot/session-20260905-paix/`. Format de sauvegarde 111, sans nouveau bump pour ces corrections. Fondation contrôlée sur Temple/Cathédrale provincial ; SCPS_MODS strict et atomique par ligne. Runner : 51 bancs, 7 smoke.
- Parcours réel militaire : sélection sur silhouette, fond clair local du panneau, vignette réservée aux batailles. Arrivée en paix = repos ; guerre exigée pour siège et butin mensuel, aperçu raccordé à la même règle. Campagnes 47/47, cadence 19/19, picking/mouvement 15/15 ; preuve avant/après sur vrai écran dans `docs/RAPPORT_SELECTION_PAIX_2026-09-05.md`. Fermeture du rendu complet : ressources graphiques non libérées encore signalées, ne pas déclarer le viewer exempt de diagnostics.
- Carte militaire : soldat individuel 2D, position liée à la progression quotidienne, aperçu avant ordre et segment engagé `[loc,next]` après ordre. Pas de route complète mémorisée ni de marche articulée. Cadence/lecteur 19/19, Godot mouvement 8/8, sauvegardes siège/bataille 20/20 chacune. Déterminisme 5×12 stable mais golden historique divergent après cadence quotidienne ; aucune re-baseline masquée.
- Parcours : fiche relation avec progression de route (priorité aux chantiers), feedback 34/34 et Godot vert. Trois paires commerciales sur un an : échanges effectifs sur 9/11, aucun avec la cible choisie sur 7 ; rendement culturel ≠ valeur échangée. Rapport `docs/RAPPORT_PARCOURS_2026-09-05.md`.
- Cadence militaire : `sim_campaign_day` résout désormais marche/bataille chaque jour et appelle l'interception navale avec `dt=1`; les pertes et le butin restent mensuels. `campaign_tick` garde des écarts à traiter autour des compteurs de déroute/ralliement et des contacts détectés dans un lot. Preuves et golden attendu après changement : `docs/RAPPORT_CADENCE_2026-09-05.md`.
- Marine hors combat : `NAVY_COMBAT_ON=0` validé par `runs/cadence-2026-09-05/navy-off.log` (40/40) ; le banc doit rester explicite sur ce mode et ne vaut pas preuve du combat naval actif.
- Campagnes : pilote public API, trois graines × témoin/production × cinq ans. Résultats contrastés, aucun tunable changé ; ne pas présenter ce pilote comme trois stratégies validées. Route acceptée = STARTED, recrutement/levée enfilés = ordre transmis. Voir `docs/RAPPORT_CAMPAGNES_2026-09-05.md` (feedback 25/25, Godot et export testés).
- Objectif encore ouvert : campagnes intéressantes et stratégies viables, progression jusqu'aux fins, confort et performances mesurés. Des bancs verts ne démontrent pas que le jeu est terminé.
- Pour les agents de cette session Codex, l'utilisateur a demandé Luna comme exécutants et une revue par l'orchestrateur. La hiérarchie Claude Code ci-dessous ne remplace pas cette instruction de session.
- Progression : seule la branche du Sol est implémentée (`DESS_BRANCH_COUNT=1`), avec deux voies alternatives. Les six autres branches du document de conception sont proposées/reportées, pas jouables. Le sceau revalide maintenant la condition réelle avant récompense ; `ready` est un cache mensuel, les preuves historiques du pivot restent des latches. Voir `docs/RAPPORT_PROGRESSION_2026-09-05.md`.
- Merveille : événement de fondation et paliers lisent `endgame_wonder_metab_count` avec `TechState` (contacts profonds compris). Une apocalypse déjà latchée empêche le verdict Ascension et l'effacement de l'empire. L'aide décrit les trois paliers ; ne pas réintroduire arbre complet/assimilation totale comme exigences retirées du verdict.

## Hiérarchie multi-agents (Claude Code)

**Opus** orchestrateur (plan, review, décisions). **Sonnet** implémenteurs + advisor. **Haiku** tâches mécaniques. Chaque brief d'agent inclut un **digest de mission** (doc-scps) + la consigne **TROUVAILLES.md** (handoff structuré : Découvertes · Pièges · Restes — ce qui a coûté cher à trouver, pas ce qui a été écrit).

---

## Principes de collaboration (non négociables)

- **Demander, ne pas supposer.** Si quelque chose n'est pas clair, demander AVANT
  d'écrire une seule ligne. Jamais de supposition silencieuse sur l'intention,
  l'architecture ou les exigences.
- **La solution la plus simple d'abord.** Toujours implémenter la chose la plus simple
  qui puisse marcher. Pas d'abstraction ni de flexibilité non explicitement demandée.
- **Ne pas toucher au code hors sujet.** Si un fichier ou une fonction n'est pas
  directement concerné par la tâche en cours, ne pas le modifier — même si on pense
  pouvoir l'améliorer.
- **Signaler l'incertitude explicitement.** En cas de doute sur une approche ou un
  détail technique, le dire avant de continuer. La confiance sans certitude fait plus
  de dégâts qu'un manque admis.
- **Ouvert aux meilleures idées.** Ne pas hésiter à proposer une meilleure façon de
  faire, ou une qui a un impact durable plutôt qu'un correctif tactique.

## LA PROVINCE EST LA SEULE RÉALITÉ ÉCONOMIQUE (non négociable)

- La **PROVINCE** (tuile) possède : pop/strates/groupes, ≤ 2 raws (son tirage), bâtiments, allocation, production/consommation, prix projeté, culture locale — mais **le TRÉSOR et les STOCKS sont NATIONAUX** (un par empire, aucun plafond ; décision joueur 2026-09-03, docs/DESIGN_TRESOR_NATIONAL.md). TOUT verbe joueur et TOUT reader façade est au grain PROVINCE (pid), sauf l'or et la matière d'État qui sont au grain PAYS.
- La **RÉGION** n'est qu'un AGRÉGAT politique/UI (`region[]` est une VUE reconstruite chaque clôture). AUCUN nouveau verbe/reader au grain région ; JAMAIS l'indirection `econ_region_rep_province` dans un chemin joueur. Si un chemin existant est région-grain : le TRANSFÉRER sur province (même au prix d'un bump de save), pas le contourner.
- **UI** : une fiche province ne montre que SES champs (≤ 2 raws) ; les agrégats multi-tuiles vivent dans des vues NOMMÉES (onglet Région, pays), jamais mélangés.

## Doctrine d'interface (non négociable)

- **TOPBAR** = NATIONALE · **BARRE DROITE** = menu, raccourcis/monde · **MENU GAUCHE** = CONTEXTUEL (liste de menus → sous-onglets → détails, TOUJOURS).
- Toute info à ≤ 3 clics ; l'info générale à l'œil, le DÉTAIL en hover.
- Toute valeur : **PAR MOIS** (croissance démo incluse), VALEUR RÉELLE, jamais le calcul, jamais d'annuel.
- Fiche province = le BÂTI seul (chips icône + [−][+], nom en hover) ; le **MENU CONSTRUCTION** = la vérité de TOUT (cartes : effet · entretien/mois · ressources · « Prochain palier [X] »).
- Cultures : tout le monde est HUMAIN — jamais « sphère »/« espèce » face joueur.

---

## Build & vérification

- `make test` : runner des bancs auto-vérifiants, précédé des gates membrane/langue/écritures régionales ; liste autoritative `tools/run_tests.sh:BENCHES_FULL` (49 à la livraison verbes). `make smoke` : 7 bancs rapides. `make full-test` ajoute déterminisme, golden et exécution sanitizer sur 20 ans.
- `make determinism` : deux runs appariés, 5 graines × 12 ans ; `make golden` compare à la référence enregistrée. `make determinism-deep` : runs appariés à 200 ans (endgame/cataclysme/crédit), à exécuter pour prouver le contrat long.
- `make chronicle && ./chronicle <seed> <sims> <years>` : balayage headless ; télémétrie = preuve d'équilibre.
- `make asan && ./chronicle_asan` : ASan+UBSan muets.
- `scps_viewer --savetest` : compare un digest partiel après continuation et save→reload→continuation ; ni replay des actions joueur, ni comparaison de tout l'état. `--fuzztest` : edge cases.
- `make golden` : vérif non-régression (golden_hashes.txt) ; re-baseline = décision joueur documentée.
- `make lang-check` : littéraux face-joueur vs base.
- `make scps` : console viewer (SDL-free) · `scons -C godot` : DLL Godot.

---

## Contrats actifs (l'historique : git log + TROUVAILLES.md + AUDIT.md)

- **SAVE_VERSION** : bump si sizeof(struct sérialisée) change ; `save_sane` revalide TOUT ; <version refusé.
- **Golden hash** : gate non-régression ; 5 graines × 12 ans identiques à la livraison verbes. Les anciennes mentions de KO Windows ne remplacent pas les résultats actuels (49 bancs validés, API relancée après timeout).
- **Tunables registre J** : 627 clés à la livraison verbes. `SCPS_TUNE=X=Y`, F10 validation/reset/copie ; phase d'application distincte de la valeur affichée. Cinq clés historiques inactives, deux diagnostics. Les seuils TIER se rafraîchissent via la révision du registre. Empreinte des valeurs effectives indépendante du texte d'affichage ; anciens profils de sauvegarde signalés, pas restaurés automatiquement. F4 recharge scps_lang.txt.
- **Modtools 3 canaux** : `SCPS_MODS` fichier (éco/tech/unités) · `scps_lang.txt` chaînes · gen_content.py codegen.
- **Membrane stricte** : jamais un flottant moteur face joueur ; MOTS + coords tangibles ; effets via ENTRÉES moteur, jamais bonus plat.
- **Déterminisme** : pas de Date.now/rand hors xs32 ; objectif de continuation identique après save/reload. Le savetest actuel ne compare qu'un digest partiel ; les métadonnées time/nonce excluent la comparaison brute des fichiers.
- **Pool national P1** : stocks empire-wide, main-d'œuvre locale, prix NATIONAL par empire (projeté sur les provinces). Allocation joueur override : CMD_* + onglet.
- **Province grain** : les commandes CMD_* sont revalidées au drain. Les actions religieuses et les taux de rachat sont encore des mutations directes, à inclure explicitement dans toute preuve de replay. La granularité province reste l'exigence pour les verbes économiques ; vérifier les exceptions existantes au lieu de les déclarer migrées. Build legal mirror : `scps_build_legal_ex` ↔ `agency_build_acct`.
- **Accumulateurs EMOB/COLC/TXYR/RVLT/ITRD** : inter-ticks ⇒ sérialisés ; savetest les prend.
- **Religion** : groupe-grain · cap ⌈N/2⌉ · fondation Temple T2+ exigée par le design et contrôlée dans `scps_religion_found` via `scps_religion_founding_ready` (bâti provincial possédé). La façade a des lecteurs ET des mutations ; propriété et éligibilité de schisme sont contrôlées. Ralliement au plafond distinct de la création d'une nouvelle racine.
- **Héritage + accès tech** : métabolisation (déverrouille/boost/remise) + 12 rungs + combos paires + apex T5 + coût √N · UI Medusa.
- **IA AUTO** : allocation demande-driven ; override = levier joueur ; diplo via value SUBJECTIVE ; support révoltes.
- **Worldgen** : 8 archétypes · falaises maritimes · rivières emergent (mouth-up) · lacs priority-flood · biomes pente+alluvion.
- **Endgame §27** : 5 fins + Merveille · entropie += charge faustienne · repli CHAUD an-240.
- **Front Godot** : scps_api façade · GDExtension binding · parchemin shader + urbaniste procédural · sons réels + ui_click.
- **Viewer console** : partagé sim+save avec Godot · --savetest/--fuzztest · SDL-free.

---

## Disciplines non négociables

- **La membrane** : `viewer.c` n'inclut jamais `scps_core.h` et ne lit aucun flottant SCPS — des MOTS (readout) et des nombres tangibles seulement.
- **On lit des coordonnées, on n'assigne jamais de modificateur** : un effet passe par les entrées du moteur (K, P, H…), jamais par un bonus plat.
- **TROUVAILLES.md — la mémoire des agents** : tout agent APPEND en fin de mission (Découvertes · Pièges · Restes) ; le successeur LIT ce fichier avant de fouiller.
- **SYNTHESE_SESSION.md** : handoff roulant (état courant · ce qui vient de sortir · ce qui RESTE · prochain pas) ; rafraîchir aux jalons et avant compaction.

---

## Langue

- **STR_* obligatoire** : tout texte face-joueur naît en `scps/strings_ids.h` (FR) + `scps/strings_en.h` (EN) ; compilation vérifie la paire.
- **Surcharge** : `scps_lang.txt` à côté du binaire remplace par ID (display-only ; F4 recharge à chaud).
- **Console** : `chronicle.c` / `econ_scan.c` / `batch.c` + tout `printf` / commentaires = FRANÇAIS définitivement (outillage ingénieur).
- **Cliquet** : `make lang-check` échoue si littéraux > base (scps/lang_baseline.txt) ; reflux progressif.

---

## Sauvegarde

- Format versionné (`SAVE_VERSION`), sections taguées, ChaCha20 (obfuscation, pas un secret) + empreinte FNV du clair.
- Toute valeur désérialisée qui borne une boucle ou indexe un tableau **se revalide** au chargement (`save_sane`) — refus net.
- Changer la taille d'une struct sérialisée ⇒ bump `SAVE_VERSION` (« ère antérieure »).
