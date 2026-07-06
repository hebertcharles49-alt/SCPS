# Synthèse de session — 2026-07-05/06 : la BOUCLE DE GAMEPLAY (membrane de décision · 21 dilemmes · décrets · Annales)

> Handoff. Branche `claude/vibrant-euler-1tgfp3`. **SAVE_VERSION 65** (v62 EVNT membrane ·
> v63 annales · v64 DCRE décrets · v65 TXYR étendue — l'instrument I0 de l'année en cours).
> Mission utilisateur : « Termine tout » — les 3 docs de design (boucle, registres
> d'événements v4 + lot 2, décisions-leviers, pack flavor tech) implémentés en hiérarchie
> multi-agents (orchestrateur Fable + implémenteurs sonnet parallèles sur fichiers disjoints).

---

## Ce qui a été livré (commits f5bd6c7 → aff0358 + la vague précédente 5d2d168 → bc51fca)

- **§1 La MEMBRANE DE DÉCISION** : EvOption 3-4 choix (label/blurb/effets/ai_chance/hook/
  flavor), cicatrices ScarKind à mémoire (delay par kind), cooldowns, file joueur pending[8],
  `resolve_choice` COMMUN IA/joueur (l'IA tire ai_chance, le joueur enfile CMD_EVENT_CHOICE),
  `d_treasury_mois` = fraction signée du revenu réel × IPM, titres gabarits « %s » → noms
  réels de provinces, **LE PARI** (gamble_eff/gamble_p — chaque dilemme a une option
  incertaine, résolue au rng d'état).
- **21 dilemmes** : Marbrive + Pont effondré (la crise phare + son chaînage), 6 W1
  (cloches · entrepôts fermés · deux cartes · eau noire · dernière décision · salve runique),
  16 lot 2 — §A tech-latch (6 : la tech découverte pose son dilemme moral, une fois par pays),
  §B culturels (2), §C religieux (3), §D chaînage de cicatrices (5 : chaque K consomme sa
  cicatrice mûrie). Sautés & documentés : B2/B3/B5/B6, C2/C3/C4 (helpers moteur absents).
- **§3 DÉCRETS** (`scps_decrees`, player-only ⇒ golden intact) : levée permanente · mécénat ·
  ambassades · politique de tribut — tous sur des leviers EXISTANTS, le coût EST la
  contrepartie. 4 différés documentés (aucun levier propre).
- **§4 ANNALES + §4bis** : frise cliquable, causalité affichée (la cicatrice pointe son
  dilemme d'origine), récap d'ÂGE au chip « Engager » (écran de chapitre), ÉPILOGUE
  (« Votre règne en une phrase »), 12 épithètes émergentes. 8 bannières thématiques sur les
  popups, conseillers-visages (« — {faction} » par option).
- **SAVETEST v65** : la dérive d'or post-reload venait de `g_flux[][]` (I0, année en cours)
  non sérialisé → TXYR étendue. Savetest 7/9/11/42 **byte-identique**.
- **Télémétrie chronicle** « dilemmes (lots 1-2) » : 666-1172 W1 · 19-43 culturels ·
  61 religieux · 133-353 chaînages/sim — les registres VIVENT.
- Bug latent pris : débordement de pile `events_text_clean` (texts[256] < besoins réels).

## Vérifs finales

37 bancs verts (3 KO Windows pré-existants : intertrade setenv, campaign/warhost stack) ·
events_demo 85/85 · scps_api_demo 131/131 · determinism STABLE · golden RE-BASELINÉ
(les dilemmes mordent < 12 ans) puis confirmé IDENTIQUE · fuzztest 7/7 · scons 0 warning ·
probes headless ANNALES-2/age/diplo OK · sweeps seed 9/11 200 ans SAINS (Laborer 75-77 %,
IPM 1.05-1.12, hégémon mortel).

## Pour la prochaine session

- **Latches tech silencieux en sweep** : les §A + salve exigent l'arbre profond (~15 %
  d'arbre/empire en 200 ans) — prouvés au banc, jamais vus en chronique. Si on veut les
  VOIR : sweep 400 ans ou seed à grand empire.
- **Volume W1** : ~3-6 dilemmes/an monde entier (IA auto-résout). Pour le JOUEUR le popup
  n'arrive que sur SES provinces — à jouer pour sentir si le rythme est bon ; sinon serrer
  les cooldowns/triggers de cloches (le plus bavard, 1172/sim).
- **Décrets différés** (centralisation, tolérance, creuset, isolationnisme) : exigent des
  leviers moteur neufs (setter de crédo, ouverture commerciale par pays…).
- **Événements lot 2 sautés** : B2/B3/B5/B6, C2/C3/C4 attendent credo_drift/ethos_drift/
  creuset_state/fracture-tracking.
- Env : `make scps` sans SDL ; scons : jonction godot/godot-cpp + PROCESSOR_ARCHITECTURE=
  AMD64 (cf. mémoire scps-build-windows).
