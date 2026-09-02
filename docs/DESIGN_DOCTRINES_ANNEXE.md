# ANNEXE — Doctrines : catalogue simple (révision 2026-09-01, style EU4)

Révision sur retour joueur : **pas de gate, pas de prérequis, tu cliques t'as
le bonus** · noms courts (un mot/une action) · bonus en une ligne lisible ·
flavor ≤ 1 phrase · **coût fixe et scalable** (façon unités Stellaris) ·
l'IA choisit **par score** sur ses propres modificateurs (pays côtier →
Colonisation score haut, etc.), aucune restriction d'éthos.

## Coûts (influence) — révision 2026-09-02 : slots LIBRES, dépense linéarisée

Les 6 slots sont ouverts d'office (plus d'ouverture par âges). Tous les coûts
sont **linéarisés sur la pop de nobles** : × `é = assiette de génération
(SANS le mult du Conseil — anti-exploit) / INFLUENCE_BASE_REF (2.0)`,
plancher 0.25. Un empire 2× plus noble paie 2× plus cher ET gagne 2× plus
vite — le temps d'acquisition est constant, la liberté est ressentie.

- **Adopter une doctrine** : `(DOCT_BASE 50 + DOCT_STEP 25 × actives) × é`.
- **Acheter une idée** : `(IDEA_BASE 30 + IDEA_STEP 3 × idées possédées,
  toutes doctrines confondues) × é`. Abandonner libère le compte.
- **Entretien** : `DOCT_UPKEEP 1.0 × é /mois par doctrine active`.
- **Synergie de paire** : entretien fibonaccien 2/3/5/8… /mois (inchangé).
- **Complet = rien de spécial** ; la paire complète ouvre le sous-menu de
  synergie. **Abandon libre**, sans remboursement ni cicatrice.
- **Entretien en INFLUENCE, pas en couronnes** (révision joueur 2026-09-01) :
  `DOCT_UPKEEP (1.0) /mois par doctrine active`. Avec les synergies (2/3/5/8)
  et les émissaires, l'influence devient une vraie économie : les nobles la
  génèrent, les doctrines la consomment — d'où l'intérêt de faire GRANDIR sa
  noblesse, à double tranchant (plus de voix, mais plus de panier de luxe à
  servir et plus de rivalité aspirants/positions ; et le courant choisit
  l'assiette). Insolvable ce mois ⇒ les doctrines les plus récemment adoptées
  se suspendent CE mois (ordre déterministe), mults à 1.0.

Structure inchangée : 6 slots · 17 doctrines · idées achetées EN SÉQUENCE ·
Commerce↔Mercantilisme exclusifs · un seul courant politique sur quatre (le
courant re-siège l'assiette d'influence). **V** = l'idée débloque un verbe.

---

## 1. Offense
*Le fer d'abord.*

| # | Idée | Bonus | Levier moteur |
|---|---|---|---|
| 1 | Arsenaux | +25 % d'armes produites, rouille ÷2 | `ARMS_PER_LABORER` ×1.25 · `ARSENAL_DECAY` 0.99→0.995 |
| 2 | Discipline | +10 % de dégâts au combat | `BT_DMG_K` ×1.10 · `CTR_BITE` ×1.15 (côté attaquant) |
| 3 | **V** Ost permanent | Renfort automatique des armées (solde de guerre payée en paix) | `CMD_HOST_STANDING` · `campaign_refill_corps` inchangé |
| 4 | Butin | +30 % de butin de siège, +15 % au sac | `SIEGE_LOOT_FRAC` ×1.30 · `PILLAGE_INCOME_FRAC` ×1.15 |
| 5 | Prétextes | −40 % de coût de revendication, maturation −30 % | `FAB_CB_COST_YEARS` ×0.60 · `FAB_MATURE_DAYS` ×0.70 |
| 6 | Grande levée | +30 % de limite de force | `SOLDE_FL_PER_REG` ×1.30 · `SOLDE_OVER_K` ×0.80 (promotions de `#define`) |

## 2. Défense
*On ne gagne pas la guerre : on la fait perdre.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Remparts | +30 % de défense des places | `DEF_PER_H` ×1.30 |
| 2 | Magasins | +25 % de vivres de siège (15 mois au lieu de 12) | `SIEGE_FOOD_MONTHS_FULL` ×1.25 (promotion) |
| 3 | **V** Ban | Milice levée instantanément dans une province envahie | `CMD_LEVEE_BAN(pid)` |
| 4 | Corvées | −20 % de coût des fortifications | `BUILD_COST_MULT_FORT` ×0.80 |
| 5 | Terre brûlée | −40 % de butin pris par l'envahisseur | `SIEGE_LOOT_FRAC` ×0.60 · `PILLAGE_INCOME_FRAC` ×0.80 (côté victime) |
| 6 | Génie | Fortifications un palier de tech en avance | gate `tier−1`, famille fortifiée seule |

## 3. Commerce  *(exclusif avec Mercantilisme)*
*Ce qui circule librement enrichit plus que ce qu'on enferme.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Franchises | −25 % de droits de douane | `TRADE_LEVY` ×0.75 (bout importateur) |
| 2 | Routes longues | +30 % de portée du marché | `MARKET_DIST_FALLOFF` ×0.80 |
| 3 | **V** Comptoir | Fonder un comptoir chez une cité-état (péage partagé) | `CMD_FOUND_COMPTOIR` · `COMPTOIR_TOLL_SHARE` 0.5 |
| 4 | Négoce | −15 % de marge d'import chez les tiers | `IMPORT_MARGIN_THIRD` ×0.85 |
| 5 | Guildes marchandes | +30 % de volume marchand bourgeois | `COMMERCE_W_BOURGEOIS` ×1.30 · `_ELITE` ×0.80 |
| 6 | Libre-échange | Immunisé aux embargos — et ne peut plus en décréter | règle symétrique |

## 4. Mercantilisme  *(exclusif avec Commerce)*
*Ce qui entre passe par mon étape, paie mon droit, ou ne passe pas.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Réserves | +30 % de réserves de chantier et de coussin d'État | `BUILD_RESERVE_BULK` · `AI_SAFE_STOCK_MONTHS` ×1.30 |
| 2 | Régie | Le stockeur d'État achète et revend plus tôt | `SPEC_BUY_BAND` ×1.10 · `SPEC_SELL_BAND` ×0.92 |
| 3 | Blocus | Mon embargo traverse les pactes et ferme mes Centres | règle (`intertrade_embargoed_by` directionnel) |
| 4 | **V** Étape | Désigner une province-étape servie la première, hors export | `CMD_ETAPE_SET(pid,res)` |
| 5 | Péages | Import au pair chez soi · 75 % du péage à la couronne | `IMPORT_MARGIN_OWN` ×0.80 (plancher 1.0) · `TOLL_STATE_SHARE` ×1.50 |
| 6 | Halles | +30 % de capacité d'entrepôt · pertes de stock −15→−10 %/mois | `STOCK_CAP_ENTREPOT` ×1.30 · `STOCK_DECAY_PERISH` ×1.06 (clamp ≤ 0.98) |

## 5. Peuple
*L'étranger devient un bras, un métier, une tech.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Accueil | Pactes migratoires acceptés plus facilement | `AI_OFFER_MIG_OPINION` ×0.7 · `MIG_PACT_MIN` ×0.5 |
| 2 | Écoles | +6 %/mois d'intégration | `ASSIM_K_INST_REF` ×0.8 |
| 3 | Asile | Les réfugiés se fixent plus tôt et repartent moins | `REFUGEE_SETTLE_INTEG` ×0.9 · `REFUGEE_RETURN_PULL` ×0.8 |
| 4 | **V** Affranchissement | Sous pacte, les déportés deviennent migrants | `CMD_OFFER_MIGRATION` élargi (bascule `demography_manumit`) |
| 5 | Tolérance | −25 % de friction des cultures étrangères | `OFF_CULTURE_SAT_PEN`/`_SOC_PEN` ×0.75 (promotions) |
| 6 | Métissage | Héritages étrangers accessibles plus tôt · +20 % de recherche du creuset | `METAB_TIER1/2` ×0.75 (TIER3 épargné — gate Merveille) · `AI_METAB_RES_W` ×1.20 |

## 6. Colonisation
*On part de plus petit, on tient sur la terre rude.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Colons | −15 % de population requise par colonie | `COLONY_MIN_POP` ×0.85 · `COLONY_COST_POP` ×0.90 |
| 2 | Ravitaillement | −25 % de réserve vivrière exigée | `FOOD_STOCK_MONTHS` ×0.75 |
| 3 | Acclimatation | −20 % de malus de terre rude | `HAB_MALUS_K` ×0.80 (2 sites) |
| 4 | **V** Double chantier | 2 chantiers coloniaux simultanés | `ColonyWork[2]`, cadence par slot inchangée |
| 5 | Climats | Apprentissage des climats 10 % plus tôt | `CLIM_LEARN_INTEG` ×0.90 (promotion) |
| 6 | Grand large | +50 % de rendement des colonies lointaines | `COLONY_YIELD_HREF` ×1.5 (promotion) |

## 7. Diplomatie
*Parler plus souvent, plus vite, à deux voix.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Prestige | +25 % d'opinion des alliés et partenaires | `OPINION_ALLY`/`_PACT` ×1.25 (clé : le porteur comme OBJET) |
| 2 | Chancellerie | −20 % de coût d'influence des émissaires | `INFLUENCE_COST_ENVOY` ×0.80 |
| 3 | Oubli | Vos griefs s'effacent 30 % plus vite | `OPINION_MEM_DECAY` ×1.30 · `OPINION_RANCOR_W` ×0.85 |
| 4 | **V** Second émissaire | 2 actions diplomatiques simultanées | `DIPLO_ENVOY_SLOTS` 1→2 (plancher anti-spam PAR slot) |
| 5 | Persuasion | Vos offres acceptées plus facilement | `AI_OFFER_ALLY_OPINION` ×0.75 · `_MIG` ×0.80 |
| 6 | Congrès | Les guerres se concluent plus tôt (les vôtres aussi) | `AI_WAR_EXHAUST` ×0.75 · `AI_WAR_DECISIVE` ×0.85 |

## 8. Vassaux
*Faire jurer, faire payer, faire mûrir.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Serments | Vassaux intégrés 15 % plus vite | `AI_VASSAL_INTEGRATE_YEARS` ×0.85 |
| 2 | Tribut vassal | Contribution plus tôt et +20 % | `AI_VASSAL_CONTRIB_GATE` ×0.85 · `_BASE` ×1.20 (miroir M3f posé 27dc5ed) |
| 3 | **V** Contrats | Choisir le contrat de vassalité à la paix | `PEACE_VASSALIZE` étendu (Servage ×1.5 · Protectorat ×1.0 · Concordat ×0.75 de score) |
| 4 | **V** Leviers | Don, allègement, division, intimidation des vassaux | `CMD_VASSAL_LEVER` (le répertoire IA ouvert) |
| 5 | Annexion | Peut annexer ses vassaux · durée −25 % | gate `annexeur ‖ doctrine` · `ANNEX_INTEGRATION_DISCOUNT` ×1.25 |
| 6 | **V** Suzeraineté | Proposer la vassalité en pleine paix | `CMD_OFFER_SUZERAINTY` · `AI_OFFER_SUZ_OPINION` 55 |

## 9. Production
*Creuser plus profond, équiper plus de bras.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Extraction | +12 % de bras à l'extraction | `EXTRACT_LABOR_SHARE` ×1.12 (à descendre dans la boucle ; inerte sous override — à dire en hover) |
| 2 | Outillage | +30 % d'outils par ouvrier | `TOOLS_PER_LABORER` ×1.30 (promotion) |
| 3 | **V** Exploitation profonde | Paliers d'exploitation jusqu'à 12 (au lieu de 8) | `RAW_BOOST_MAX_TIER` ×1.5 · `CMD_RAW_BOOST` joueur (≤ 2 brutes du tirage, jamais de greffe) |
| 4 | Manufactures | +15 % de capacité des manufactures | `MANUF_QOUT_MULT` ×1.15 (intrants et bras suivent) |
| 5 | Gages | −15 % de coût des manufactures · −20 % de gages d'État | `MANUF_BUILD_COST` ×0.85 · `JOB_UPKEEP_TAX_FRAC` ×0.80 |
| 6 | Rendement | +6 % d'extraction par palier · paliers −25 % | `RAW_BOOST_PER_TIER` ×1.2 · `RAW_BOOST_COST` ×0.75 |

## 10. Infrastructure
*La pierre posée ne redevient jamais une ruine.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Maçons | −10 % de matière par chantier | `BUILD_MAT_MULT` ×0.90 (UN helper, 3 sites + miroir légal) |
| 2 | Carrières | +30 % de réserves de construction | `RAW_WORKS_NEED` · `BUILD_RESERVE_BULK` ×1.30 |
| 3 | Entretien | Usure du bâti −25 % | `VETUSTE_RATE` ×0.75 · `VETUSTE_FLOOR` ×1.15 |
| 4 | **V** Rénovation de masse | File nationale de rénovation · coût −20 % | `CMD_RENOV_MASS` (`RENOV_MASS_SLOTS` 3) · `RENOV_COST_FRAC` ×0.80 |
| 5 | Logements | +25 % de logements par manufacture | `HOUSE_MANUF` ×1.25 (mult DANS la macro partagée) |
| 6 | Intendance | −30 % de surcoût d'étendue | `BUILD_EXTENT_K` ×0.70 (promotion ; 2 sites, miroir UI) |

## 11. Technologie
*La recherche devient une politique.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Bibliothèques | +25 % de bonus de la chaîne Bibliothèque | `SAVOIR_LIB_PER` ×1.25 · `_MAX` ×1.30 |
| 2 | Écoles de ville | +30 % de recherche bourgeoise, +25 % ouvrière | `SAVOIR_W_BOURGEOIS` ×1.30 · `_LABORER` ×1.25 |
| 3 | **V** Programme | Orienter la recherche : −20 % sur un quartier choisi, +10 % ailleurs | `CMD_DOCTRINE_QUARTIER` (10 infl. pour changer) |
| 4 | Copistes | Techs répandues jusqu'à −52 % | `AI_TECH_DIFFUSE_MAX` ×1.30 |
| 5 | Dispense | 2 paliers d'âge en avance (nœuds non faustiens) | gate levé Société 3 + Savoir 4, filtre ⚠ structurel |
| 6 | Sobriété | −10 % techs propres · +50 % techs faustiennes | mults via `doctrine_tech_cost_mult` (clamp [0.70, 1.60]) |

## 12. Connaissances du monde
*Connaître le monde avant de le prendre.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Portulans | ×2 de côtes révélées autour du connu | `FOG_SEA_HALO` ×2 (promotion ; façade pure) |
| 2 | Truchements | Contacts culturels 20 % plus vite | `SYNC_TRADE_METIER`/`_PROFOND` ×0.8 |
| 3 | **V** Expédition | Révéler une contrée lointaine et ouvrir un contact | `CMD_EXPEDITION_LOIN` (10 infl. + 0,25 an de revenu, cadence 36 mois) |
| 4 | Dictionnaires | Héritages étrangers accessibles plus tôt | `METAB_TIER1/2` ×0.8 (TIER3 épargné) |
| 5 | Collèges des langues | +40 % de recherche des peuples digérés | `AI_METAB_RES_W` ×1.4 |
| 6 | Langue franque | Vos alliés et routes partagent leurs cartes | transitivité `known[][]` (un saut, cumulatif) |

## 13. Faustien
*La puissance immédiate contre la damnation lente.*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Pages interdites | −15 % de coût des techs faustiennes | via `ai_tech_tradition_mult` (cid-scopé) |
| 2 | Creusets | Alambic et Atelier de mage débloqués | `scps_manuf_legal` : sorties FLUX/ESSENCE autorisées |
| 3 | **V** Le Pacte | Les trois transmuteurs débloqués · plus aucun refus | `bld_is_faustian` levé · feed-check élargi · `FAUST_BRECHE_CAUTION` sans effet joueur |
| 4 | Or du puits | +30 % d'or de la Foreuse — la monnaie se débase | panier précieux ×1.30 → frappe → IPM |
| 5 | Terre changée | +25 % de mutations · la charge se lave −35 % | `FAUST_MUTATION_K` ×1.25 · `CHARGE_DECAY` ×0.65 |
| 6 | Prix consenti | +25 % de sortie des machines · +50 % de charge | `FAUST_YIELD_MULT` ×1.25 · `FAUST_SPAWN_CHARGE` ×1.50 |

---

# Les courants politiques (un seul des quatre — re-siègent l'assiette d'influence)

## 14. Aristocratie  *(assiette : élites ×0.0025/mois)*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Bannerets | +25 % de contribution vassale | `AI_VASSAL_CONTRIB_BASE` ×1.25 · `_GATE` ×0.85 |
| 2 | Offices | +30 % de loyauté achetée · renvoi +50 % de grief | `COUNCIL_PAY_ADJ` ×1.30 · `COUNCIL_DISMISS_GRIEF` ×1.50 |
| 3 | **V** Adoubement | Promouvoir des bourgeois en élites (8 infl. + la dot) | `CMD_ADOUBE(pid)` via `mobility_move`, refusé au plafond `SHARE_CAP_ELITE` |
| 4 | Fiefs | +35 % de charges d'élite par édifice | `EDI_ELITE_JOBS` ×1.35 |
| 5 | Ban féodal | +15 % de moral · −25 % d'impôt des élites | `moral_mul` ×1.15 · `INCOME_TAX_RATE_ELITE` ×0.75 |
| 6 | Clôture | Noblesse plus accessible · bourgeoisie plus fermée | `PROMOTE_BASKET_MULT_ELITE` ×0.75 · `PROMOTE_BASKET_MULT` ×1.30 (promotions) |

## 15. Bourgeoisie  *(assiette : bourgeois ×0.0006/mois — `INFLUENCE_PER_BOURGEOIS`)*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Chartes | −15 % de coût administratif | `ADMIN_BASE` ×0.85 |
| 2 | Jurandes | +20 % de volume marchand · accession +10 % | `COMMERCE_W_BOURGEOIS` ×1.20 · `PROMOTE_BASKET_MULT` ×1.10 |
| 3 | **V** Emprunt intérieur | La bourgeoisie prête à l'État (2e tranche de crédit) | `CMD_LOAN_DOMESTIC` · `CREDIT_LINE_DOMESTIC` 1.0 · banqueroute = grief plein + fermé 10 ans |
| 4 | Crédit | −20 % de taux d'intérêt | `CREDIT_RATE_BASE` ×0.80 |
| 5 | Robe | Un siège de Conseil supplémentaire | slot ajouté (le vivier est déjà multi-classes) · `COUNCIL_ROT_BOOST` ×1.25 |
| 6 | Clés de la ville | Accession bourgeoise −25 % | `PROMOTE_BASKET_MULT` ×0.75 (l'assiette grossit d'elle-même) |

## 16. Populaire  *(assiette : journaliers ×0.00012/mois — À MESURER au chronicle)*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Doléances | Politique +20 % ressentie · −15 % d'agitation | `POL_SAT_W` ×1.20 · `W_AGITATION_UNREST` ×0.85 |
| 2 | Pain | Panier vital exonéré d'impôt · +25 % de croissance des provinces contentes | `TAX_EXEMPT_BASKET_MULT` ×1.30 · `POP_SAT_W` ×1.25 |
| 3 | **V** Levée en masse | Conscription au-delà de la limite pendant 5 ans (agitation +1,5/mois) | `CMD_LEVEE_MASSE` · `SOLDE_OVER_K` ×0.35 · `LEVEE_MASSE_AGIT` |
| 4 | **V** Concession | Apaiser une province avant la révolte · coût −30 % | `CMD_CONCEDE(pid)` (bloc existant `scps_revolt.c:955`) · `CONCEDE_GOLD` ×0.70 |
| 5 | Impôt du rang | +20 % d'impôt des élites · rangs fermés | `INCOME_TAX_RATE_ELITE` ×1.20 · `EDI_ELITE_POP_PCT` ×0.85 (revers turchinien émergent) |
| 6 | Souveraineté | Céder ne coûte plus ni légitimité ni capacité · verrou ÷2.5 | `C3_K_HOLLOW` ×0.25 · `C3_L_HOLLOW` ×0 · `CONCEDE_CD_DAYS` ×0.40 |

## 17. Divin  *(assiette : foi bâtie × (1+ferveur) × `INFLUENCE_PER_FAITH` 0.08 — volatile par nature)*

| # | Idée | Bonus | Levier |
|---|---|---|---|
| 1 | Onction | +25 % de légitimité par la foi | `LEGIT_K_FAITH` ×1.25 (promotion) |
| 2 | Ferveur | +20 % de ferveur · dure 5 ans de plus | `PROVMOD_FERVEUR_K` ×1.20 · `_DECAY` ×0.75 |
| 3 | Sacerdoce | Missionnaire pour tous les crédos · fondation hors plafond | règle (`scholar_role` + `religion_can_found`) |
| 4 | **V** Appel à la foi | Mobiliser la ferveur : concorde (−30 % d'agitation 5 ans) ou zélotes (levée +1, minorités +50 % de grogne) | `CMD_FAITH_CALL`, cadence 10 ans |
| 5 | Clergé | 2 lettrés · missions +40 % plus longues | 2e slot `g_scholar` · `SCHOLAR_DURATION_DAYS` ×1.40 (promotion) |
| 6 | Orthodoxie | Schismes ÷2 · minorités +60 % de grogne | `AI_DERIVE_ODDS` ×2.0 · `RELIG_MINORITY_SAT` ×1.60 |

---

# Synergies de paires (sous-menu, entretien fibonaccien 2/3/5/8…/mois)

| Paire | Synergie | Bonus |
|---|---|---|
| Commerce × Aristocratie | Maison de commerce | +15 % d'influence par comptoir debout (`INFLUENCE_PER_NOBLE` ×1.15) |
| Commerce × Diplomatie | Traités de commerce | −20 % de douane vers les partenaires |
| Commerce × Connaissances | Route des langues | Contacts par mer/terre +25 % |
| Commerce × Bourgeoisie | Ligue des villes | Chaîne commerciale +40 % de plafond |
| Mercantilisme × Production | Manufacture d'État | Le dispatch sert les manufactures avant le marché · +25 %/palier sur la brute d'étape |
| Mercantilisme × Colonisation | Pacte colonial | Colonies liées à la métropole · convois −40 % de ponction |
| Mercantilisme × Bourgeoisie | Compagnie privilégiée | Mes marchands exemptés de mon embargo · crédit +15 % |
| Offense × Vassaux | Marches d'épée | Inféoder une région occupée à la paix, sans cicatrice |
| Offense × Défense | Marteau et enclume | Le secours d'un long siège entre au choc en défenseur |
| Offense × Divin | Guerre sainte | CB religieux gratuit · +15 % de moral sous ferveur |
| Offense × Aristocratie | Noblesse d'épée | Un siège pris = un adoubement gratuit |
| Offense × Populaire | Nation en armes | Levée en masse sans agitation en guerre défensive |
| Offense × Production | Arsenal du royaume | +30 % d'armes des manufactures |
| Défense × Infrastructure | Marches de pierre | Rénovation priorise la frontière · usure des forts ÷2 |
| Défense × Populaire | Levée des paroisses | Le ban lève des gardes (moral incassable) |
| Diplomatie × Vassaux | Concert des princes | Suzeraineté offerte −20 % · les ligues gèlent sous serment frais |
| Diplomatie × Peuple | Cour cosmopolite | Pactes +50 % d'opinion · flux d'alliés +20 % |
| Vassaux × Peuple | Noces des maisons | Vassaux sous pacte migratoire intégrés bien plus vite |
| Vassaux × Aristocratie | Concordat des maisons | Adoubement gratuit sur vassal intégré |
| Colonisation × Connaissances | Grandes découvertes | Coloniser hors adjacence sur zone révélée · portée navale +50 % |
| Colonisation × Peuple | Terres promises | Colonies +30 % d'attraction · ferveur fondatrice +40 % de durée |
| Colonisation × Faustien | Pain des lointains | −30 % de vivres exigés tant qu'une Corne tourne (le FROID approche) |
| Technologie × Connaissances | Grand Atlas | Contacts culturels −20 % de seuils |
| Technologie × Peuple | Collège des langues | Recherche du creuset +15 % · catch-up 0.50 |
| Technologie × Bourgeoisie | Presses de la guilde | Recherche bourgeoise +40 % |
| Production × Infrastructure | Atelier de pierre | Logements +15 % · paliers −20 % |
| Production × Populaire | Bras de l'atelier | +10 % de bras à l'extraction |
| Infrastructure × Populaire | Grands travaux | L'or des chantiers aux journaliers · agitation −15 % |
| Peuple × Divin | Conversion des âmes | Assimilation +25 % sur régions converties |
| Infrastructure × Divin | Œuvre de pierre | Le bâti de foi ne vieillit plus (usure ÷2) |
| Faustien × Divin | Autel de la Brèche | +30 % de ferveur sur provinces à transmuteur |

Faustien × Technologie : paire VIDE (Sobriété l'interdit — décision H3.2).

---

# Annexe technique (l'essentiel des passes d'agents, condensé)

- **Patron d'effet** : `tune_f × decree_mult × doctrine_mult(cid)` au site de
  lecture, JAMAIS `tune_set` ; **clamp composé [0.60, 1.60] par clé**
  (`DOCFAUST_*` exempt) ; kill-switch par famille (tous les mults à 1.0 =
  golden byte-identique, à prouver en entrée de vague).
- **Golden** : P1/P3 joueur seul par construction ; adoption IA (P3, par
  score) = re-baseline + sweep apparié 3×3.
- **Save** : états neufs (2e émissaire, comptoir franc, quartier de
  recherche, étape, tranches de crédit, cooldowns d'adoubement/appel/levée,
  compte d'idées) = accumulateurs sérialisés, `save_sane`, bump.
- **Pièges clés par doctrine** : Diplomatie — `opinion[a][b]` : clé sur le
  porteur comme OBJET · Offense — `SOLDE_*` sont des `#define` warhost à
  promouvoir ; idée 4 = conservation (invariant muet) · Commerce — un
  Comptoir chez soi supprime déjà tout le péage : le comptoir franc PARTAGE
  (sinon on assèche les cités-états) ; le catchment n'a pas de rayon, la
  portée réelle = `MARKET_DIST_FALLOFF` · Mercantilisme — marge OWN plancher
  1.0 ; décrue plafond 0.98 ; l'étape ne descend jamais une province sous sa
  cible vivrière · Peuple — idée 5 est la contrepartie de l'idée 4 (le
  servile libéré pèse plein) ; ne pas toucher `_AMP` · Colonisation — le
  miroir UI applique le MÊME mult ; `HAB_MALUS_K` a 2 sites · Vassaux — le
  gate `annexeur` se lit sur la doctrine, pas l'éthos · Production —
  `MANUF_BUILD_COST` : 3 sites payeurs + le prix affiché · Infrastructure —
  quantité = 1 helper ; formule d'étendue dupliquée dans l'UI ; `HOUSE_MANUF`
  dans la macro partagée · Technologie — `tech_cost()` sans cid : mult aux 2
  sites appelants ; la Dispense n'ouvre JAMAIS l'Éveil · Connaissances —
  `arch_depth` = cache à rafraîchir post-load ; la transitivité déplace
  l'avènement des Découvertes (sweep dédié) · Faustien — l'IA saute les
  transmuteurs (`scps_ai.c:1242`) : atterrir joueur-seul ; leviers de genèse
  et grain-monde écartés · Populaire — le verbe mord au site de la solde
  (warhost saute la mobilisation auto du joueur) ; `g_lowsat_streak` static à
  migrer si lu · Divin — grain région de la religion : contacts par pid
  seulement · Courants — assiettes à parité ~4/mois visée, Populaire à
  mesurer avant de figer.
- **Collisions inter-doctrines cataloguées** (composés à clamper) :
  `BUILD_RESERVE_BULK` (Infra × Mercantilisme ×1.69), `AI_METAB_RES_W`
  (Peuple × Connaissances × synergies), `AI_VASSAL_CONTRIB_BASE` (Vassaux ×
  Aristocratie ×1.5 — miroir M3f posé), `W_AGITATION_UNREST`, `HOUSE_MANUF`.
  `SIEGE_LOOT_FRAC` attaquant/victime = duel émergent SAIN.
