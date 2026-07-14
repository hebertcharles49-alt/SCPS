# SYNTHÈSE DE SESSION — handoff roulant (2026-07-14 après-midi)

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
- MONNAIE : attend un « fais-le » explicite du joueur (M0 audit = premier pas sans risque).

## PROCHAIN PAS ATTENDU
Proposer : export pour tester en vrai, OU readers armée + verbes sociaux pid, OU M0 (audit monétaire, lecture seule).
