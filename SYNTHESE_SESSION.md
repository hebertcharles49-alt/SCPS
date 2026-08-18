# SYNTHÈSE SESSION — 2026-08-18 : vagues CLIMAT puis ÎLES committées

## Vague ÎLES (dernier commit)
- 3 étages : PLATEAU (fragment détaché, détroit dérivé de la taille, tangent à la côte,
  ~55 %/noyau, 6 essais anti-avalement ellipse-exacts) · MOYEN (chaînes 1-2 garanties,
  tête ×1.9, rejet vers l'océan) · HAUTURIER (pics fantômes francs : mer fantôme locale
  abaissée une fois le seuil passé).
- Protections structurelles : mers internes creusent AVANT l'union îlots (une Caspienne
  noyait une Sicile) ; force d'émergence PAR îlot (0.55 était codé en dur).
- Mesure 24 graines : 24/24 mondes ≥1 île, spectre écueil→22.8k cellules ; carte regardée
  (graine 219 : « l'Angleterre » provincée + satellite). Restes : poussières 1-2 cellules
  (cull à juger), fusion des noyaux multi-masses (« archipel » = 1 bloc) = chantier
  COMPACITÉ (aussi la clé des déserts continentaux).
- Appellation : trace genèse « N noyau(x) » (demandé) vs ligne « masses » (continents
  RÉELS mesurés + centroïdes pour pointer les probes).
- 2e recalibrage bancs du jour (5 bancs, agents, fixtures seules) — pièges frais dans
  TROUVAILLES.md : régions d'îlots SANS province active (rep=-1), micro-nation à grief nul.


## Ce qui vient de sortir (ce commit)
- **Vague CLIMAT** (critique worldgen du joueur) : itération climat→biomes provisoires→climat
  (recyclage forestier VIVANT via bio_prev), RAZ du fetch entre passes directionnelles,
  bonus fluvial relocalisé post-trace_rivers en retouche RIVERAINE (rayon 3, plancher
  0.58→0.45, boost×0.45), ZCIT ajoutée (gaussienne humide lat 0.06 σ0.09 ×0.22, subtrop
  0.52→0.56), histogramme des biomes instrumenté à chaque genèse.
  Mesures (100 graines) : famille sèche 23.4→30.5 %, marécages 11.6→4.3 %,
  jungle 0.00→1.81 % (96/100), désert 0.64 % (plafond GÉOMÉTRIQUE — archipels).
  Carte REGARDÉE (graines 9/205) : ceintures sèches lisibles, corridors riverains verts.
- **Curseur HUMIDITÉ à la création du monde** (décision joueur) : new_game_panel.gd —
  initialisé au TIRAGE de la graine (non touché = archétype intact), mot vivant
  aride/modéré/humide, T_NG_HUMIDITY (ui.csv). Le tuyau moteur existait déjà.
- **6 bancs recalibrés post-climat** (3 agents, fixtures SEULES, moteur intact) :
  statecraft (la fixture n'appelait jamais demography_tick — élite=0, canal grief mort
  depuis toujours), revolt (région 3→6, rep-province disparue), agency (graine 42→1,
  capitale à 1 hab + érosion M3h DEBASE_K_EROSION_RATE sans plancher), ai (graine 9→2),
  scps_api (graine 9→1 : l'archipel post-climat laminait le joueur ; + faux négatif
  structurel du delta de guerres agrégé → wt_at_war ciblé), endgame (C5 lisait le grain
  au grain RÉGION alors qu'econ_cold_refresh écrit PROVINCE — doctrine).
- Gates : 40/40 bancs · savetest 2/2 · déterminisme 5×12 ans stable · goldens re-baselinés.

## Restes consignés (non tranchés)
- Plafond géométrique du désert : compacité des archétypes « continents » (décision design).
- Démontage du bloc « IA navale frugale » inline de sim.c (routes maritimes dupliquées ai.c).
- Churn de conseil (COUNCIL_CLASS_SAT_W), drain servile (MANUMIT_INTEG), M-vague
  assèchement monétaire, fusion strata↔groups, Découvertes an 3-5, diplomatie/personnages.
- CI cloud GitHub Actions : workflow prêt, Actions à activer côté repo.

## Pièges frais (détail : TROUVAILLES.md)
- scons sous shell -l MSYS2 : exporter PROCESSOR_ARCHITECTURE=AMD64 (intelc.py KeyError).
- python-en-heredoc mange les \n — TOUJOURS Write un .py.
- « GCC » dans les processus = Gigabyte Control Center, pas le compilateur.
