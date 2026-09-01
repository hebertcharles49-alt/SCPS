# ANNEXE — Les branches de Desseins (bibliothèque de gabarits)

Passe de design 2026-09-01 (« termine le design ») : 7 agents, un par branche.
Sources imposées : DESIGN_MISSIONS_DOCTRINES.md §2 (grammaire, canaux §2.4,
pivots §2.5) + LEVIERS.md + DESIGN_DOCTRINES_ANNEXE.md (frontière : la
DOCTRINE = capacité durable, le DESSEIN = jalon ponctuel daté/nommé).

Règles transverses : cibles résolues DÉTERMINISTIQUEMENT sur le monde réel
(graine+cid) ; conditions = prédicats d'état réel (jamais `region[].owner`,
dérivé) ; récompenses = claim nommé / mult national DATÉ 15-25 ans / provmod
nommé / coordonnée bâtie / déblocage / cadence — JAMAIS d'or créé ; l'échelon
N arme le N+1 ; scellage gratuit, pivot = 15-25 d'influence ; golden P1
intact par construction (joueur seul).

Attribution des branches à la genèse : **Sol** (toujours) + **une branche
d'ouverture** (Mer & Comptoirs si capitale côtière, sinon Routes & Caravanes)
+ **une branche d'esprit** par cascade déterministe (héritage/éthos NATIF de
genèse, jamais le métabolisé — la première vraie gagne) :
```
1. FOI      si w_faith(cid) >= AI_FAITH_ZEAL (0.5)
2. HORDE    si héritage==CLANIQUE && éthos==DOMINATEUR
3. CREUSET  si héritage==ADAPTATIF || éthos==PACIFISTE
4. SAVOIR   si héritage ∈ {ÉSOTÉRIQUE, MÉCANISTE}
5. défaut   SAVOIR
```
Une passe d'harmonisation clôt le document.

---

## Branche du CREUSET

**Identité :** prouver par jalons que l'étranger est devenu une ressource :
un pacte tenu → un quartier plein → une génération intégrée → un héritage
digéré → un climat appris par les gens → la nation des nations. Frontière
avec la doctrine Peuple tenue par le CANAL : la doctrine règle des TAUX
(`ASSIM_*`, `REFUGEE_*`, `METAB_TIER1/2`…) — le Dessein n'en touche AUCUN et
prend six canaux que personne d'autre ne réclame (gate de maturation des
pactes, provmod neuf, K_inst BÂTI, coût des nœuds de signature, DON du bit
climat, ferveur/cd_days — les deux réservés au Dessein par la doctrine
Colonisation).

**Éligibilité — cascade déterministe des branches d'esprit (premier match
gagne, lue sur l'héritage/éthos NATIF de genèse, jamais le métabolisé) :**
```
1. FOI      si w_faith(cid) >= AI_FAITH_ZEAL (0.5)
2. HORDE    si héritage==CLANIQUE && éthos==DOMINATEUR
3. CREUSET  si héritage==ADAPTATIF || éthos==PACIFISTE
4. SAVOIR   si héritage ∈ {ÉSOTÉRIQUE, MÉCANISTE}
5. défaut   SAVOIR
```

**Échelons 1-4 communs · pivot (20 influence) · 3 échelons par voie = 7/partie.**

| # | Nom FR (EN) | Cible (résolution) | Condition (prédicat moteur) | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **Le Droit de passage** (*The Right of Passage*) | le voisin PACIFIQUE de culture la plus DISTANTE (connu, pas en guerre, opinion ≥ 0, argmax distance culturelle, départage cid croissant) | pacte migratoire en vigueur avec la cible, tenu ≥ 365 j (latch sérialisé) | **Cadence permanente** : `MIG_PACT_ALLY_GATE_DAYS` ×0.5 pour MES pactes (12 ans → 6) | On n'a pas signé un traité de commerce : on a signé le droit de venir. |
| 2 | **Le Quartier des arrivants** (*The Newcomers' Quarter*) | MA province hébergeant le plus d'âmes migrantes+réfugiées (argmax sur `prov[].pop.groups`, pid croissant) | ≥ 600 âmes migrantes/réfugiées dans le pays (somme directe, aucun accumulateur neuf) | **Provmod nommé** « Le Quartier des arrivants » (`PMOD_QUARTIER` neuf, intensité = part d'arrivants — s'éteint si le quartier se vide, `PROVMOD_QUARTIER_K` 0.25) | Un faubourg d'accents, de métiers et d'enfants — la ville a poussé un bras. |
| 3 | **La Première génération** (*The First Generation*) | le GROUPE (culture nommée) le plus nombreux d'arrivée non-native | ce groupe ≥ 0.60 d'intégration moyenne sur ≥ 400 âmes | **Coordonnée bâtie permanente** : `K_inst +1.0` sur la province du Quartier (« le Tribunal des deux langues ») — l'assimilation accélère par la voie EXISTANTE (`ASSIM_K_INST_AMP` lit le bâti), effet émergent | Leurs enfants plaident dans notre langue et jurent sur notre code. |
| 4 | **Ce que leurs pères savaient** (*What Their Fathers Knew*) | l'héritage étranger le plus digéré (argmax hors natif) | cet héritage franchit `METAB_TIER1` (le Dessein LIT le seuil, ne le déplace pas) | **Remise DATÉE 20 ans** : nœuds de SIGNATURE de cet héritage ×0.75 (deux sites de coût, jamais `tech_cost()`) | Assez d'entre eux ont fait souche pour que leurs outils cessent d'être un secret. |
| — | **PIVOT** (20 infl., irréversible) | — | échelon 4 scellé | garder le trop-plein chez soi (A) ou le laisser fonder au-dehors (B) | — |
| A5 | **Le Concile des langues** (*The Council of Tongues*) | le siège Société du Conseil | ≥ 3 héritages étrangers > `METAB_TIER1` | **Ministre issu de la diaspora** assis d'office (rang III, loyauté haute — motif Pizarro/Endral), membrane « né au Quartier des arrivants » | Le chancelier a l'accent d'un pays que son grand-père a fui. |
| A6 | **L'Alliage** (*The Alloy*) | la paire d'héritages T4 la plus proche de l'accès plein (natif × plus digéré) | accès plein aux DEUX (prédicat combo T4 existant) | **Remise DATÉE 25 ans** ×0.70 sur les nœuds COMBO T4 (canal coût, jamais la barre d'accès) | Deux métiers qui ne se connaissaient pas font un troisième que personne n'avait. |
| A7 | **La Nation des nations** (*A Nation of Nations*) — parachèvement | ma capitale | `econ_country_metabolized ≥ 0.35` ET `fracture ≤ 3.0` (pas une poudrière) | **Provmod permanent** « La Ville aux cent langues » + `PE_infra +1.5` bâtie + Annale + épithète candidate | Le recensement ne demande plus d'où l'on vient : la réponse serait trop longue. |
| B5 | **Le Climat appris** (*The Climate Learned*) | la classe de climat étrangère la plus représentée chez mes groupes déplacés SANS le bit (via `home_reg → world_climate_class` ; les déportés `home_reg=-1` ne comptent JAMAIS) | un groupe de ce climat ≥ 0.75 d'intégration (avant le seuil 0.99 du legs automatique) | **Déblocage cliquet** : `Country.climates |= bit` — le bit est DONNÉ une génération plus tôt (le canal réservé au Dessein) | Le désert n'a pas été conquis : il a été raconté, par des gens qui y sont nés. |
| B6 | **Les Cadets au large** (*The Younger Sons Abroad*) | la zone frontière Z : BFS d'habitabilité VUE PAR MOI (`world_hab_for`, éclairée par le bit de B5) depuis la capitale | 2 colonies vivantes fondées dans Z | **Mult DATÉ 15 ans** : `PROVMOD_FERVEUR_K` ×1.25 | Le second fils n'hérite de rien ; on lui donne une carte et le nom d'une côte. |
| B7 | **Les Marches d'accueil** (*The Marches of Welcome*) — parachèvement | la première colonie de Z | ≥ 4 provinces colonisées à pop majoritairement d'arrivée non-native | **Cadence permanente** `cd_days` ×0.75 + provmod « La Porte des peuples » (PE_infra bâti) + Annale + épithète | Le royaume n'a pas exporté des colons : il a exporté ses invités, et ils ont fondé pour lui. |

**Notes (agent) :** re-résolution = la MÊME fonction pure à la clôture,
jamais un échec (voisin annexé ⇒ le suivant ; province cédée ⇒ nouvelle
argmax ; le bit climat est un cliquet jamais repris) ; départages cid/pid
croissants, zéro rand · golden P1 : détection/scellage gatés `human_player`,
5 mults par patron decree_mult, kill-switch (5 clés à 1.0 +
`PROVMOD_QUARTIER_K=0` + branche non générée = byte-identique) ; TROIS
récompenses écrivent l'état MONDE (K_inst bâti, bit climat, ministre) —
joueur-seul en P1, symétrie IA en P4 · save : `PMOD_QUARTIER` appendu en fin
d'enum (grep `PMOD_COUNT`), macro `ECON_PROVMOD_BODY` partagée
moteur+membrane (le mult DEDANS), latches ⇒ bump + `save_sane` · pièges :
compter les âmes sur `prov[].pop.groups` (jamais `region[].pop`, miroir
stale) ; le gate de pacte est lu HORS boucle (`scps_demography.c:916`) — le
descendre dans la boucle de paires sinon bouton mort ; `arch_depth` = cache à
rafraîchir post-load ; clamp composé [0.60, 1.60] sur les coûts de nœuds
(composition avec tradition + Cénacle) · rimes : doctrine Peuple = taux /
Dessein = jalons, canaux disjoints ; le Creuset grossit MÉCANIQUEMENT
l'assiette du courant Populaire (les arrivants entrent journaliers) ;
anti-rime assumée avec l'Orthodoxie du Divin (la diaspora hétérodoxe gronde
×1.6 — la facture d'un choix, ne pas corriger).

**Tunables neufs (`DESS_CREUSET_*`)** : `_PACT_DAYS` 365 · `_SOULS` 600 ·
`_INTEG` 0.60 · `_CLIM_INTEG` 0.75 · `_METAB_FIN` 0.35 · `_FRACTURE_MAX` 3.0
· `_COLONIES` 4 · `_PIVOT_INFLUENCE` 20 · 5 clés de mult ·
`PROVMOD_QUARTIER_K` 0.25.

---

## Branche MER & COMPTOIRS

**Identité :** franchir — voir au-delà du trait de côte, poser un pied, le
nourrir par le commerce, puis un choix qu'on ne reprend pas : tenir des
QUAIS (comptoirs, verrous, péages) ou tenir des TERRES (peuplement, climats,
un continent neuf). L'un taxe la mer, l'autre la traverse.

**Éligibilité (genèse, déterministe) :** capitale côtière OU ≥ 3 provinces
côtières (`Province.coastal`, posé à `econ_init`) — sinon Routes & Caravanes.
**Rade de référence R0** (ancre des résolutions) : ma province côtière
maximisant (harbor de sa région, puis port bâti, puis −pid) — `Region.harbor`
lu À LA RÉSOLUTION seulement, jamais par un reader façade.

**Tronc commun 1→4 · pivot en 5 (20 influence) · 4 échelons par voie = 8.**

| # | Nom FR (EN) | Cible (résolution) | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **Les éclaireurs** (*The Scouts*) | les 3 provinces côtières inconnues de plus faible `world_sea_days(R0,·)`, tie pid↑ | connaissance acquise sur les 3 | **Déblocage permanent** : rayon de brouillard +1 région (canal `ages_fog_radius_add`) · désigne Z1 | « Les barques de %s reviennent avec un trait de côte que nul n'avait dessiné. » |
| 2 | **La première fondation** (*The First Landing*) | Z1 : BFS habitabilité depuis R0 (côtière, libre, non-colonisée, ≤ 6 sauts ; max hab × hab. culturelle) | `prov[Z1].owner==cid && colonized` | `PROVMOD_FERVEUR_K` ×1.30, **15 ans** | « La quille de %s touche le sable de %s : ce n'est pas une escale, c'est une adresse. » |
| 3 | **Le grenier de la mer** (*The Sea Granary*) | la colonie de l'échelon 2 (repli : ma colonie vivante la plus lointaine) | `econ_colony_food_ok` par la voie GRENIER, avec `food_sat < gate`, tenu 12 clôtures (nourrie par le commerce, pas par sa terre) | `COLONY_FOOD_GATE` ×0.80, **20 ans** | « %s ne mange pas ce qu'elle sème : elle mange ce qui arrive, et cela suffit désormais. » |
| 4 | **Les clefs de la mer** (*The Keys of the Sea*) | LE DÉTROIT : goulet de `world_chokepoints()` le plus proche de R0 (⚠ stocker la province-flanc + la CELLULE (sx,sy), jamais l'index — la table est reconstruite ≤ 180 j) ; repli : hub du Centre le plus dépendant | `prov[pid].owner==cid` ET `econ_region_has_keeper` (sans habitant, route franche M13) ET ≥ 1 route maritime au goulet | **Provmod permanent** « Le Verrou » (P_open +0.5) · ouvre le PIVOT | « Deux caps, un chenal : tout ce qui va d'un monde à l'autre passe désormais sous les murs de %s. » |
| 5 | **PIVOT** — Thalassocratie OU Empire du large | — | échelon 4 scellé | 20 d'influence, irréversible | « On tiendra les quais — ou on tiendra les terres. Jamais les deux. » |

**Voie A — LA THALASSOCRATIE (l'empire des quais)**

| # | Nom | Cible | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 6A | **Le comptoir d'outre-mer** (*The Factory Abroad*) | le Centre étranger le plus dépendant (via `scps_market_catchment` de mes pid) ; comptoir = ma côtière rattachée la plus proche du hub | Comptoir bâti (`edi_built`) ET catchment = ce centre | *NOUVEAU TUNABLE `IT_CONN_MULT_COMPTOIR` (promotion du littéral 0.67, `scps_intertrade.c:1042`)* ×0.85, **20 ans** | « Une maison, un quai, un registre : %s n'a pas pris %s — elle y a ouvert boutique. » |
| 7A | **Le péage des détroits** (*The Toll of the Narrows*) | le second goulet (même ordre, hors le tenu) | 2 goulets tenus (owner + keeper) ET ≥ 3 routes maritimes payantes, 12 clôtures | *NOUVEAU TUNABLE `CHOKE_TOLL_FRAC` (promotion d'`IT_CHOKE_TOLL` 0.12)* ×1.30, **25 ans** — transfert réel exportateur→tenant | « Le monde n'a que quelques portes. %s en tient deux, et présente la note. » |
| 8A | **La ligue des quais** (*The League of the Quays*) | les 2 cités-états portuaires les plus proches par mer de R0 | pacte commercial avec les deux, tenu 5 ans, aucune guerre | *NOUVEAU TUNABLE `BUILD_COST_MULT_PORT` déf. 1.0* ×0.75 sur les quantités de la famille Port→{Arsenal, Amirauté, Port marchand}, **20 ans** | « Les échevins de %s et de %s signent le même registre : pas un empire — une adresse commune. » |
| 9A | **L'EMPIRE DES QUAIS** — parachèvement | état global | ≥ 40 % des routes maritimes ouvertes du monde me touchent ou franchissent mes goulets · ≥ 6 Comptoirs/Ports marchands côtiers | **Provmod permanent** « La Corne d'or » sur ma capitale portuaire (PE_infra +2.0) + Annale + épithète | « On ne demande plus qui règne sur la mer : on demande à qui l'on paie. » |

**Voie B — L'EMPIRE DU LARGE (l'échelle coloniale §2.3, barreaux 4→7)**

| # | Nom | Cible | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 6B | **S'endurcir au climat** (*Hardened to the Climate*) | le climat étranger le plus porté par mes groupes déplacés (bit absent) | ≥ 400 habitants de ce climat à intégration ≥ `CLIM_LEARN_INTEG` | **Déblocage permanent** : `climates |= bit` — le désert, la jungle s'ouvrent | « Une génération a survécu à %s. La suivante y naîtra sans le savoir. » |
| 7B | **La seconde vague** (*The Second Wave*) | Z2 : re-BFS depuis ma colonie la plus lointaine | ≥ 4 colonies vivantes fondées après l'échelon 2 | cadence `cd_days` ×0.70, **20 ans** | « Le second convoi n'attend plus le retour du premier. » |
| 8B | **Par-delà les mers** (*Beyond the Seas*) | la côtière vacante de meilleure habitabilité sur un continent ≠ le mien, par `sea_days`↑ | possédée + colonisée + continent ≠ | *NOUVEAU TUNABLE `COLONY_OVERSEAS_MULT` (promotion du ×2, `scps_econ.c:6385`)* ×0.75, **25 ans** — ⚠ vérifier QUEL chemin fonde outre-mer chez le JOUEUR (ColonyWork vs econ_colonize_overseas), sinon mult inerte (bug miroir déjà vécu) | « L'autre rive n'est plus un mot de marin : c'est une province qui paie l'impôt. » |
| 9B | **UN MONDE NOUVEAU** — parachèvement | le continent visé | ≥ 25 % de ses provinces habitables à moi · ≥ 8 colonies vivantes | **Provmod permanent** « Porte des Indes » sur la capitale coloniale + Annale + épithète | « On ne dit plus "les terres de l'ouest". On dit leur nom, et c'est %s qui l'a donné. » |

**Notes (agent) :** re-résolution par le MÊME comparateur en excluant
l'ancienne cible, chaque bascule émet une ligne du Fil (le joueur voit la
cible bouger) ; goulets : ré-apparier par CELLULE (≤ 3 cellules), jamais par
index ; Centre absorbé ⇒ 6A SE SCELLE (objectif atteint par un autre chemin)
· golden P1 par construction ; 4 tunables neufs = promotions aux valeurs
actuelles, kill-switch byte-identique · mults datés : stocker le JOUR
D'ÉCHÉANCE absolu, jamais un compte à rebours (bump + savetest) · pièges :
verrou sans habitant = route franche (`TOLL_NEEDS_KEEPER`) ; grain province
(projection région→pid UNE FOIS à la résolution) ; `BUILD_COST_MULT_PORT` =
3 sites de quantité + miroir légal ; `ages_fog_radius_add` lu à DEUX sites ;
branche la plus exposée aux fins EAU/CHAUD (l'endgame dé-côtise les provinces
englouties — la re-résolution est une condition de survie) ; empilement
Mercantilisme×Thalassocratie ×1.95 sur la part d'État du péage (taux vs
split, pas un doublon — métrique sweep : trésor des cités-états an 180) ·
rimes : zéro clé commune avec Colonisation/Connaissances/Commerce (le
`IMPORT_MARGIN_THIRD` a été volontairement écarté de 6A — effet exact de
Commerce idée 4) · restes à mesurer : les seuils (40 % routes / 25 %
continent) au chronicle apparié ; repli 7A si aucun goulet retenu (« ≥ 5
routes maritimes ouvertes » à valider).

---

## Branche du SAVOIR

**Identité :** les autres branches prennent de la terre ; celle-ci prend la
tête de quelqu'un d'autre. L'arc est celui de l'arbre lui-même : la chaîne
Bibliothèque → les premiers tiers → métaboliser un voisin → un combo T4 → un
apex T5 — et son pivot est le seul endroit des Desseins qui regarde le
faustien en face. Elle ne DONNE jamais de savoir : elle le rend atteignable.

**Éligibilité :** héritage Ésotérique/Mécaniste, OU `w_base[2] ≥ 0.60`
(⚠ `w_savoir` N'EXISTE PAS — les axes IA sont expand/trade/build/faith/
faustian ; on lit `w_build` et son SOCLE DE GENÈSE `w_base[2]`, jamais la
valeur courante qui glisse avec les factions — sinon l'arbre n'est plus
déterministe au rejeu).

| # | Nom FR (EN) | Cible (résolution) | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **Le premier rayonnage** (*The First Shelf*) | pid* = ma province au `build.savoir` max (tie : capitale, puis pid↓), latché au scellage | Bibliothèque bâtie sur pid* | **Provmod « Le Quartier des lettrés »** : `build.savoir +1.0` bâti, permanent | On avait des greffes, des rôles et des dettes. On n'avait pas encore d'endroit où les ranger. |
| 2 | **La règle et le cloître** (*The Rule and the Cloister*) | pid* | Monastère bâti sur pid* | **Mult daté 20 ans** ×1.15 sur `AI_RESEARCH_INCOME_W` — plié dans `rw` (les 4 sites + miroir UI `scps_api.c:802`) | Des hommes ont juré de ne rien posséder afin de pouvoir tout recopier. |
| 3 | **Lire la langue du voisin** (*Reading the Neighbour's Tongue*) | h* = l'héritage étranger le plus digéré (re-résolu chaque clôture) | `tech_heritage_access_tier(h*) ≥ 2` (contact métier OU métabolisation TIER2) | **Remise CIBLÉE** ×0.70 sur le premier nœud-signature non acquis de h* (aux DEUX sites de coût) | Trois générations de leurs enfants sont nées ici. Nos savants viennent de s'apercevoir que leurs grands-pères savaient forger autrement. |
| 4 | **L'alliage** (*The Alloy*) | combo* = le combo T4 trié par (âge ouvert, déficit d'accès, TechId) | `unlocked[combo*]` | mult ×1.20 daté 25 ans (composé avec #2) + Annale + Fil. Ouvre le PIVOT | Ni leur métal ni notre mesure : la chose sortie de la forge n'a de nom dans aucune des deux langues. |
| 5A | **PIVOT — La Voie propre** (*The Clean Way*) | — | 2 nœuds t3 non-⚠ acquis ET AUCUN ⚠ jamais pris | ×1.20 daté 20 ans plié dans `rw` LUI-MÊME ⇒ le découplage §27 suit : l'arbre propre accélère, les ⚠ renchérissent d'autant et restent à cadence baseline. Zéro règle neuve | On scelle un rayon et on jette la clef. |
| 5B | **PIVOT — La Porte entrouverte** (*The Door Ajar*) | — | ≥ 1 nœud ⚠ acquis OU ≥ 1 province à charge > 0 | **RÈGLE** : `ts[cid].has_ruins_access = true` — la porte de l'arcane s'ouvre (voir TROUVAILLE) | Personne ne t'empêchera plus. C'est tout ce que la porte accorde, et c'est déjà trop. |
| 6A | **La chaire cosmopolite** (*The Cosmopolitan Chair*) | apex* trié par (âge ouvert, déficit, TechId) — la clé « âge ouvert » est OBLIGATOIRE : `TECH_APEX_CONCILE` est Savoir t5, gaté par… la Brèche (l'ironie serait un blocage) | accès plein tier 3 aux TROIS héritages de apex* | remise ciblée ×0.65 sur apex* + 2e cran du provmod (+1.5) | Trois peuples, une salle, et pour la première fois personne n'y traduit vers sa propre langue. |
| 6B | **Le cercle d'invocation** (*The Summoning Circle*) | `TECH_INVOCATION` (le seul ⚠ Savoir t3, hors gate d'âge) | `unlocked[TECH_INVOCATION]` | ×1.25 daté 15 ans sur les SEULS sites de REVENU (le découplage §27 au défaut) ⇒ pour la première fois les ⚠ accélèrent vraiment — le prix est l'entropie | Les soldats sont apparus sans mères et sans solde. Le trésorier a souri le premier ; il est aussi le premier à avoir cessé de dormir. |
| 7A | **Le Concile des savants** — parachèvement | apex* | `unlocked[apex*]` | **Provmod permanent « La Chaire du Concile »** (+2.5 savoir bâti) + Annale + épithète + **un slot de doctrine ouvert en avance** (§4.5) | Aucun d'eux ne détenait la réponse. Ensemble, ils détenaient assez de désaccords pour la faire sortir. |
| 7B | **Ce qui répond** (*That Which Answers*) — parachèvement | `TECH_SAVOIR_INTERDIT` — **jamais `TECH_EVEIL`** (déclencheur de crise : le Dessein mène au bouton, il ne le presse pas) | `unlocked[TECH_SAVOIR_INTERDIT]` | **Provmod « La Crypte ouverte »** (+2.5) + Annale + épithète + slot de doctrine en avance | Le sceau portait neuf avertissements et trois malédictions. Le Conseil y a vu une table des matières. |

**⚠ TROUVAILLE (ticket) : la porte de l'arcane est MURÉE.**
`tech_state_init(..., false)` pour tous les pays et AUCUN site ne repasse
`has_ruins_access` à true (seul un banc le fait) — or Invocation/Éveil/Savoir
interdit portent `needs_ruins` ⇒ **tout le bras faustien du Savoir est
inatteignable en partie réelle aujourd'hui**. « La Porte entrouverte » est le
porteur légitime de la clef — mais trancher d'abord avec le joueur : oubli ou
mise en sommeil délibérée ?

**Notes (agent) :** le canal `EvEffect.unlock_branch/tier` est INUTILISABLE
en récompense nationale (il écrit `age_tech_mask` MONDIAL — ouvrirait le
palier pour tout le monde ; et jamais Savoir 4/5 : doublon Dispense /
souveraineté Brèche) · `arch_depth` = cache — préférer `ai_heritage_access`
(recalcule) dans les prédicats · la vétusté masque l'usure d'une province au
provmod (paraît neuve plus longtemps — à dire en hover) · `SAVOIR_LIB_MAX`
plafonne la chaîne : le provmod est un cadeau de DÉBUT de branche, inerte au
plafond (relever le plafond = le métier du Cénacle) · clamp composé
[0.60, 1.60] sur remises (composition diffusion × tradition × Cénacle) ·
frontière 3 voix : le Cénacle CULTIVE, les Cartes vont CHERCHER, le Pacte
ACHÈTE — le Dessein DATE les étapes ; zéro clé commune vérifiée.

---

## Branche de la FOI

**Identité :** la seule branche où l'objet conquis n'est pas une terre mais
une âme. Sa monnaie propre est la DISTANCE culturelle (chaque conversion
rapproche un axe que le moteur lit partout). Et la seule branche qui
récompense d'avoir manqué son but : un schisme, sur la voie du dehors, est
une descendance.

**Éligibilité (CORRIGE la cascade) :** ⚠ ne PAS keyer sur `w_faith` (dérivé
du crédo, GLISSE chaque tick avec les factions, et un Pluraliste est
« inatteignable à vie » — commentaire du moteur). Lire la fiche culture de la
capitale : `credo != PLURALISTE` OU `culture.religion ≥ FOI_ELIG_AXIS (6.0)`
— environ la moitié des empires d'un monde. Départage : Purificateur ⇒ FOI
gagne sur tout ; Évangéliste ⇒ FOI sauf si héritage Éso/Méca (Savoir devant).

| # | Nom FR (EN) | Cible (résolution) | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **L'autel de pierre** (*The Stone Altar*) | ma province à `build.faith` max (tie pid↓) — LE BERCEAU | Temple bâti sur CE pid | *NOUVEAU TUNABLE `BUILD_COST_MULT_FAITH` déf. 1.0* ×0.85 daté 20 ans sur la famille Sanctuaire→Temple→Cathédrale (miroir devis+gate+légal) | Le dieu qu'on prie sous un toit de branches n'a pas encore de nom. |
| 2 | **Le Nom du dieu** (*The Naming of the God*) | le berceau | foi FONDÉE par moi (`founder_cid==cid` — sous le plafond ⌈N/2⌉, `scps_religion_found` RALLIE en silence : voir re-résolution) | **Ministre « le Premier Lettré »** au siège Savoir (rang II — le siège que `mission_responsible_seat` route déjà pour la foi) + Annale | On a écrit le nom, et le nom s'est mis à demander des choses. |
| 3 | **Le premier troupeau** (*The First Flock*) | BFS depuis le berceau : mes 3 provinces les plus proches ne professant pas la foi | chacune professe ma foi — via **NOUVEAU LECTEUR PUR `religion_of_province(econ,pid)`** (voir piège n°1) | **Provmod « Lieu saint »** sur le berceau : bit stocké, `+LIEU_SAINT_FAITH (1.5)` de foi APRÈS la vétusté (une pierre consacrée ne rouille pas) + chip `PMOD_LIEU_SAINT` (`demo_bonus` 0.08 — les pèlerins) | Trois villages qui chantent la même chose un dimanche : c'est là qu'une foi cesse d'être une opinion. |
| 4 | **Le sermon aux marches** (*The Sermon at the Marches*) | BFS hors frontières : la province PAÏENNE la plus proche (priorité hameau WILD, puis vassal, puis voisin) | elle professe ma foi (lettré OU héritage à la conquête/vassalisation — double voie) | *promotion `FAITH_BRANCH_PEN` (3.5, `scps_econ.c:763`)* ×0.80 daté 20 ans — le converti d'une autre branche cesse d'être un étranger. Arme le PIVOT | Le prêcheur est revenu avec de la boue jusqu'aux genoux et un village de plus. |
| — | **PIVOT** (20 infl. · **0 si le courant Divin est tenu** — §4.5) | — | échelon 4 scellé | irréversible | — |
| 5A | **Le pays d'une seule prière** (*A Realm of One Prayer*) | toutes mes provinces | ≥ 80 % de la POP du pays professe la foi (somme PAR PROVINCE sur les groupes, jamais le lecteur rep-province) | *promotion `SCHISM_FLIP_L` (4.0)* ×0.75 daté 25 ans — une marche doit être bien plus illégitime avant de dériver | Il n'y a plus qu'un calendrier, une fête, un serment. |
| 6A | **La Contre-Réforme** (*The Counter-Reformation*) | la province la plus dérivante | le signal de schisme s'est armé au moins une fois ET la province tient encore la foi 60 clôtures plus tard (latch sérialisé) | **Déverrouillage permanent** : la famille de foi lit `tier−1` au gate de tech (rime revendiquée avec Défense idée 6, autre famille, autre prix) | Le prêche a tenu là où la couronne n'aurait pas tenu. |
| 7A | **La Cathédrale** — parachèvement | le berceau | Cathédrale bâtie + les 80 % tenus | **Coordonnée bâtie** (+1.0 foi, +0.5 K_inst — « la Chaire ») · Lieu saint promu « Siège de la foi » · Annale MONUMENT + épithète « l'Oint » + slot de doctrine en avance | Le pays entier lève la tête vers la même pierre. |
| 5B | **La foi du voisin** (*The Neighbour's Faith*) | le pays le plus lié commercialement d'une AUTRE racine ; chez lui, la province de plus forte pop hors ma foi | elle professe ma foi (lettré déployé chez l'autre — ⚠ le lettré n'a AUJOURD'HUI aucun gate d'appartenance : à borner avant de livrer) | **Claim nommé « Le droit de prêche »** : `fab_region` + `CB_RELIGIOUS` posé MÛR et gratuit (fenêtre 5 ans normale) | On a demandé un temple ; on nous a donné une raison. |
| 6B | **Le schisme essaimé** (*The Schism Sown*) | ma racine et sa descendance | une foi ENFANT de ma racine professée hors de mes frontières (provoquée OU échappée — double voie) | `RELIG_SCHISM_MAX` +2 POUR MA RACINE SEULE (5→7) · *promotion `SCHISM_FLIP_D` (5.0)* ×0.85 daté 25 ans | Ce qui se brise en deux ne meurt pas : ça se met à marcher sur deux jambes. |
| 7B | **Le Concile** — parachèvement | le berceau + les couronnes de ma racine | ≥ 3 pays étrangers de ma racine ET Cathédrale au berceau | **Règle permanente « La Communion des couronnes »** : `FAITH_BRANCH_PEN` ×0.60 envers les seuls pays de ma racine · +1.0 savoir/+0.5 K bâtis (le concile est une école) · Annale + épithète « le Pontife » + slot de doctrine | On n'a pas conquis trois royaumes : on leur a donné le même dimanche. |

**Notes (agent) — pièges vérifiés :**
1. **⚠ LE piège : la religion ne voit qu'UNE province par région** — tous les
   sites (refresh, set, inherit, scholar) passent par la rep-province.
   « N converties » serait satisfait par une seule province sur dix. TOUT
   prédicat de la branche compte par pid via un **lecteur pur neuf
   `religion_of_province`** (même tally, sur `prov[pid].pop`) ; et le verbe
   missionnaire devra à terme poser la foi PAR PROVINCE (on transfère le
   chemin, on ne le contourne pas). `religion_of_region` reste l'API
   d'affichage.
2. `econ_content_dist_faith(a,b)` n'a pas de cid ⇒ mult national sur
   `FAITH_BRANCH_PEN` = variante `_ex` + threading (~10 sites) ; repli acté :
   n'appliquer qu'au site légitimité, et le dire en hover.
3. Berceau perdu ⇒ re-résolution ; le Lieu saint PART AVEC LA TERRE (le bon
   récit) · voisin rallié à ma racine avant conversion ⇒ 5B SE SCELLE et le
   claim tombe (pas de CB contre un cousin de foi) · plafond ⌈N/2⌉ atteint ⇒
   l'échelon 2 devient « Le Nom qu'on reprend » (rallier suffit) mais la voie
   B perd son parachèvement plein — alternative plus simple à trancher :
   générer FOI seulement si `religion_can_found` à la genèse.
4. Composition `FAITH_BRANCH_PEN` : échelon 4 (×0.80 daté) puis 7B (×0.60
   permanent, portée racine) = ×0.48 pendant la fenêtre ⇒ clamp H2bis, ou le
   permanent REMPLACE le daté (recommandé).
5. Le Conseil n'a que 3 sièges (Savoir/Royaume/Ouvrages) — pas de siège de la
   Foi : le ministre s'assied au Savoir · `RELIG_MINORITY_SAT` n'existe pas
   encore (proposé par le courant Divin) : ne rien câbler dessus d'ici ·
   `PMOD_LIEU_SAINT` : enum appendu + macro partagée · zéro recouvrement de
   clé avec le Divin, vérifié une à une (fondation vs schisme : deux plafonds,
   deux mécanismes) · 7B ne touche pas `diplo_cb_needs_fabrication` (doublon
   exact de « La Guerre sainte ») : la branche DÉSIGNE l'hérétique, la
   synergie ARME l'épée.

---

## Branche de la HORDE

**Identité :** un pays qui ne produit pas sa richesse : il la PREND. Une
machine à états du pays (bande armée → puissance prédatrice → État) — chaque
échelon est un cran de la transformation. Tout le butin est un TRANSFERT réel
(pillage, tribut, réparations) : zéro canal qui crédite ex nihilo. Sa moitié
du sac est celle qu'Offense ne prend pas : les ÂMES (`SLAVE_*`) et le TRIBUT
(`REPARATIONS_*`).

**Éligibilité :** héritage CLANIQUE + éthos Dominateur/Honneur + **∃ ≥ 1
voisin non-WILD** (toutes les cibles sont des voisins — un insulaire sans
voisin recevrait un arbre injouable ; repli : Foi si w_faith haut, sinon
Creuset). Rime assumée : l'éthos Dominateur porte déjà l'épithète de pays
« Horde » — le pays éligible s'appelle souvent littéralement *Horde de X*
(décision : garder, le nom nu réservé au pivot A).

**Échelons 1-4 communs · pivot (20 influence) · voies de 3.**

| # | Nom FR (EN) | Cible (résolution) | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **La première proie** (*The First Prey*) | PROIE = voisin au ratio de force ≥ `HORDE_PREY_RATIO` (1.15) de plus forte `diplo_country_value`, tie cid↑ ; aucun candidat ⇒ le filtre tombe (la cible se résout TOUJOURS) | `horde_sacks[moi][PROIE] ≥ 1` (sac de siège, occupation-capture OU razzia côtière) — compteur sérialisé NEUF | **Revendication nommée** sur la région saccagée (`diplo_claim_region` SANS les 2 ans de revenu de fabrication) | On ne choisit pas la première proie : on choisit la plus grasse qu'on puisse rattraper. |
| 2 | **Le butin nourrit le camp** (*The Camp Fed on Spoils*) | ma capitale | butin+tribut sur 10 ans glissants ≥ 0.5 × mon revenu annuel (accumulateur sérialisé) | `SLAVE_FRACTION` ×1.30, **20 ans** — le raid ramène plus de bras, jamais plus d'or | Les chariots rentrent chargés ; le camp qui les décharge est déjà une ville. |
| 3 | **Le tribut imposé** (*The Tribute Imposed*) | T₁ = la PROIE si elle vit et ne paie personne, sinon re-résolue en EXCLUANT tout `t` déjà tributaire d'un autre (`reparations_to[]` est MONO-PAYEUR — contrainte moteur) | `reparations_to[T₁]==moi` avec jours > 0 (obtenu via `PEACE_REPARATIONS`) | **Remise datée 25 ans** sur 2 promotions de littéraux : *`REPARATIONS_RATE` (0.10, `scps_diplo.c:1325`)* ×1.40 · *`REPARATIONS_DAYS` (3650, `:1317`)* ×1.50 → 15 ans | Il a signé pour dix ans. Nous comptons en générations. |
| 4 | **La terreur établie** (*Terror Established*) — **LE VERBE** | LE CERCLE = les voisins de mes tributaires (ensemble nommé) | ≥ 2 tributaires simultanés ET ≥ 4 sacs cumulés | `CMD_DEMAND_TRIBUTE(cible)` — exiger le tribut SANS guerre : accepté ssi ratio de force ≥ `HORDE_DEMAND_RATIO` (2.0), cible sans maître, aucun allié ne couvre le ratio (la PEUR, pas `ai_consider_offer`) ; 15 influence + CD 5 ans ; effet = `diplo_peace_start_reparations` (canal EXISTANT) | On n'a pas eu besoin d'entrer. Le nom a suffi, et les chariots sont venus à nous. |
| 5 | **PIVOT** — Horde éternelle vs Sédentarisation | — | échelon 4 scellé | 20 influence, irréversible | Le camp doit décider s'il est une saison ou un siècle. |

**Voie A — La Horde éternelle (rester prédateur)**

| # | Nom | Condition | Récompense | Saveur |
|---|---|---|---|---|
| A6 | **L'économie de horde** (*The Horde Economy*) | butin+tribut ≥ 25 % de mon revenu, 12 clôtures consécutives | mults datés 25 ans sur le marché servile (canal LIBRE) : `SLAVE_PRICE` ×0.80 · `SLAVE_POOL_REF` ×1.40 · `SLAVE_AI_KEEP_FRAC` ×1.60 (ramené en bande) | Le trésor du royaume tient dans des chariots, et il roule. |
| A7 | **Les tributs perpétuels** (*Tributes Without End*) | ≥ 3 tributaires tenus ≥ 5 ans | **RÈGLE permanente** : tant que le ratio de force ≥ 2.0, `reparations_days[T]` se RENOUVELLE à la clôture (borné à sa valeur de départ) ; le ratio tombe ⇒ le décompte reprend où il en était | Le traité expire. La peur, non. |
| A8 | **La Horde éternelle** — parachèvement | butin cumulé ≥ 3 ans de revenu ET ≥ 2000 âmes déportées vivantes | **Provmod permanent** « La Halle du butin » (`PMOD_HALLE_BUTIN` appendu) sur la capitale + Annale + épithète « le Fléau ». AUCUN frein éteint : `SLAVE_REVOLT_SHARE` (20 %) est dépassé bien avant — le prix est structurel et déjà simulé | Cent ans que nous n'avons rien semé. Cent ans que nous n'avons jamais eu faim. |

**Voie B — La Sédentarisation (la machine à états Anbennar : chaque échelon
change une règle et convertit un acquis prédateur en institution)**

| # | Nom | Condition | Récompense | Saveur |
|---|---|---|---|---|
| B6 | **La ville des chariots** (*The City of Wagons*) | ≥ 3 édifices civils en capitale ET aucun sac depuis 5 ans (latch sérialisé) | **RÈGLE-PRIX** : `CMD_RAID_COAST` refusé au drain, DÉFINITIVEMENT (miroir checklist obligatoire) ; en échange : +K_inst/+PE_infra bâtis sur la capitale + `ADMIN_BASE` ×0.85 daté 25 ans | On plante les piquets une dernière fois, et cette fois on bâtit dessus. |
| B7 | **La caste des guerriers** (*The Warrior Caste*) | ≥ 1 province arrachée par traité ET part d'élite ≥ 6 % (la bande est devenue une classe — CONDITION lue, jamais subventionnée) | une **Garnison ouverte à la file** sur 4 provinces frontalières nommées (or/matière débités normalement) + RÈGLE une-fois : sur ces 4 provinces, servile→journalier (la bascule exacte de `demography_manumit_country`, portée PROVINCE) | Ceux qui pillaient gardent les murs ; ceux qu'on avait pris labourent derrière eux. |
| B8 | **Le Royaume né du camp** — parachèvement | ≥ 2 tributaires tenus ET un édifice de 960 j en capitale | **RÈGLE une-fois** : chaque tributaire devient VASSAL en Concordat (`diplo_set_vassal`) et son tribut S'ÉTEINT — conversion de canal, jamais création (la rente cesse, la contribution vassale M3f commence) + provmod capital + Annale + épithète « le Législateur » | Ils ne paient plus : ils jurent. C'est plus cher pour eux, et moins cher pour nous. |

**Notes (agent) — dont un PRÉREQUIS DE CHANTIER :**
1. **AUCUN compteur de sacs exploitable n'existe** : `g_pil_*` sont des
   statics sans cid, RAZ par sim, non sérialisés (télémétrie) ; le Fil est un
   anneau 64 write-only qui OUBLIE. La branche exige des accumulateurs
   sérialisés neufs (`horde_sacks`, `horde_loot` fenêtre 10 ans,
   `horde_last_sack_day`, `horde_eco_streak`) ⇒ bump + savetest.
2. `reparations_to[]` mono-payeur : les résolveurs 3/4 EXCLUENT les déjà-
   tributaires d'autrui ; `polity_death` nettoie ⇒ re-résolution au tick.
3. `PILLAGE_COOLDOWN_Y` et `raid_cd_days` INTOUCHÉS (le tempo vient du NOMBRE
   de proies, jamais de la fréquence — c'est l'arc voulu : la horde se
   déplace).
4. Golden P1 par construction SAUF `CMD_DEMAND_TRIBUTE` (acte joueur→monde
   lu par l'IA — admissible, mais sweep an 180 : revenu médian des IA,
   tributs simultanés, trésor des voisins directs).
5. `PMOD_HALLE_BUTIN` : enum appendu + macro `ECON_PROVMOD_BODY` partagée
   (le cas DEDANS).
6. Frontières prouvées par table (zéro clé commune) : Offense = la valeur du
   sac / la Horde = les âmes et le tribut ; Vassaux idée 3 = verbe répétable /
   B8 = conversion unique payée par l'extinction du tribut ; B7 lit la part
   d'élite, ne la subventionne pas ; le helper de bascule servile→journalier
   est UNIQUE, deux appelants (pacte national Peuple · scellage provincial B7).
7. Un pays devenu mon allié/vassal sort du vivier de proies (la branche ne
   demande jamais de trahir un serment) ; un échelon scellé fige ses noms
   (l'Annale reste lisible même si la proie a disparu).

---

## Branche du SOL

**Identité :** la terre d'abord : rassembler ce qui est à soi, tenir ce qui
borde, puis peser sur tout un continent — et choisir à mi-parcours si l'on
tient le sol par l'épée ou par le serment.

**Éligibilité : TOUJOURS.** Propriété golden forte : la branche NE TIRE AUCUN
`xs32` — toutes ses cibles dérivent de la seule géographie (BFS `prov_adj`,
`diplo_province_price`, `rancor`) : le flux rng est intact même générée pour
tous. Grain : cible = pid, condition = `prov[pid].owner` ; seule la
REVENDICATION est région-grain (le seul grain que le moteur donne à un claim).

**Échelons communs :**

| # | Nom FR (EN) | Cible | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **Le Ban du sol** (*The Ban of the Soil*) | T1 = toutes les provinces actives de la région-capitale (figé à la genèse) | toutes possédées ET colonisées | **Coordonnée bâtie permanente** : `K_inst +0.6` sur la capitale (motif `dir_region_food_cap_add`) | On a fini de compter les hameaux : chaque feu de la vallée répond au même ban. |
| 2 | **La Première marche** (*The First March*) | T2 = la plus proche province étrangère/vierge (BFS ; sauts↑, habitabilité↓, pid↑ ; figée au scellage de 1) | `owner==cid` par N'IMPORTE QUELLE voie (colonisation, conquête, digestion, héritage) | **Revendication nommée GRATUITE** sur la région de T3 (`fab_state=READY`, coût d'or sauté, fenêtre normale) + `reconstruction=1.0` sur T2 (~10 ans, décroissance native) | La borne est déplacée d'un cran. Ce n'est rien ; c'est la première fois. |
| 3 | **Le Voisin nommé** (*The Named Neighbour*) | RIVAL = argmax `rancor[cid][b]` (repli : le plus proche) ; T3 = sa province adjacente de plus forte valeur (figés au scellage de 2) | `owner(T3)==cid` OU le RIVAL est mon vassal — l'épée OU le serment (la répétition générale du pivot, sans le forcer) | **Remise datée 20 ans** : `FAB_VALID_DAYS` ×1.60 (5→8 ans — « Les torts consignés ») + Annale | Il a désormais un nom, et ce nom est écrit dans un registre qui ne se ferme plus. |

**PIVOT « Le Choix du sol »** — décision d'agent PROPOSÉE : **l'éthos fixe le
PRIX, pas le mur** (§2.5 disait « gate d'éthos », mais la branche est
toujours générée et Ordre/Mercantile seraient sans pivot) ; le vrai gate est
une PREUVE D'USAGE :
- **Voie A — L'Empire des marches** : condition ≥ 1 province arrachée par
  traité ; 15 influence si Dominateur/Honneur, 25 sinon.
- **Voie B — La Couronne des serments** (⚠ renommée : « La Toile des
  serments » est DÉJÀ la doctrine Vassaux — collision H3.4-like tranchée ;
  l'échelon 5b garde « La Toile » comme titre local) : condition ≥ 1 vassal ;
  15 si Bureaucrate/Pacifiste, 25 sinon.
Aucune récompense propre : le choix EST la récompense.

**Voie A — L'Empire des marches :**

| # | Nom | Cible | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 4a | **La Cicatrice qu'on referme** (*The Scar We Close*) | T3 si à moi, sinon la province du RIVAL passée à moi | possédée ET `annex_scar < 0.05` (la plaie refermée, pas le drapeau planté) | `ANNEX_SOFT_SCAR` ×0.75 daté 20 ans + reconstruction sur T4a | Prendre est l'affaire d'un été ; faire oublier qu'on a pris est l'affaire d'un règne. |
| 5a | **Les Trois marches** (*The Three Marches*) | 3 provinces étrangères adjacentes de plus forte valeur | les 3 possédées | claim gratuit sur la région de T6a + `AI_ANNEX_YEARS_PER_PRICE` ×0.80 daté 20 ans | Une marche est un accident ; trois marches sont une frontière. |
| 6a | **Le Siège abattu** (*The Fallen Seat*) | la capitale du RIVAL | PROPRIÉTÉ (l'occupation ne suffit pas : le siège n'est pas la paix) | **embauche GRATUITE du meilleur candidat au siège Royaume** (sans coût ni grief — précédent `EVID_CONSEIL_A2`) + Annale | Sa couronne est dans ta chapelle et son chancelier siège à ta table. |
| 7a | **L'Empire des marches** — parachèvement | le continent de la capitale | ≥ `DESSEIN_SOL_HEGEMON_FRAC` (0.40) des provinces actives du continent possédées | **« La Porte du sol »** : `K_inst +1.0` ET `H_coerc +1.0` bâtis sur la capitale + Annale + épithète + slot de doctrine en avance | On ne dit plus « le royaume » et « le continent » : on dit le même mot deux fois. |

**Voie B — La Couronne des serments :**

| # | Nom | Cible | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 4b | **Le Premier serment** (*The First Oath*) | V1 = le RIVAL (s'il vit), sinon le pays indépendant le plus proche | mon vassal, tout contrat | `OPINION_VASSAL` ×1.30 daté 20 ans (« Le crédit du serment » — la SEULE clé OPINION_* qu'aucune doctrine ne touche) | Un homme a plié le genou, et trois autres ont regardé. |
| 5b | **La Toile** (*The Web*) | les 3 pays indépendants les plus proches (V1 inclus) | les 3 mes vassaux | `AI_ANNEX_MIN_INTEGRATION` ×0.85 daté 20 ans + Annale | Trois fils tendus ne font pas trois liens : ils font une surface. |
| 6b | **Le Vassal qui n'est plus un autre** (*The Vassal Made Kin*) | celui de la Toile au `v_integration` max (ré-évalué chaque clôture — la cible est CELUI QUI MÛRIT) | `v_integration ≥ 1.0` SANS exiger l'annexion | ministre gratuit (l'héritier du vassal entre au Conseil) + Annale | Son fils parle notre langue sans accent, et personne ne se souvient de la date du serment. |
| 7b | **La Couronne des serments** — parachèvement | le continent | même seuil 0.40, comptabilité : possédé OU vassal de moi | **« La Chambre des serments »** : `K_inst +2.0` bâti (AUCUN H_coerc : la Toile ne tient rien par la garnison) + Annale + épithète + slot de doctrine | Tu ne possèdes pas le continent. Il te répond, ce qui coûte moins cher. |

**Notes (agent) — l'ossature TECHNIQUE de toutes les branches :**
- **N1 Re-résolution** : l'invalidation est la DESTRUCTION, jamais
  l'inconvénient — une province passée à un allié/tiers NE se re-résout PAS
  (la cible est la terre, pas le drapeau : il faudra la prendre) ; engloutie
  (`!active`) ⇒ règle d'origine sur le monde courant ; pays mort ⇒ re-tirage
  déterministe, le RIVAL remplacé ne revient jamais (latch) ; aucune cible
  atteignable ⇒ l'échelon est « en attente », re-tenté chaque clôture (une
  branche ne se bloque jamais, elle patiente) ; JAMAIS de résolution pendant
  le chargement (`prov_adj` = pointeur tas rebâti — attendre la première
  clôture post-load).
- **N2 LE CANAL DATÉ N'EXISTE PAS — le créer au plus simple** : pas
  d'accumulateur, un **latch d'ANNÉE de scellage** (motif `year_eligible[]`) :
  `dessein_mult = m si (year − sealed_year) < dur_years sinon 1.0` — zéro
  tick, sérialisation triviale. Site : `tune_f × decree_mult × doctrine_mult
  × dessein_mult`, clamp H2bis [0.60, 1.60] ÉTENDU aux Desseins.
  ⚠ `Modifier.expires_tick` est déclaré mais NON APPLIQUÉ — ne pas s'y fier.
- **N3 Save** : `MissionsState` en blob brut ⇒ tout ajout bump ; champs :
  rung, voie, targets[], rival, `sealed_year[]` ; `save_sane` borne tout.
- **N4 Scellage** : `CMD_SEAL_DESSEIN(branche, échelon)` copié de
  `CMD_AGE_ENGAGE` (le patron exact, sémantiquement vide).
- **N6 Pièges vérifiés** : `fab_state[a][b]` = slot UNIQUE par paire — la
  revendication de Dessein ne s'y pose que si FAB_NONE, sinon RETENUE jusqu'à
  libération (jamais écraser une intrigue payée) · `occupier`/`conquered` =
  région-grain, la guerre transfère la RÉGION entière (le libellé dit « la
  marche de {ville} ») · toponymes = région-grain (une province se nomme par
  sa ville) · AUCUN poseur de modificateur nommé n'existe : on écrit les
  CHAMPS SOURCES (`reconstruction=1.0`) — un `PMOD_DESSEIN` nommé coûte
  enum+champ+branche, à ne payer que pour l'icône · le Conseil = 3 sièges
  (Savoir/Royaume/Ouvrages), pas de siège de la Guerre, rang I-III DÉRIVÉ du
  seed jamais stocké ⇒ « ministre au rang fixé » impossible tel quel — la
  récompense réelle = embauche gratuite du meilleur candidat ·
  `statecraft_council_hire` lève DÉJÀ la faction (ne pas doubler le hook) ·
  `annal_push` peut renvoyer −1 (ne jamais en dépendre) · `MIS_COORD_*`
  n'expose que 6 des 8 deltas (rester sur K_inst/H_coerc) · ne JAMAIS toucher
  `edi_built` pour un delta permanent.
- **N7 Correction au doc §2.2** : « rallier le hameau WILD (défection
  pacifique OU conquête) » est PÉRIMÉ — `WILD_DEFECT_YEARS=0` (décision
  joueur H4.3) : tout échelon visant un WILD est conquête ou vassalité, point.
- **N9 Frontières prouvées par table** : prétexte (Offense = coût/vitesse,
  Sol = DURÉE de fenêtre — Offense dit « VALID_DAYS inchangé ») · cicatrice
  (la règle de la synergie DOMINE le mult daté — aucun double-dip) · annexion
  (permission/barre/durée = 3 clés distinctes) · intégration & tribut =
  abstention explicite (clés déjà tenues ou en collision) · gabarits texte en
  `{0}`/`{1}` (motif maison), pas `%s`.
- **N10 Tunables neufs : 2 seulement** — `DESSEIN_SOL_HEGEMON_FRAC` (0.40) ·
  `DESSEIN_BOON_YEARS` (20) ; les deux à neutre + rien scellé = byte-identique.

---

## Branche ROUTES & CARAVANES

**Identité :** le pays sans mer ne se plaint pas de ce qu'il n'a pas : il
fait de son enclavement une position. Sa richesse vient d'ÊTRE TRAVERSÉ. Il
commence en autarcie — le prix du sel y est une rumeur — et finit en étape
obligée entre deux mondes qui commercent sans s'apercevoir qu'ils paient sa
cour. Les 4 premiers échelons sont un geste unique à quatre échelles :
RELIER. Le pivot demande ce qu'on fait d'une porte : la tenir ou l'ouvrir.

**Éligibilité :** `!province[capital_prov].coastal` — le complément EXACT de
Mer & Comptoirs (tout jouable reçoit une branche d'ouverture et une seule).

| # | Nom FR (EN) | Cible (résolution) | Condition | Récompense | Saveur |
|---|---|---|---|---|---|
| 1 | **La halte** (*The Way-Station*) | ma province la plus ENCLAVÉE (max distance de hub, l'autarcie l'emportant ; tie pid↑) | possédée + Caravansérail bâti | **Coordonnée bâtie** `P_open +0.4` + provmod « La Halte » | Une route ne se décrète pas : on bâtit un toit, un puits, une écurie — et le chemin se fait tout seul autour. |
| 2 | **Au bout du bassin** (*The Reach of the Market*) | le Centre le plus proche de ma capitale — le pid qui PORTE `EDI_TRADE_CENTER` (jamais la rep-province), latché | AUCUNE de mes provinces en autarcie (toutes rattachées à un hub) | `COMMERCE_BLD_PER` ×1.30 daté 20 ans + claim nommé sur la région du Centre si à autrui | Le jour où le prix du sel cesse d'être une rumeur, le royaume découvre qu'il vivait à côté du monde. |
| 3 | **Les gens de la piste** (*The People of the Track*) | le hameau libre le plus proche (BFS) | conquête OU vassalité IMPOSÉE à la paix (un WILD ne se rallie ni ne se soumet jamais seul — décision joueur) | `MIG_PULL_MAX` ×1.25 daté 15 ans + provmod « Le Relais » (`PE_infra +0.5`) — un WILD est DÉMONÉTISÉ : jamais d'or sur cet échelon | Ils connaissaient les cols avant nous. Maintenant ils les connaissent pour nous. |
| 4 | **Le col** (*The Pass*) | la province de col la plus proche (biome Highlands/Mountains — la toponymie la nomme déjà « Col/Passe/Porte de… », aucun gabarit neuf) | possédée + Garnison bâtie | `H_coerc +0.5` bâti + provmod « La Porte » + claim sur la 1re région étrangère adjacente au col. Ouvre le PIVOT. Pas de péage moteur au col (système neuf — hors périmètre) : la récompense est la coordonnée + la revendication | Qui tient la porte n'a plus besoin de tenir la route. |
| — | **PIVOT** (20 infl., irréversible) | — | échelon 4 scellé | Maître des étapes vs Libre Passage | — |
| A5 | **L'entrepôt du carrefour** (*The Crossroads Warehouse*) | ma province-carrefour : max degré d'adjacence (`region_flow_score` PRIVÉ de son +2.5 côtier — le pays est continental) | Entrepôt ET Marché bâtis dessus | **Provmod permanent « Le Carrefour »** (`PE_infra +1.0`) + claim sur la région du Centre latché | Six routes se croisent ici depuis toujours ; il ne manquait qu'un toit et des balances. |
| A6 | **Le droit de passage** (*The Right of Way*) — **VERBE** | le Centre latché en 2 | `owner==moi` (conquête, cession, ou mon Centre devenu le plus proche) | `CMD_CENTRE_RELOCATE(pid)` — déplacer un Centre que je tiens vers une de mes provinces à Entrepôt (primitive `intertrade_relocate_centre` existante, recette Centre 10/30/15, 540 j) + `ADMIN_EXP` ×0.93 daté 25 ans | Le marché n'est pas où les marchands vont : il est où le roi décide qu'ils s'arrêtent. |
| A7 | **L'Étape du monde** (*Where the World Halts*) — parachèvement | ma province-Centre | j'ai un Centre ET il porte le plus fort trafic réel du monde (`intertrade_centre_value`, MOYENNE GLISSANTE 12 clôtures — la valeur brute est un dernier-tick volatil) | **Provmod permanent « L'Étape »** (`PE_infra +1.5` · `K_inst +0.5`) + Annale + épithète + slot de doctrine en avance | Deux mondes commercent, et aucun des deux ne s'aperçoit qu'il paie ma cour. |
| B5 | **Le sauf-conduit** (*The Safe-Conduct*) | le voisin terrestre le plus commerçant | pacte commercial ET ≥ 1 route terrestre ouverte avec lui | **RÈGLE (le verbe de la voie)** : sous pacte, le bassin de marché TRAVERSE la frontière (`hub_map_build` franchit l'adjacence entre pays liés) + provmod « Le Sauf-conduit » (`P_open +0.5` frontalier) | Un sceau sur un parchemin vaut trois garnisons sur un col. |
| B6 | **Les marchés sans maître** (*The Masterless Markets*) | les Centres sans propriétaire atteignables | ≥ 3 pactes simultanés ET aucun embargo décrété par moi | `IMPORT_MARGIN_NONE` ×0.85 daté 20 ans (la SEULE marge qu'aucune doctrine ne touche) + provmod « Le Franc-passage » | Nos caravaniers savent le prix de tout, partout, et rentrent avant les autres. |
| B7 | **Le Grand Chemin** (*The Great Road*) — parachèvement | ma capitale | accès global + ≥ 5 routes actives + aucun embargo décrété depuis 10 ans | **Provmod permanent « Le Grand Chemin »** (`PE_infra +1.0` · `K_inst +0.5`) + Annale + épithète + slot de doctrine | Nous n'avons pas de port. Nous sommes le port. |

**Notes (agent) — trouvailles vérifiées :**
1. **Le réseau routier PEINT n'est pas du moteur** (`api_roads_build` = cache
   façade A*+MST, jamais lu par la sim, jamais sérialisé) — aucune condition
   ne peut le lire ; le substrat moteur est `RouteNetwork` (région-grain,
   plafond MONDIAL 256 routes : « ouvrir plus de routes » n'est pas une
   récompense).
2. **Deux définitions du bassin coexistent** : moteur `hub_map_build` (source
   = Centre SEUL) vs façade `market_hub_regions` (Marché|Comptoir|Centre). Le
   Dessein lit la MOTEUR (elle fixe la marge) — et le mode carte
   « Marché-proximité » est à réaligner ou à renommer (décision).
3. **`CMD_ROUTE` est un chemin joueur RÉGION-grain existant** (contraire à la
   doctrine province) — le prérequis honnête de la vague est de le TRANSFÉRER
   sur pid, pas de le contourner.
4. `region[].edi_built` = agrégat OR reconstruit chaque tick (poser sur
   `prov[].edi_built` + miroir — jurisprudence `intertrade_seed_centres`) ·
   rallier un hameau = `econ_region_set_owner` (l'affectation directe avait
   produit ~23 ralliements/hameau — commentaire `scps_sim.c:112`).
5. Golden : DEUX exceptions à gater strictement joueur (le verbe
   `CMD_CENTRE_RELOCATE` salit `g_centre[]` mondial ; la règle B5 modifie
   `hub_map_build`) — sinon re-baseline immédiat.
6. Collisions de noms tranchées par l'agent : « Les Portes ouvertes » (pivot
   proposé) = idée 3 du Creuset ⇒ « **Le Libre Passage** » ; « Le Relais
   franc » frôlait « le comptoir franc » ⇒ « Le Relais ».
7. Clés propres (aucune ailleurs au catalogue) : `COMMERCE_BLD_PER` ·
   `MIG_PULL_MAX` · `ADMIN_EXP` · `IMPORT_MARGIN_NONE` + coordonnées bâties
   et revendications nommées (la signature du système Desseins).

---

# HARMONISATION DESSEINS (passe d'orchestrateur, clôture du design)

## D1. Mécanismes communs adoptés (issus des meilleures notes de branches)

1. **Le canal DATÉ** (toutes branches) : il n'existe pas dans le moteur —
   adopté le plus simple qui marche (Sol N2) : **latch d'ANNÉE de scellage**
   (`sealed_year[]`, motif `year_eligible[]`) ;
   `dessein_mult = m si (year − sealed_year) < durée sinon 1.0`. Zéro tick,
   sérialisation triviale. ⚠ `Modifier.expires_tick` est déclaré mais NON
   APPLIQUÉ — interdit de s'y fier.
2. **Site de lecture unifié** : `tune_f × decree_mult × doctrine_mult ×
   dessein_mult`, **clamp H2bis [0.60, 1.60] par clé étendu aux Desseins**.
3. **Gabarits de texte : `{0}`/`{1}`** (la convention maison vérifiée —
   `STR_*_FMT` existants), PAS `%s` : les branches écrites en `%s`
   (Mer/Horde/Savoir/Foi/Creuset) se normalisent à l'implémentation.
4. **Scellage** : `CMD_SEAL_DESSEIN(branche, échelon)` copié de
   `CMD_AGE_ENGAGE` (accusé de réception pur) ; enum `CMD_*` appendu en fin +
   grep des boucles.
5. **Re-résolution** (règle unifiée, Sol N1 + Mer) : l'invalidation est la
   DESTRUCTION, jamais l'inconvénient (une cible passée à un allié reste la
   cible) ; cible détruite ⇒ même règle sur le monde courant ; objectif
   atteint par un autre chemin ⇒ l'échelon SE SCELLE ; aucune cible ⇒
   « en attente », re-tenté chaque clôture ; chaque bascule émet une ligne du
   Fil ; jamais de résolution pendant le chargement (`prov_adj` = tas rebâti).
6. **Pivots** : la règle du Sol généralisée — **l'éthos/le contexte fixe le
   PRIX (15/20/25), jamais le mur** ; le vrai gate est une preuve d'usage.
   Motif §4.5 : le pivot est GRATUIT ou réduit si la doctrine sœur est tenue
   (acté pour Foi/Divin ; généralisable à Mer/Colonisation,
   Routes/Commerce-Mercantilisme, Horde/Offense — au calibrage).
7. **Provmods nommés** : deux mécaniques au choix par branche — champ SOURCE
   existant (reconstruction/ferveur, gratuit) ou `PMOD_*` appendu + latch
   sérialisé (coûte enum+champ+bump — à payer quand on veut l'icône). Les
   parachèvements passent tous par `ProvBuild` (coordonnées bâties).
8. **Parachèvements** : tous donnent Annale + épithète candidate + **un slot
   de doctrine ouvert en avance** (§4.5 — la spécialisation débloque la
   spécialisation).

## D2. Corrections au doc principal (appliquées)

- §2.2 : la double voie du hameau WILD est « conquête OU vassalité imposée »
  (la « défection pacifique » est périmée — `WILD_DEFECT_YEARS=0`, décision
  joueur H4.3).
- §2.5 : la voie B du pivot du Sol est renommée « **La Couronne des
  serments** » (« La Toile des serments » est la doctrine Vassaux — collision
  d'écrans voisins ; l'échelon 5b garde « La Toile » comme titre local).
- La cascade d'éligibilité FOI est celle de l'agent Foi (fiche culture :
  crédo ≠ Pluraliste OU axe religion ≥ 6.0 — jamais `w_faith`, qui glisse) ;
  celle du SAVOIR lit le SOCLE `w_base[2]`, jamais le courant.

## D3. Trouvailles moteur de la passe (tickets/chantiers à trancher)

1. **⚠ La porte de l'arcane est MURÉE** : `has_ruins_access` n'est jamais
   posé à true en partie réelle — Invocation/Éveil/Savoir interdit
   inatteignables aujourd'hui. « La Porte entrouverte » (Savoir 5B) est le
   porteur légitime de la clef — mais DÉCISION JOUEUR d'abord : oubli ou mise
   en sommeil délibérée ?
2. **La religion ne compte qu'une province par région** (tous les sites
   passent par la rep-province) : la branche Foi exige un lecteur pur neuf
   `religion_of_province(econ,pid)` et, à terme, un missionnaire posant la
   foi PAR PROVINCE (on transfère le chemin — doctrine province).
3. **`CMD_ROUTE` région-grain** : à transférer sur pid dans la vague
   (dette doctrine province rendue visible par la branche Routes).
4. **Deux définitions du bassin de marché** (moteur Centre-seul vs façade
   Marché|Comptoir|Centre) : réaligner le mode carte ou nommer distinctement.
5. `econ_content_dist_faith` sans cid : variante `_ex` (~10 sites) ou repli
   au site légitimité (acté comme repli).
6. La Horde exige des compteurs de sacs/butin sérialisés neufs (les `g_pil_*`
   sont de la télémétrie RAZ-par-sim).

## D4. Collisions de noms — état final

Tranchées : Couronne des serments (Sol B) · Le Libre Passage (Routes B) · Le
Relais (Routes 3) · Cénacle des lettrés (doctrine, H3.4) · « Horde » gardé
(l'épithète d'éthos EST la rime, le nom nu réservé au pivot A). Tolérées (les
branches d'esprit sont EXCLUSIVES entre elles — jamais deux à l'écran d'un
même pays) : « L'Alliage » (Creuset A6 / Savoir 4) · les parachèvements
homonymes d'ambiance.

## D5. Ce qui reste à mesurer (sweep de la vague, le joueur lance)

Les seuils chiffrés des parachèvements (40 % du continent, 40 % des routes,
25 % du continent colonial, 80 % de la pop convertie, 3 tributaires…) sont
des PARIS à mesurer au chronicle apparié 3×3 an 180 — pas des décisions de
design. Métriques déjà nommées par branche : trésor des cités-états (Mer),
revenu médian des voisins d'une Horde, alliances tenues (Diplomatie),
guerres CB_RELIGIOUS (Foi), année d'avènement des Découvertes (Connaissances).
