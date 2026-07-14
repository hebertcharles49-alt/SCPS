# LA MONNAIE MÉTALLIQUE — concept & feuille de route

> Statut : **CONCEPT ACTÉ, non implémenté** (discussion joueur 2026-07-14).
> Le chantier ne démarre que sur ordre explicite ; ce document est le contrat.

## Vision

L'or-monnaie est le **dernier grand compteur abstrait** de SCPS (la matière est réelle
depuis P1/lot B, la pop est réelle, la doctrine dit « on lit des coordonnées »).
Ce chantier le rend **diégétique** : la monnaie est du métal frappé, créée par la
frappe SEULE, conservée, transférée — jamais inventée. L'inflation cesse d'être un
thermostat borné pour devenir un **comportement émergent** de la géologie et de la
politique monétaire, y compris l'inflation SÉCULAIRE et les chocs historiques
(Mansa Musa qui déstabilise les économies sur son passage ; l'Espagne qui croule
sous l'argent américain et exporte sa révolution des prix).

## Décisions actées (joueur, 2026-07-14)

1. **Frappe pilotée par un curseur** du MENU ÉCONOMIQUE : « part de la réserve
   frappée » (0-100 %), à côté des impôts/enveloppes. Régalien : la couronne frappe,
   la monnaie entre au trésor, la redistribution passe par les canaux EXISTANTS
   (gages, dépense publique) — pas de plomberie neuve de distribution.
2. **Le marché du luxe d'abord, le surplus en réserve** : l'or/cuivre extraits se
   vendent tant que la demande (joaillerie…) en absorbe ; l'invendu tombe en
   **RÉSERVE MÉTALLIQUE** par pays (un stock visible du menu éco). La frappe puise
   dans la réserve, jamais dans le marché. Couplage émergent voulu : monde prospère
   → le luxe absorbe le métal → réserve maigre → monnaie rare.
3. **1 or = 10 cuivre** (un seul canal monétaire, deux métaux pondérés).
4. **L'impôt per-capita NE CHANGE PAS** (forfait 0.06/0.15/0.27 × curseur ; le gain
   réel des classes reste la borne + le pilote d'évasion).
5. **Sans mine = troc pénalisant, ÉMERGENT** : aucune injection monétaire, aucun
   plancher artificiel. Importer du métal ou conquérir une mine devient rationnel.
6. **AUCUNE perte de monnaie** (pas d'usure, pas de puits) : les métaux sont
   stables, un drain serait dur à équilibrer et inintéressant. Conséquence ASSUMÉE :
   **l'inflation séculaire est un TRAIT** — la masse ne fait que croître sur 250 ans,
   comme le monde réel jusqu'à la monnaie papier.
7. **Pas de simulateur de trading** pour l'IA : des **réflexes monétaires** lisant
   des coordonnées réelles (cf. M5).
8. **Monnaie-objet en deux étages** : d'abord CONSERVÉE (scalaire pays, comptabilité
   fermée), ensuite LOCALISÉE (le coffre de la capitale, pillable).

## L'architecture cible (la chaîne complète)

```
extraction or/cuivre
   → marché du luxe (la demande achète d'abord)
      → SURPLUS → RÉSERVE MÉTALLIQUE du pays        [visible menu éco]
         → curseur « part de la réserve frappée »    [levier joueur ; défaut IA]
            → MONNAIE au trésor (or ×10, cuivre ×1)
               → dépense publique / gages / chantiers (canaux existants)
                  → circule : salaires ↔ marché ↔ impôts ↔ commerce   [TRANSFERTS seuls]
```

Invariant final (étage M3) : **Σ monnaie du monde = Σ monnaie frappée depuis l'an 0.**

## Points à suivre (étapes ordonnées — chaque étape a son gate)

### M0 — L'AUDIT de la création monétaire (lecture seule, 0 risque)
- [ ] Cartographier TOUS les sites qui CRÉENT de la monnaie ex nihilo aujourd'hui :
      `wealth += VA×part` (scps_econ.c:2915/3082 — chaque atelier est une planche à
      billets), récompenses de mission, events (+trésor), butin, crédit… Liste
      exhaustive fichier:ligne.
- [ ] Télémétrie chronicle « masse monétaire » : Σ(trésors + wealth des strates) et
      sa DÉRIVE par an — la mesure de référence AVANT tout changement.
- Gate : golden IDENTIQUE (pure lecture) ; le rapport chiffre « combien le monde
  imprime par an aujourd'hui ».

### M1 — LA RÉSERVE MÉTALLIQUE (le stock, pas encore la frappe)
- [ ] Le surplus invendu d'or/cuivre (post-demande de luxe) bascule en
      `reserve_metal` par pays (⚠ champ neuf sérialisé → **SAVE BUMP**).
- [ ] Menu économique : ligne « Réserve : X or · Y cuivre » (readers purs + UI).
- [ ] Le métal en réserve SORT du marché (plus vendable) — c'est le choix « le
      marché d'abord » qui borne l'accumulation.
- Gate : re-baseline golden documentée (les stocks bougent) ; sweep : la joaillerie
  vit toujours (le luxe est servi AVANT la réserve).

### M2 — LA FRAPPE (le levier)
- [ ] Curseur « part de la réserve frappée » (0-100 %) dans le menu éco (motif
      CMD_BUDGET_POLICY existant) ; défaut IA raisonnable (~10-25 %).
- [ ] Frappe mensuelle : `monnaie = (or×10 + cuivre) × part` — réserve décrémentée,
      trésor crédité (grain province de la capitale, doctrine).
- [ ] Télémétrie « frappe : X or/an · N empires frappeurs ».
- Gate : sweep apparié OFF/ON (satisfaction ±5 pts, hégémon mortel, pop stable) ;
  l'IPM ACTUEL reste clampé à cette étape (le déclampage attend M3).

### M3 — LA CONSERVATION (la révolution comptable — le gros œuvre, classe P1)
- [ ] Fermer la boucle des paiements : les salaires/profits/rente ne sont plus
      CRÉÉS depuis la VA — ils sont PAYÉS par les acheteurs via le marché
      (le solde du marché devient le canal des revenus). Chaque site de M0
      converti en TRANSFERT.
- [ ] Les events/butins/missions qui « donnaient » de l'or deviennent des
      transferts (depuis un trésor, un pillé, un marché) ou des dons de MÉTAL.
- [ ] **Banc invariant** : Σ monnaie mondiale == Σ frappée (à l'unité près),
      vérifié chaque année de sim — le garde-fou absolu du chantier.
- [ ] Sous-étapes séquencées (une famille de sites à la fois, sweep entre chaque) —
      le monde est BISTABLE (cf. POP_R_BASE), on avance au pas.
- Gate : invariant vert sur 250 ans × 5 graines ; satisfaction/pop dans les bandes ;
  re-baseline documentée.

### M4 — LE DÉCLAMPAGE (l'inflation émergente, séculaire incluse)
- [ ] Retirer les bornes IPM (0.85-1.35) et la mean-reversion : le niveau des prix
      devient `f(M, PIB)` — stable par construction (M borné par la géologie).
- [ ] **Design clé à trancher ici** : passer de l'IPM MONDIAL unique à des niveaux
      de prix PAR EMPIRE avec **contagion par le commerce** (les routes/Centres
      transmettent l'inflation) — c'est LÀ que vivent Mansa Musa et l'Espagne :
      l'empire qui croule sous l'or déstabilise ses PARTENAIRES, pas la planète
      uniformément.
- [ ] Mesurer l'inflation séculaire au sweep 250 ans (attendu : hausse lente des
      prix, chocs locaux aux conquêtes de mines) — c'est un TRAIT, pas un bug.
- Gate : aucun runaway numérique (le banc anti-NaN du crédit reste vert) ; les
  longs runs finissent ; le sweep raconte des histoires monétaires lisibles.

### M5 — LES RÉFLEXES MONÉTAIRES IA (pas un simulateur)
- [ ] *Fuite vers le métal* : prix hauts chez soi → l'IA thésaurise, frappe moins.
- [ ] *Débase de guerre* : trésor vide + guerre → frapper fort, assumer l'inflation
      (le dilemme historique).
- [ ] *Arbitrage* : acheter l'or bon marché d'un voisin pour le frapper chez soi
      (germe existant : le spéculateur intertrade).
- Gate : chaque réflexe lit des coordonnées réelles (IPM local, trésor, guerre) ;
  télémétrie par réflexe ; sweep.

### M6 — LE TRÉSOR LOCALISÉ (étage 2 : la monnaie-chose)
- [ ] Le trésor vit physiquement dans la CAPITALE (⚠ save bump) : le sac d'une
      capitale PILLE la monnaie réelle (fin du butin ex nihilo), le trésor de
      guerre se transporte/se capture en campagne.
- [ ] Thésaurisation/coffres régionaux optionnels (plus tard).
- Gate : le pillage conserve l'invariant M3 ; sweep guerre (le sac devient plus
  rentable — mesurer l'agressivité).

## Ce qu'on NE fait PAS (décisions fermées)
- Pas de perte/usure de monnaie (métaux stables ; drain inéquilibrable).
- Pas de dîme proportionnelle (le forfait per-capita reste l'impôt).
- Pas de bimétallisme à deux masses (un canal, or ×10).
- Pas de simulateur de trading IA (réflexes seulement).
- Pas de plancher pour l'empire sans mine (le troc pénalisant est voulu).

## Risques nommés
- **Bistabilité** : chaque étape M1-M4 re-baseline ; le sweep apparié est l'arbitre
  (jamais le premier chiffre venu).
- **M3 est le vrai mur** : convertir la création en circulation peut affamer les
  classes si la vitesse de circulation est mal réglée (la monnaie frappée doit
  ATTEINDRE les salaires assez vite) — d'où l'invariant + l'avance au pas.
- **M4 contagion** : des prix par empire multiplient l'état — vérifier le coût
  sim/save avant de s'engager (déclampage mondial simple en repli).
- L'inflation séculaire rend les VALEURS NOMINALES croissantes sur 250 ans —
  l'UI doit rester lisible (les prix montent, c'est raconté, pas cassé).
