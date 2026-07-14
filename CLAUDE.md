# SCPS — Codebase Instructions

**SCPS** est un jeu de civilisation en temps réel déterministe, codé en C99 (`scps/`), avec façade Godot 4 (`godot/project`). Moteur pur : une graine + un journal de commandes rejouent le monde byte-identique. Membrane stricte : le front Godot ne lit que des MOTS et des coordonnées, jamais un flottant moteur.

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

- La **PROVINCE** (tuile) possède tout : pop/strates, ≤ 2 raws (son tirage), bâtiments, allocation, prix, stock, culture locale. TOUT verbe joueur et TOUT reader façade est au grain PROVINCE (pid).
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

- `make test` : 40 bancs auto-vérifiant (core_demo + modules) ; `make smoke` (8 rapides) · `make full-test` (tout).
- `make determinism` : 12 ans byte-identique vs golden · `make determinism-deep` : 200 ans (endgame/cataclysme/crédit).
- `make chronicle && ./chronicle <seed> <sims> <years>` : balayage headless ; télémétrie = preuve d'équilibre.
- `make asan && ./chronicle_asan` : ASan+UBSan muets.
- `scps_viewer --savetest` : save→reload→rejoue A==B byte-identique · `--fuzztest` : edge cases.
- `make golden` : vérif non-régression (golden_hashes.txt) ; re-baseline = décision joueur documentée.
- `make lang-check` : littéraux face-joueur vs base.
- `make scps` : console viewer (SDL-free) · `scons -C godot` : DLL Godot.

---

## Contrats actifs (l'historique : git log + TROUVAILLES.md + AUDIT.md)

- **SAVE_VERSION** : bump si sizeof(struct sérialisée) change ; `save_sane` revalide TOUT ; <version refusé.
- **Golden hash** : gate non-régression ; 3 KO Windows pré-existants (intertrade setenv, campaign/warhost stack).
- **Tunables registre J** : override `SCPS_TUNE=X=Y` · F10 live panel · F4 recharge scps_lang.txt.
- **Modtools 3 canaux** : `SCPS_MODS` fichier (éco/tech/unités) · `scps_lang.txt` chaînes · gen_content.py codegen.
- **Membrane stricte** : jamais un flottant moteur face joueur ; MOTS + coords tangibles ; effets via ENTRÉES moteur, jamais bonus plat.
- **Déterminisme** : pas de Date.now/rand hors xs32 ; save/reload bit-identique.
- **Pool national P1** : stocks empire-wide, main-d'œuvre locale, prix NATIONAL par empire (projeté sur les provinces). Allocation joueur override : CMD_* + onglet.
- **Province grain** : verbes = journal CMD_* drainé point fixe, revalidés drain, golden-neutres. Build legal mirror : `scps_build_legal_ex` ↔ `agency_build_acct`.
- **Accumulateurs EMOB/COLC/TXYR/RVLT/ITRD** : inter-ticks ⇒ sérialisés ; savetest les prend.
- **Religion** : groupe-grain · cap ⌈N/2⌉ · fondation Temple T2+ · façade read-only.
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
