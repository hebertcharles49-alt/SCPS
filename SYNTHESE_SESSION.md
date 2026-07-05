# Synthèse de session — 2026-07-05 (ter) : mission éco (matière réelle · IA · calibrage Anno)

> Handoff. Branche `claude/vibrant-euler-1tgfp3`. **SAVE_VERSION 61** (COLC : le répit de
> colonisation g_colony_cd est un accumulateur inter-ticks — le savetest seed 11 l'a exigé).
> Mission utilisateur : « fix l'économie, ses chiffres et la logique de colonisation/
> construction de l'IA » + « stop aux chiffres aberrants (provinces à ~20 000 nourriture) »,
> référence design Anno 1800. Orchestration : hiérarchie CLAUDE.md (orchestrateur Fable +
> 3 agents d'audit + 4 implémenteurs sonnet, relance après une limite de session).

---

## Les trois découvertes structurantes (audits croisés)

1. **La matière était FANTÔME.** `region[]` est une VUE reconstruite à chaque clôture ;
   l'or passait par les provinces (`it_treasury`) mais la matière s'écrivait sur la vue →
   effacée ≤ 30 j. Le marché ne déplétait jamais (acheteur dupliqué), navy gratuite
   (or/bois/cuivre/ÉQUIPAGE), tribut de vassal évaporé, pillage = robinet d'or, marge
   d'import morte 11 mois/12. PROVINCE_MODEL.md le notait comme « raffinement futur » —
   c'était en réalité TOUT le canal physique hors-tick.
2. **Le « panneau B » n'existait pas.** Le §NF exclut le joueur (« il construit à la
   main ») mais aucun verbe ne posait de manufacture → le joueur était structurellement
   privé de logement manufacturier (plafonné ½·cap_pop) pendant que l'IA doublait.
3. **Les ~20 000 nourriture étaient RÉELS** (pas un bug d'affichage) : un ouvrier-grain
   nourrissait ~15 personnes (rendement 8/an vs bouche réelle ~0,53/hab/an) ; l'UI
   affichait en plus le cumul ANNUEL (per_day × 365).

## Livré (commits `3ed69d9` + `b0964fe` + docs)

- **Lot B — matière réelle** : helpers `econ_region_{stock,treasury,pop}_add` (porteuse →
  sœurs, delta réel rendu, repli fixture pour bancs synthétiques) ; ~20 sites convertis
  (intertrade/trade/navy/warhost/diplo/spéculateur/péage). On FACTURE le réel pris.
- **Lot A — I0 complet** : FX_BUILD / FX_REDEP / FX_CREDIT (le trou de ~1500 or/mois).
- **Lot C1 — panneau B** : `CMD_BUILD_MANUF` + façade + binding + UI « Bâtir une
  manufacture » (légalité grisée, flash). `scps_api_demo` 111/111.
- **Lots F1-F5 — IA** : colonisation cadencée par `w_expand` (AI_COLONY_TEMPO 3.0) et
  gelée en guerre ; grenier sur la région affamée ; gate chantier borné au pool commercial
  (compteur `nocap` dans EDI_DBG) ; anti-double-commande d'édifice (fenêtre 960 j fermée).
- **Lots E1-E7 — calibrage Anno** : vivrier **÷1.5** (÷2 mesuré/rejeté : seed 11 < 70 %),
  papier 209 / remède 404 (labor), statuaire gatée, readouts logement → `econ_prov_effcap`,
  `struct_deficit`→raw_boost (nomat Port −13 %), top_flow en tie-break, télémétrie
  colonisation (fondations/survie). Plafond E2 vérifié SAIN (fausse alerte : artefact
  d'agrégation des régions partagées).
- **Affichage** : flux province en « +N,N/j » (style Anno), fin du ×365.

## Vérifs (fin de mission)

`make test` 37/40 runnable verts (3 KO Windows pré-existants) · **golden RE-BASELINÉ**
(matière réelle + rendements mordent dès l'an-0 — documenté) · determinism STABLE ·
savetest **7/9/11/42 byte-identique** (après sérialisation COLC — seed 11 avait pris la
divergence) · fuzz-save 7/7 · GDExtension scons 0 warning · sweeps : Laborer 72-78 %,
zéro famine, pop en croissance, colonisation mesurée ~190 fondations/sim, IPM 1.05-1.19.
⚠ Accession 360 j désormais TARDIVE (an ~35-78 — la matière est réelle ; l'ancien an-2
se payait en matière fantôme) : point d'équilibrage ouvert, pas une régression.

## Pour la prochaine session

- **E3 stockeuse** toujours morte (l'IA ne bâtit jamais d'Entrepôt : gate has_halles +
  hub + trésor≥400 jamais réuni) — one-liner possible mais à MESURER (post-matière-réelle
  la spéculation a enfin un sens physique).
- **Accession 960 j** : le mur nomat baisse (E5) mais l'Académie/Citadelle restent rares —
  si voulu, passer la priorité raw_boost trio du tie-break au forcing mesuré.
- **Prix convergence intertrade** : nudge transitoire mort sous le prix national (commenté
  en place) — retirer ou re-router si le différentiel régional doit revivre.
- Doublons d'outillage assumés (avg_price chronicle/econ_scan, write_ppm dump/mapshot).
- Env : `make scps` sans SDL ni WIN=1 ; scons : jonction godot/godot-cpp +
  PROCESSOR_ARCHITECTURE=AMD64 (cf. mémoire scps-build-windows).
