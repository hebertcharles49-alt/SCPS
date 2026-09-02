# BRIEF vague P3-IA — l'IA joue l'influence et les doctrines · le chronicle mesure

Préparé 2026-09-02 (« prépare le chronicle et l'ia pour ces modifications »).
SE TIRE APRÈS le merge de la vague P2 (moteur doctrines + façade) — dépend de
`scps_doctrines.{c,h}` et des extensions `scps_influence`.

## 1. L'IA — mêmes règles, choix par score (design §4.6, révisions fermes)

1. **Génération d'influence pour TOUS les pays vivants** : le gate joueur ne
   reste que sur la DÉPENSE diplomatique (les cadences IA propres — `AI_*` —
   restent la loi de l'IA en diplomatie ; symétrie diplo éventuelle = décision
   joueur ultérieure, documentée au site). L'assiette suit le courant adopté,
   comme pour le joueur.
2. **Slots IA** : 1 + âges ADVENUS (cap 6) — l'engagement (`CMD_AGE_ENGAGE`)
   est un verbe joueur, l'IA n'a pas à s'engager (documenté).
3. **Adoption PAR SCORE sur l'état réel** (cadence `AI_DOCT_CHECK_MONTHS` 12,
   à la clôture d'année, déterministe — score desc puis id asc) :
   | Doctrine | Signal (état réel du pays) |
   |---|---|
   | Colonisation | capitale côtière · chantiers/colonies actifs · zones vivables adjacentes |
   | Commerce | routes ouvertes nombreuses · pactes commerciaux |
   | Mercantilisme | stocks/coussin hauts · peu de routes · Centres tenus |
   | Offense | guerres déclarées récentes · rancune sortante haute |
   | Défense | guerres subies · voisins plus forts (ratio `diplo_mil_power`) |
   | Vassaux | ≥ 1 vassal tenu |
   | Diplomatie | alliés/pactes nombreux · opinion moyenne haute |
   | Peuple | âmes étrangères hébergées · pactes migratoires |
   | Connaissances | contacts culturels profonds · héritages en digestion |
   | Production | manufactures nombreuses · brutes riches |
   | Infrastructure | bâti dense · vétusté haute |
   | Technologie | bibliothèques · savoir/médiane haut |
   | Faustien | techs ⚠ déjà prises ET `FAUST_BRECHE_CAUTION` non franchi (le garde-fou existant) |
   | Aristocratie / Bourgeoisie / Populaire / Divin (courants) | la classe (ou la foi bâtie) DOMINANTE en part relative — un seul courant, le meilleur |
4. **Achat d'idées** : quand `influence ≥ coût × AI_DOCT_RESERVE (1.5)`
   (coussin — l'IA garde de quoi vivre), la prochaine idée de la doctrine
   adoptée au meilleur score. Entretien/suspension : mêmes règles que le
   joueur (les dernières adoptées se suspendent).
5. **Effets** : `doctrine_key_mult(cid, …)` est déjà par-pays — les sites
   lisent le cid, rien à re-câbler ; les idées `wired=false` restent inertes
   pour l'IA aussi (parité).
6. **HORS PÉRIMÈTRE** : Desseins IA (P4, biais `ai_province_value`) ; verbes
   de doctrine (vague dédiée) ; dépense diplo IA en influence.

## 2. Le chronicle — la télémétrie qui prouve l'équilibre

1. **Colonnes périodiques par pays** : influence (stock, gain/mois) ·
   doctrines actives (compte) · idées possédées · suspensions du mois.
2. **Bilan de fin de sim** : distribution mondiale des doctrines (compte par
   doctrine, courants inclus) · influence médiane/max · entretien payé cumulé
   vs généré · suspensions cumulées · le TOP-3 doctrines par pays survivant.
3. **Vérification « par score »** (le juge du design) : % des pays côtiers
   ayant adopté Colonisation, % des suzerains ayant Vassaux, % des
   belligérants ayant Offense/Défense — imprimés au bilan (des corrélations
   franches = l'IA choisit sur son état, pas au hasard).
4. Console = FRANÇAIS (outillage ingénieur), printf directs, aucun STR_*.

## 3. Gates & mesure

- **Kill-switch** `AI_DOCT` (1.0 ; 0 = l'IA n'adopte jamais) ⇒ hash
  byte-identique au golden re-baseliné — LA preuve avant toute mesure.
- **Re-baseline golden** (UNE fois, documenté avant/après) : l'adoption IA
  change les trajectoires — c'est le but.
- full-test (bancs IA recalibrés FIXTURES seules si besoin) · determinism ·
  savetest (l'état IA est le même état sérialisé) · lang-check 127.
- **Sweep apparié 3×3** (préparé par l'orchestrateur, LANCÉ PAR LE JOUEUR —
  règle ferme) : `AI_DOCT=0` vs `1`, 3 graines × 3 répétitions, mesures an
  120/180 : pop/prix/guerres/trésors + distribution des doctrines + les
  corrélations §2.3. Plafond 30-40 sims ; GIGA sur demande explicite
  seulement.

## 4. Tunables neufs

`AI_DOCT` 1.0 · `AI_DOCT_CHECK_MONTHS` 12 · `AI_DOCT_RESERVE` 1.5 — rien
d'autre (les coûts/entretien sont les tunables communs existants).
