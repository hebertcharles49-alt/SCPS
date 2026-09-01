# ANNEXE — Les 6 idées des 17 doctrines + synergies de paires

Passe de design 2026-09-01 : 17 agents (un par doctrine), sources imposées =
DESIGN_MISSIONS_DOCTRINES.md + LEVIERS.md. Règles : 6 idées séquentielles
(influence 30-60 croissant), verbe fixé en idée 3-4, AUCUN bonus de complétion,
effets = mults nationaux sur tunables NOMMÉS (patron decree_mult) ou
règles/verbes, jamais d'or créé. Une passe d'HARMONISATION (prix, collisions de
leviers, synergies) clôt le document — voir dernière section.

Ordre : Offense · Défense · Commerce · Mercantilisme · Peuple · Colonisation ·
Diplomatie · Vassaux · Production · Infrastructure · Technologie ·
Connaissances du monde · Faustien · puis les 4 courants (Aristocratique ·
Bourgeoise · Populaire · Divin).

---

## Diplomatie — La Voix des cours
*(Diplomacy — The Voice of the Courts)*

**Identité :** un État qui parle plus souvent, plus vite et à deux voix — non
pas plus fort, mais mieux écouté, et dont les torts se laissent oublier.

**Gate d'adoption :** ≥ 1 **Chancellerie** bâtie · ~100 d'influence ·
entretien mensuel en couronnes (patron décret).

| # | Idée FR (EN) | Coût | Effet mécanique exact (levier=valeur) | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les Lettres de créance** *(Letters of Credence)* | 30 | `OPINION_ALLY` ×1.25 (50→62.5) · `OPINION_PACT` ×1.25 (15→18.75) — mults nationaux, clés sur le porteur comme OBJET de l'opinion | — | Un ambassadeur qui présente ses lettres n'est plus un étranger : il est une main tendue par un nom connu. |
| 2 | **La Chancellerie ordinaire** *(The Standing Chancery)* | 40 | `INFLUENCE_COST_ENVOY` ×0.80 (12→9.6) — *NOUVEAU TUNABLE : `INFLUENCE_COST_ENVOY`, défaut 12* | — | Les greffes tiennent registre, les scribes recopient : envoyer un homme n'est plus un événement, c'est une routine. |
| 3 | **Les Torts qu'on laisse dormir** *(Grievances Let Lie)* | 50 | `OPINION_MEM_DECAY` ×1.30 · `OPINION_RANCOR_W` ×0.85 — mêmes clés que #1 | — | Nos fautes ne sont pas effacées : elles sont racontées autrement, assez longtemps pour que plus personne ne se rappelle laquelle était la nôtre. |
| 4 | **Le Second émissaire** *(The Second Envoy)* — **VERBE** | 55 | Gate élargi : `DIPLO_ENVOY_SLOTS` 1→2 (2 actions diplo en vol ; plancher anti-spam PAR slot) — *NOUVEAUX TUNABLES : `DIPLO_ENVOY_SLOTS` déf. 1 · `DIPLO_ENVOY_FLOOR_DAYS` déf. 30* | **2 alliances tenues sans rupture pendant 10 ans** | On n'attend plus le retour du premier cavalier : deux routes partent le même matin, et la cour cesse de parler à tour de rôle. |
| 5 | **L'Art de la proposition** *(The Art of the Offer)* | 58 | `AI_OFFER_ALLY_OPINION` ×0.75 · `AI_OFFER_MIG_OPINION` ×0.80 — seuils d'acceptation chez autrui pour NOS offres | — | Ce n'est pas l'offre qui a changé, c'est la façon de la poser — et un prince qui hésitait signe. |
| 6 | **Le Congrès des cours** *(The Congress of Courts)* | 60 | `AI_WAR_EXHAUST` ×0.75 · `AI_WAR_DECISIVE` ×0.85 — sur les guerres où le porteur est partie | **3 guerres conclues par une paix négociée** | Les armées attendent dehors pendant qu'on discute de la table : la guerre finit là où elle a toujours fini, dans une pièce. |

**Total : 293 d'influence** (+ ~100 d'adoption ≈ 8 ans à 4/mois).

**Notes de calibrage (agent) :**
1. **Directionnalité des `OPINION_*`** : `opinion[a][b]` = opinion de `a` envers
   `b` ; tous les mults se clés sur `b` = le porteur (comment le monde NOUS
   voit) — sans cette convention au site de lecture, #1/#3 sont un buff
   égocentrique inutile.
2. **Auto-alimentation plafonnée** : une seule idée (#2) baisse un seul coût
   d'influence, −20 % ; revendication, pivots, décrets, synergies restent
   plein tarif. Premier cran de nerf : ×0.80→×0.85.
3. `AI_OFFER_PACT_OPINION` vaut 0 — un mult y est inerte, d'où #5 sur `_ALLY`
   et `_MIG` seulement.
4. **Second émissaire = état inter-ticks ⇒ sérialisé** (jurisprudence
   EMOB/COLC), revalidé `save_sane`, savetest le prend.
5. Plancher anti-spam PAR slot, sinon le 2e émissaire n'est qu'un burst.
6. #6 exige des paix négociées — un empire strictement pacifique ne finit pas
   la piste : assumé (« allié appelé » suffit, jamais d'agression exigée).
   Assouplissement propre si trop mordant : « 3 paix OU 3 vassalités ».
7. #6 coupe dans les deux sens (les guerres qu'on gagne finissent aussi plus
   tôt) — prix identitaire, ne pas « corriger ».
8. P1/golden : effets gatés `human_player >= 0` ; l'adoption IA (P3) déplace
   `AI_OFFER_*`/`AI_WAR_*` chez l'IA — doctrine parmi les plus sensibles au
   re-baseline (métrique : alliances tenues à l'an 180).

**Synergies candidates :** Diplomatie×Vassaux « Le Concert des princes »
(`AI_VASSAL_CONTRIB_GATE` ×0.85 — le serment paie avant l'intégration) ·
Diplomatie×Peuple « Les Noces des peuples » (`MIG_PACT_FRAC_ALLY` ×1.20) ·
Diplomatie×Commerce « Les Traités de commerce » (`TRADE_LEVY` ×0.80 sur les
routes vers un partenaire de pacte/alliance).

---

## Offense — Le Fer en avant

*(Gate : éthos Dominateur ou Honneur + nœud **Caserne** acquis (le NŒUD TECH,
pas un édifice — l'édifice martial de base est la Garnison) · Adoption ~100 ·
entretien couronnes, contrat décret)*

**Identité :** l'État n'entretient pas une armée, il *est* une armée — le fer
part avant que le prétexte soit sec, et il se nourrit sur le pays d'en face.

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **« Les armes d'abord »** (*Arms First*) | 30 | `ARMS_PER_LABORER` ×1.25 · `ARSENAL_DECAY` 0.99→0.995 (« la rouille tombe de 1 %/mois à 0,5 %/mois ») | — | On forge avant de savoir contre qui ; un arsenal plein est déjà une politique. |
| 2 | **« L'école du choc »** (*The School of Shock*) | 36 | `BT_DMG_K` ×1.10 · `CTR_BITE` ×1.15, scopés au pays QUI FRAPPE (site `bt_day`, un mult par camp) | — | Le drill quotidien : on apprend à frapper le flanc qui casse, pas celui qui est en face. |
| 3 | **« L'ost permanent »** (*The Standing Host*) — **VERBE** | 42 | `CMD_HOST_STANDING` (posture on/off, revalidée au drain) : (a) chaque corps en territoire ami comble son déficit à la clôture via `campaign_refill_corps` INCHANGÉ (mêmes gates, même coût armes/pop/or — rien de gratuit) ; (b) la levée se tient au Pied de guerre en paix. **Prix : le ×1.25 de solde du pied de guerre s'applique aussi en paix.** | — | Les régiments ne rentrent plus chez eux : la moisson se fait sans eux, et le trésor le sent chaque mois. |
| 4 | **« Les greniers de l'ennemi »** (*The Enemy's Granaries*) | 48 | `SIEGE_LOOT_FRAC` ×1.30 · `PILLAGE_INCOME_FRAC` ×1.15 — transferts RÉELS (l'or sort de la victime) | **Tenir 3 places ennemies en même temps** (`Σ conquered[moi][b] ≥ 3`, état sérialisé) | Une armée qui campe sur les silos d'autrui coûte moins cher qu'une armée au dépôt. |
| 5 | **« Le prétexte est toujours prêt »** (*A Pretext Always Ready*) | 54 | `FAB_CB_COST_YEARS` ×0.60 (2.0→1.2 an) · `FAB_MATURE_DAYS` ×0.70 (365→255 j) ; `FAB_VALID_DAYS` inchangé | — | La chancellerie garde un tiroir de griefs mûrs ; on y pioche le matin d'une déclaration. |
| 6 | **« L'État en armes »** (*The State in Arms*) | 60 | `SOLDE_FL_PER_REG` ×1.30 · `SOLDE_OVER_K` ×0.80 (« +30 % de limite de force · surcoût de dépassement −20 % ») — ⚠ NOUVEAUX TUNABLES : ces deux valeurs sont des `#define` de `scps_warhost.c` DÉJÀ notés « à migrer au registre J » ; la doctrine exige la migration, sans autre changement | **≥ 1 province arrachée par traité de paix** (`Σ conq_value[moi][b] > 0`, sérialisé) | Le royaume porte plus d'hommes qu'il n'en peut nourrir — et c'est précisément ce qu'on appelle une puissance. |

**Total : 270** (+100 adoption = 370, ~6-8 ans).

**Notes de calibrage (agent) :** kill-switch obligatoire (8 mults à 1.0 +
posture éteinte = golden byte-identique) · l'idée 2 est la seule à toucher
l'issue des batailles — surveiller `battle_days` et le ratio choc/curée en
chronique (+dégâts ⇒ morts au choc ⇒ entropie SANG §27), mesure appariée 3×3
an 180 · l'idée 4 est la seule à toucher la conservation monétaire
(`chronicle_invariant_check` muet ; ne PAS toucher `PILLAGE_COOLDOWN_Y` sinon
le pillage devient une rente) · l'idée 6 est le levier le plus lourd (armée =
10-15 % des dépenses d'État visées) — à balayer seule avant de l'empiler avec
l'idée 3 (composition multiplicative sur la même ligne de solde).

**Synergies candidates :** Offense×Vassaux « Les Marches d'épée » (inféoder
une région occupée à la paix au lieu de l'annexer : `ANNEX_SOFT_SCAR` sauté,
vassal au-dessus du seuil de contribution) · Offense×Divin « La Guerre
sainte » (guerre contre une religion étrangère : les provinces d'origine des
levées gagnent `PROVMOD_FERVEUR_K` tant que le conflit dure) ·
Offense×Aristocratique « La Noblesse d'épée » (un siège mené à terme déclenche
l'adoubement SANS coût d'influence, loyauté du siège de la Guerre en hausse).

---

## Commerce — Les Routes franches
*(Free trade — EXCLUSIVE avec Mercantilisme)*

**Identité :** l'État renonce à tenir les portes : ses marchands sont chez eux
partout, et ce qui circule librement enrichit plus sûrement que ce qu'on
enferme.

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **La franchise des portes** (*The Franchise of the Gates*) | 30 | `TRADE_LEVY` ×0.75 (0.10→0.075), appliqué au bout IMPORTATEUR de chaque route (site `scps_intertrade.c:1020/1084` : `tune_f` descendu DANS la boucle × `doctrine_mult(dst_cid)` — conservation intacte) | — | On abolit le droit d'entrée : la douane ne fait plus la queue à la porte de la ville. |
| 2 | **Les chemins longs** (*The Long Roads*) | 40 | `MARKET_DIST_FALLOFF` ×0.80 (0.12→0.096) pour les régions du pays — à 8 sauts la marge tombe de ×2.16 à ×1.92 : le bassin de marché porte plus loin | — | La route ne s'arrête plus où s'arrête le regard du percepteur. |
| 3 | **Le comptoir franc** (*The Free Factory*) — **VERBE** | 45 | `CMD_FOUND_COMPTOIR(pid)` : pid = cité-état à hub, à la paix, hors embargo, dont dépend ≥ 1 de tes provinces. Coût = recette Comptoir (20 bois/10 argile, 180 j) + 15 influence. Effet : tes régions rattachées à ce hub lisent `own=true` ⇒ marge 1.3 au lieu de 1.8 ; le péage n'est PAS supprimé, il est PARTAGÉ (*NOUVEAU TUNABLE `COMPTOIR_TOLL_SHARE` déf. 0.5* ; 0 = legacy byte-identique). Nouvel état `prov[pid].comptoir_owner` ⇒ bump SAVE_VERSION, `save_sane` | **≥ 2 pactes commerciaux actifs** | On ne prend pas la ville : on y loue une maison, on y tient boutique, et on partage la caisse du pont. |
| 4 | **L'hôte accueillant** (*The Welcoming Host*) | 50 | `IMPORT_MARGIN_THIRD` ×0.85 (1.8→1.53) — acheter par le marché d'autrui cesse d'être une punition | — | Le mercantiliste voit un péager dans chaque hôte ; le libre-échangiste y voit un confrère. |
| 5 | **La chambre des marchands** (*The Merchants' Chamber*) | 55 | `COMMERCE_W_BOURGEOIS` ×1.30 ET `COMMERCE_W_ELITE` ×0.80 — le volume échangeable change de porteur (site `econ_country_commerce`, déjà par-pays via `decree_commerce_w_mult` : le patron exact existe) | **≥ 6 routes inter-pays ouvertes** | Le négoce n'est plus une faveur du prince : c'est le métier d'une classe qui siège. |
| 6 | **Les routes ne se ferment pas** (*The Roads Do Not Close*) | 60 | RÈGLE symétrique : (a) un embargo décrété CONTRE toi ne coupe plus tes routes (`intertrade_embargoed` sauté si un bout tient la doctrine ; la GUERRE coupe toujours) ; (b) tu ne peux PLUS décréter d'embargo (`scps_player_embargo` refusé au drain, checklist en mots) | — | Fermer une route, c'est se couper la main pour punir le voisin. On ne le fait plus — et on ne le subit plus. |

**Total : 280** (+100 adoption ≈ 380). Chaque idée réduit ou redirige un
transfert existant, ou change une règle — aucun crédit de trésor.

**Notes de calibrage (agent, vérifiées dans le moteur) :**
- `IMPORT_TOLL_FRAC` (registre, 0.30) est **MORT** : `scps_agency.c:284` le
  redéfinit en `#define` et le site de paiement verse TOUTE la marge à l'hôte
  sans le lire. Ne rien câbler dessus — le levier réel = la marge (idées 2/4)
  et le split (idée 3).
- Un Comptoir bâti CHEZ SOI donne déjà `own=true` et supprime tout le péage ;
  le comptoir franc est la variante étrangère — `own` SANS annuler le péage,
  sinon la doctrine assèche les cités-états d'un coup.
- Le catchment (`market_hub_regions`) est un BFS non borné : « étendre la
  portée » n'a pas de tunable de rayon ; la portée économique réelle =
  `MARKET_DIST_FALLOFF`.
- L'idée 5 s'EMPILE multiplicativement avec le décret « Comptoirs soutenus » —
  plafond combiné à surveiller au sweep.
- Miroir province : `re->import_margin`/`import_toll_region` sont recopiés sur
  la province représentative — lecture façade par pid, jamais `region[]`.
- Kill-switch : `COMPTOIR_TOLL_SHARE=0` + mults à 1.0 = byte-identique.
- Golden : idées 1-2-4-5 gatées joueur en P1 ; idée 3 = état sérialisé (bump +
  savetest dédié) ; idée 6 touche un gate lu par l'IA ⇒ gater joueur-seul en
  P3 sinon re-baseline.
- Sweep 3×3 an 180 : volume inter-pays, richesse bourgeoise du doctrinaire,
  et **trésor des cités-états** (risque n°1 : vider les banquiers du monde).

**Opposition lisible au Mercantilisme** (l'auto-exclusion est mécaniquement
nécessaire : mêmes sites de lecture, sens contraires) : marge d'autrui
abaissée vs subie/infligée · péage partagé vs capté · embargo aboli vs élargi
· porteur = bourgeois vs dispatch d'État · zéro thésaurisation vs réserves.

**Synergies candidates :** Commerce×Aristocratique « La Maison de commerce »
(chaque comptoir franc debout : `INFLUENCE_PER_NOBLE` ×1.15 — la paire canon
du joueur) · Commerce×Bourgeoise « La Ligue des villes » (`COMMERCE_BLD_MAX`
×1.40) · Commerce×Connaissances « Les Facteurs lointains » (un comptoir franc
compte comme contact maritime soutenu : `SYNC_TRADE_SEA_W` ×1.25).

---

## Défense — Le Bouclier des marches

*(Gate : Garnison bâtie · adoption ~100 · total idées 275)*

**Identité :** on ne gagne pas la guerre : on la fait perdre. Chaque marche
tenue coûte à l'envahisseur une saison qu'il n'avait pas prévue.

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les murs du bourg** (*The Burgh Walls*) | 30 | `DEF_PER_H` ×1.30, portée = propriétaire de la région assiégée (site `region_defense`, `scps_campaign.c:235`) | — | Le maçon du roi passe de bourg en bourg : on épaissit ce qu'on a avant de rêver de citadelles. |
| 2 | **Le magasin des marches** (*The March Stores*) | 35 | *NOUVEAU TUNABLE `SIEGE_FOOD_MONTHS_FULL` déf. 12* (promotion du `#define`) ×1.25 → 15 mois de vivres au lieu de 12 | — | On sale, on enterre, on compte : la marche mange l'hiver de l'autre. |
| 3 | **Le ban des marches** (*The Marcher Ban*) — **VERBE** | 45 | `CMD_LEVEE_BAN(pid)` : région assiégée/occupée, une levée par province et par guerre (latch) ; lève CE MOIS `BAN_MILICE_PACKS` (déf. 3) paquets de milice hors cadence annuelle, sans arsenal ; PRIX : les hommes sortent de la strate journalière et la coercition monte de `BAN_MILICE_COERCION` (déf. 0.10) | — | La cloche sonne à l'envers et le village descend avec ses fourches. |
| 4 | **La corvée de remparts** (*The Rampart Corvée*) | 50 | *NOUVEAU TUNABLE `BUILD_COST_MULT_FORT` déf. 1.0* → ×0.80 sur les quantités de la seule famille Garnison→Forteresse→Citadelle — miroir OBLIGATOIRE devis + gate matière + `scps_build_legal_ex`, sinon le bouton ment | **Avoir tenu un siège** (latch `def_siege_held`) | Le seigneur de marche ne paie pas ses murs : il les fait porter. |
| 5 | **La terre creuse** (*The Hollow Land*) | 55 | `SIEGE_LOOT_FRAC` ×0.60 · `PILLAGE_INCOME_FRAC` ×0.80, portée = propriétaire de la VICTIME — l'envahisseur détourne moins, rien n'est créé | — | On vide les greniers dans les puits avant qu'il arrive : qu'il assiège une coquille. |
| 6 | **Le droit de marche** (*The Marcher Right*) | 60 | RÈGLE sans nombre : pour la famille fortifiée seule, le gate de tech de palier lit `tier−1` — on bâtit un palier plus haut qu'on ne sait ; le prérequis « palier N−1 BÂTI » reste intact | **Avoir repoussé une invasion** (latch `def_invasion_repelled`) | Aux marches, on ne demande pas la permission de bâtir : on demande pardon après. |

**Notes (agent) :** ×1.30 sur `DEF_PER_H` seul ≈ +5-10 % de durée de siège
(dilué par `siege_days()`) — l'empilement avec les vivres (idée 2, en MOIS)
rend la piste lisible ; plafond `SIEGE_MAX_DAYS` 730 j intact · kill-switch :
défauts + `BAN_MILICE_PACKS=0` + `BUILD_COST_MULT_FORT=1` = golden-neutre ·
les 2 latches = inter-ticks ⇒ sérialisés (bump) · impact IA P3 : sièges plus
longs ⇒ guerres plus longues ⇒ sweep sur durée moyenne de guerre + provinces
échangées an 180.

**Synergies candidates :** Défense×Offense « Le Marteau et l'Enclume » (un
corps ami relevant un siège tenu ≥ 6 mois entre au choc en DÉFENSEUR) ·
Défense×Infrastructure « Les Marches de pierre » (`VETUSTE_RATE` ×0.5 famille
fortifiée) · Défense×Populaire « La Levée des paroisses » (le ban sort du
`U_GARDE_ESCORTE` au lieu de la milice, contre `W_AGITATION_UNREST` ×1.15 en
guerre).

---

## Vassaux — La Toile des serments
*(Vassals — The Web of Oaths)*

**Identité :** le suzerain qui ne conquiert pas : il fait jurer, il fait
payer, il fait mûrir — et le vassal cesse d'être un autre sans qu'un seul
siège ait été mis.

*(Gate : ≥ 1 vassal · adoption 100 · idées 280 ≈ 380 total)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Le serment tenu** (*The Oath Kept*) | 30 | `AI_VASSAL_INTEGRATE_YEARS` ×0.85 (20→17 ans) au site `scps_diplo.c:396` | — | Le serment prêté une fois n'a plus besoin d'être rappelé. |
| 2 | **La part du suzerain** (*The Overlord's Share*) | 40 | `AI_VASSAL_CONTRIB_GATE` ×0.85 (0.65→0.55) · `AI_VASSAL_CONTRIB_BASE` ×1.20 — le vassal verse plus tôt et un peu plus — VOIR NOTE (création grain/armes à corriger d'abord) | — | Ce qu'un vassal doit, il le doit tous les ans, et il le doit tôt. |
| 3 | **Le contrat imposé** (*Terms Dictated*) — **VERBE** | 45 | `CMD_PEACE_OFFER` étendu : `PEACE_VASSALIZE` ne pose plus Protectorat en dur — le joueur CHOISIT le contrat, prix en score de paix modulé : Servage ×1.5 · Protectorat ×1.0 · Concordat ×0.75 · Cité ×0.75 (si cité-état). Aucun contrat neuf. | — | À la table de paix, on ne demande plus la soumission : on en dicte la forme. |
| 4 | **Les mains du maître** (*The Master's Hands*) | 50 | Gate élargi : les 4 contre-leviers de fronde de l'IA (Don=transfert réel · Alléger · Diviser · Intimider, `scps_diplo.c:534-568`) deviennent `CMD_VASSAL_LEVER` joueur quel que soit l'éthos — on ouvre le répertoire, zéro règle neuve | **≥ 2 vassaux simultanés** | On ne tient pas une toile en tirant sur un seul fil. |
| 5 | **Digérer sans conquérir** (*Digest, Not Conquer*) | 55 | Gate élargi DÉFINITOIRE : `annexeur = éthos OU doctrine_tenue` (sans lui un Bureaucrate/Pacifiste ne digère JAMAIS) + `ANNEX_INTEGRATION_DISCOUNT` ×1.25 (0.6→0.75) ; or payé, cicatrice nulle d'elle-même à intégration pleine | — | La conquête est le raccourci des impatients ; la patience annexe sans plaie. |
| 6 | **Le serment sans guerre** (*The Oath Unbloodied*) — 2e verbe | 60 | `CMD_OFFER_SUZERAINTY(cible, contrat)` — la « voie mission » que `scps_diplo.h:183-184` réserve DÉJÀ en commentaire ; accepté sur motif `AI_OFFER_*` + ratio de force ≥ 1.8 ; *NOUVEAU TUNABLE `AI_OFFER_SUZ_OPINION` déf. 55* ; borné Protectorat/Concordat | **1 vassal mené à intégration pleine** | La plus belle couronne est celle qu'on vous apporte. |

**Notes (agent) — dont une TROUVAILLE à ticketer :**
- ⚠ **Création existante, ANTÉRIEURE à la doctrine** : les canaux agraire
  (`econ_region_stock_add` grain, `scps_diplo.c:413`) et martial
  (`mil_stock +=`, `:414`) de la contribution vassale CRÉDITENT le maître
  SANS débiter le vassal (seul le canal commerce a eu le miroir M3f).
  Porter le miroir M3f sur les 2 canaux AVANT de livrer l'idée 2
  (recommandé), sinon restreindre l'idée 2 au seul `_GATE`.
- Tick suzeraineté ANNUEL ⇒ membrane : « ~N couronnes/mois » (encaissé/12),
  jamais le taux.
- Seuils de ligue = littéraux en dur — délibérément non touchés (la fronde
  reste la contrepartie) ; dial futur unique si besoin : `VASSAL_GRIEF_RATE`.
- L'idée 3 crée l'arbitrage traire (servage, grief +0.10/an) vs digérer
  (concordat, +0.005/an) sans changer le prix moyen.
- L'idée 5 modifie une condition tickée : gater strictement sur la doctrine.

**Synergies candidates :** Vassaux×Offense « Les Marches d'épée » (vassal
appelé en guerre verse sa contribution martiale sans gate d'intégration) ·
Vassaux×Peuple « Les Noces des maisons » (pacte migratoire avec un vassal :
`irate` 0.3+0.7·prox → 0.5+0.5·prox) · Vassaux×Diplomatie « Le Congrès des
couronnes » (`AI_OFFER_SUZ_OPINION` ×0.8 + une ligue ne se noue pas tant
qu'un serment frais court chez le meneur pressenti).

---

## Colonisation — L'Appel du large

**Identité :** partir cesse d'être un pari : on part de plus petit, on tient
sur la terre rude, et la seconde quille est déjà sur cale.

*(Gate : Port bâti + tech Comptoirs · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les mains de trop** (*The Spare Hands*) | 30 | `COLONY_MIN_POP` ×0.85 (300→255) · `COLONY_COST_POP` ×0.90 (150→135) | — | La paroisse qui a plus de bouches que de sillons n'attend plus d'être une ville pour en envoyer au large. |
| 2 | **Le grenier d'avance** (*The Standing Granary*) | 36 | `FOOD_STOCK_MONTHS` ×0.75 (6→4,5 mois) — la voie GRENIER du gate vivrier s'ouvre plus tôt ; `COLONY_FOOD_GATE` non touché (réservé au Dessein) | — | On ne part plus le ventre plein : on part le ventre assuré. |
| 3 | **L'apprentissage de la terre rude** (*Hard Ground Learned*) | 42 | `HAB_MALUS_K` ×0.80 aux DEUX sites (prod `scps_econ.c:4044` · croissance `:5193`) | — | La lande, la mangrove et le caillou finissent par se laisser labourer par qui s'entête. |
| 4 | **La seconde quille** (*The Second Keel*) — **VERBE** | 48 | `ColonyWork` → 2 chantiers simultanés par pays ; la cadence `cd_days` (360 j) reste PAR SLOT, inchangée — on double la simultanéité, jamais la fréquence. `sizeof` change ⇒ bump | **≥ 3 colonies vivantes fondées** | Deux ports, deux départs, une seule couronne qui compte les voiles. |
| 5 | **L'école des climats** (*The Schooling of Climates*) | 54 | *NOUVEAU TUNABLE `CLIM_LEARN_INTEG` déf. 0.99* ×0.90 → 0,89 : le seuil d'intégration au-delà duquel un groupe déplacé LÈGUE son climat natal tombe — le bit n'est pas donné, il est appris plus tôt | — | Un peuple digéré du désert enseigne le désert : c'est une génération qui a survécu. |
| 6 | **Les terres du bout** (*The Farthest Shores*) | 60 | *NOUVEAU TUNABLE `COLONY_YIELD_HREF` déf. 4.0* ×1.5 → 6,0 : le rendement log-distance s'aplatit (+6 pts à 8 sauts : 48→54 %) ; durées et plancher 0,30 inchangés | **1 colonie outre-mer vivante** (continent ≠ capitale) | Le convoi qui arrivait exsangue arrive maigre : c'est la différence entre un comptoir et un cimetière. |

**Notes (agent) :** frontière Desseins/doctrine EXPLICITE — laissés au
Dessein sans y toucher : `COLONY_FOOD_GATE`, le DON d'un bit climat, la
cadence `cd_days`, `PROVMOD_FERVEUR_K`, le brouillard · les 2 tunables neufs
sont des promotions de constantes AUX VALEURS ACTUELLES (kill-switch
byte-identique) · pièges : `econ_colony_food_ok(pe)` sans cid ⇒ mult résolu
depuis `pe->owner` ; le miroir UI `scps_api.c:4073` doit appliquer le MÊME
mult (bug déjà vécu une fois) ; `HAB_MALUS_K` a DEUX sites ; le verbe rippe
`scps_colony_status` + banc `scps_api_demo` (assertion à réécrire).

**Synergies candidates :** Colonisation×Connaissances « Les Grandes
Découvertes » (`NAVY_COLONY_MAX_DAYS` ×1.5 : la portée d'expédition devient
portée de fondation) · Colonisation×Mercantilisme « La Compagnie à charte »
(`COLONY_WEALTH_SHARE` ×0.6 : la charte finance le convoi, la province-mère
cesse de s'appauvrir) · Colonisation×Peuple « Les Terres promises »
(`MIG_ATTRACT_INST_W` ×1.3 · `PROVMOD_FERVEUR_DECAY` ×0.7 : l'élan fondateur
dure une génération).

---

## Peuple — Le Creuset
*(The People — The Crucible)*

**Identité :** un pays qui ne demande pas à l'étranger de disparaître mais de
s'asseoir à table : l'âme venue d'ailleurs cesse vite d'être une friction et
devient un bras, un métier, une tech.

*(Gate : Adaptatif/Pacifiste + Tribunal & Chancellerie bâtis · adoption ~100 ·
idées 285 · entretien : NOUVEAU TUNABLE `DOCTRINE_PEUPLE_REVENUE_RATE`
déf. 0.015, motif décret)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Le Droit d'hôte** (*Guest-Right*) | 30 | `AI_OFFER_MIG_OPINION` ×0.7 (40→28) · `MIG_PACT_MIN` ×0.5 (30→15 âmes) | — | On ne fouille pas le bagage de qui frappe à la porte : le voisin méfiant signe quand même, et le mince filet de gens compte. |
| 2 | **Les Écoles de la langue** (*Schools of the Tongue*) | 40 | `ASSIM_K_INST_REF` ×0.8 (1.5→1.2) au site `scps_demography.c:1149` ⇒ ≈ +6 % d'intégration/mois sur chaque groupe déplacé | — | Le greffier, l'école et le tribunal font ce que la garnison ne fait pas : l'accent reste, la loyauté vient. |
| 3 | **Les Portes ouvertes** (*The Open Gates*) | 46 | `REFUGEE_SETTLE_INTEG` ×0.9 (le réfugié se FIXE plus tôt) · `REFUGEE_RETURN_PULL` ×0.8 (il repart moins) | **Héberger ≥ 400 âmes réfugiées** (somme directe, aucun accumulateur neuf) | Les proscrits d'un royaume en cendres découvrent qu'on leur a gardé une place ; beaucoup ne rentreront jamais. |
| 4 | **Le Pacte d'accueil** (*The Pact of Welcome*) — **VERBE** | 52 | `CMD_OFFER_MIGRATION` élargi (2e mode) : tant qu'UN pacte de ce type tient, à chaque clôture mensuelle, tout groupe DÉPORTÉ de mes provinces bascule déporté→migrant ET servile→journalier, intégration gardée (la bascule exacte de `demography_manumit_country`) ; effet réel : diffusion `METAB_DIFFUSE_SLAVE` 0.30→1.0 et la friction off-culture devient PLEINE (le prix du choix) | **≥ 1 pacte migratoire en vigueur** | Le captif d'hier signe le même registre que le marchand d'à-côté : il n'est plus un bien, il est un nouveau venu — et il faudra désormais le contenter. |
| 5 | **Le Creuset** (*The Crucible*) | 57 | *NOUVEAUX TUNABLES (promotions aux valeurs actuelles) : `OFF_CULTURE_SAT_PEN` déf. 0.45 · `OFF_CULTURE_SOC_PEN` déf. 0.60* → ×0.75 aux deux (0.34/0.45) | — | Chez nous, l'étranger mal servi par nos biens et nos rites l'est un peu moins mal : le pays porte sa diversité au lieu de la subir. |
| 6 | **Le Peuple des peuples** (*A People of Peoples*) | 60 | `METAB_TIER1/2/3` ×0.75 (la barre d'accès aux héritages digérés tombe) · `AI_METAB_RES_W` ×1.20 (le +% recherche du creuset) | **`econ_country_metabolized` ≥ 0.10** | Assez d'âmes venues d'ailleurs ont fait souche pour que leurs pères nous lèguent leurs outils : nos savants lisent enfin dans leur langue. |

**Notes (agent) :** la séquence 4→5 est le cœur : l'idée 5 est la
CONTREPARTIE de l'idée 4 (le servile libéré pèse plein en friction
off-culture) · `ASSIM_K_INST_REF` est un point de centrage — le baisser
relève K_eff uniformément sans punir la province mal bâtie ; ne PAS toucher
`_AMP` (amplifierait aussi le malus) · le verbe bascule TOUS les déportés du
pays (l'origine-pays n'est pas lisible : `home_reg=-1` posé à la déportation
— la solution la plus simple d'abord) · anti-boucle : les freins existants
(révolte servile, agitation par tension culturelle, fracture) ne sont PAS
touchés — le Creuset accélère la digestion, il ne supprime jamais
l'indigestion ; `POP_SAT_W` volontairement hors piste (bonus plat déguisé) ·
promotions de tunables byte-neutres · golden P1/P3 joueur-seul intact.

**Synergies candidates :** Peuple×Technologie « Le Collège des langues »
(`AI_METAB_RES_W` ×1.15 de plus · `AI_TECH_DIFFUSE_MAX` 0.40→0.50) ·
Peuple×Colonisation « Les Cent Peuples du large » (la part digérée d'un
héritage d'un climat étranger étend le bitmask climats 2× plus vite) ·
Peuple×Diplomatie « La Cour cosmopolite » (`OPINION_PACT` 15→22 ·
`MIG_PACT_FRAC_LATE` ×1.2 : chaque diaspora est une ambassade).

---

## Production — L'Atelier du monde

**Identité :** faire rendre à la terre plus que son tirage et à l'atelier
plus que sa taille : on ne découvre pas de ressource, on creuse plus profond
et on équipe plus de bras.

*(Gate : tech Fonderie + Outillage · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les bras à la terre** (*Hands to the Land*) | 30 | `EXTRACT_LABOR_SHARE` ×1.12 (0.65→0.728) — « +12 % de bras à l'extraction, autant de moins aux manufactures » | — | Le pays décrète que la mine passe avant l'échoppe. |
| 2 | **L'outil au poing** (*Tool in Hand*) | 36 | *NOUVEAU TUNABLE `TOOLS_PER_LABORER` déf. 0.05* (promotion du `#define`) ×1.30 — le parc-outil visé monte ⇒ prod monte, la demande tire le prix de l'outil | — | On ne donne pas d'outil : on en veut plus. Le marché suit, la forge s'allume, l'outil se paie. |
| 3 | **Le fond du filon** (*The Bottom of the Seam*) — **VERBE** | 42 | `RAW_BOOST_MAX_TIER` ×1.5 (8→12) + `CMD_RAW_BOOST` joueur (grain province, sur une des ≤ 2 brutes du tirage, or = `RAW_BOOST_COST` × ipm) — jusqu'à +60 % au lieu de +40 % sur une brute menée au bout | **≥ 1 province au palier 8 sur une de ses brutes** | Les autres arrêtent de creuser quand le filon devient dur. Nous, on descend d'un étage. |
| 4 | **La chaîne d'ateliers** (*The Chain of Workshops*) | 48 | `MANUF_QOUT_MULT` ×1.15 — pas d'efficience magique : la capacité monte, donc intrants ET bras montent d'autant | **≥ 8 manufactures actives** | Un atelier n'est rien ; huit qui se passent la matière font une industrie. |
| 5 | **Les gages de l'atelier** (*The Workshop's Wages*) | 54 | `MANUF_BUILD_COST` ×0.85 (site déjà scopé pays par `decree_manuf_cost_mult`) · `JOB_UPKEEP_TAX_FRAC` ×0.80 | — | La densité n'est pas une question de volonté mais de paie. |
| 6 | **Ce que la terre doit** (*What the Land Owes*) | 60 | `RAW_BOOST_PER_TIER` ×1.2 (5→6 %/palier) · `RAW_BOOST_COST` ×0.75 — composé avec l'idée 3 : 12×6 % = +72 % au bout | — | La terre n'a pas de fond — seulement des hommes qui renoncent trop tôt. |

**Notes (agent) :** RÈGLE DES 2 BRUTES RESPECTÉE — aucune idée ne greffe de
ressource, `raw_boost[]` ne multiplie qu'une brute déjà au tirage · pièges :
`EXTRACT_LABOR_SHARE` est lu HORS boucle (à descendre dans la boucle province
pour le scoper) et INERTE sous override d'allocation joueur (à dire en hover)
· `RAW_BOOST_MAX_TIER` a DEUX sites (clamp moteur + gate IA — chacun avec son
cid, sinon l'IA bâtit des paliers re-clampés en silence) · `MANUF_BUILD_COST`
payé en 3 sites + lu par l'UI (jurisprudence : la remise ATELIERS avait été
oubliée dans le prix montré) · empilement idée 5 × décret ATELIERS = ×0.8075,
le cumul le plus fort du lot · kill-switch : 8 clés `DOCT_PROD_*_MULT` à 1.0
= golden-neutre · à mesurer : prix mondial fer/bois an 120 (la doctrine
déplace le prix NATIONAL, pas seulement son stock).

**Synergies candidates :** Production×Mercantilisme « La Manufacture d'État »
(le dispatch préempte la sortie des manufactures à la parité fixe avant le
marché) · Production×Infrastructure « Les Fonderies perpétuelles »
(`VETUSTE_RATE` ×0.75 bâti industriel + les paliers d'exploitation cessent de
se dégrader) · Production×Offense « L'Arsenal du royaume »
(`MANUF_ARMS_MULT` ×1.3 : l'industrie devient directement de l'ost).

---

## Infrastructure — La Pierre et l'eau

**Identité :** bâtir n'est pas l'affaire d'un règne mais d'un métier : l'État
possède ses maçons, sa matière et son entretien — la pierre posée ne
redevient jamais une ruine.

*(Gate : tech Atelier de construction · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **La guilde des maçons** (*The Masons' Guild*) | 30 | *NOUVEAU TUNABLE `BUILD_MAT_MULT` déf. 1.0* ×0.90 sur les quantités de matière d'édifice (l'or suit mécaniquement) ; durées INCHANGÉES | — | Le tailleur de pierre qui connaît son banc taille moins de déchet. |
| 2 | **Les carrières du royaume** (*The Realm's Quarries*) | 36 | `RAW_WORKS_NEED` ×1.30 (raw-works armés plus tôt) · `BUILD_RESERVE_BULK` ×1.30 (fond de trio plus profond avant export) | — | On n'exporte pas la pierre dont le siècle prochain aura besoin. |
| 3 | **L'entretien ordinaire** (*Common Upkeep*) | 42 | `VETUSTE_RATE` ×0.75 (2→1,5 %/an) · `VETUSTE_FLOOR` ×1.15 (0,50→0,575) | — | Un toit réparé chaque automne ne se refait jamais. |
| 4 | **Les grands chantiers** (*The Great Works*) — **VERBE** | 48 | `CMD_RENOV_MASS` : file NATIONALE de rénovation, `RENOV_MASS_SLOTS` (déf. 3, 0 = kill-switch) chantiers simultanés, réalimentée à la clôture (tri usure croissante puis pid — ordre total, zéro rand), chaque site via `agency_renover_acct` INCHANGÉ (or débité province par province) ; + `RENOV_COST_FRAC` ×0.80 | **≥ 5 provinces à usure ≥ 95 % portant ≥ 3 édifices** | On ne demande plus au roi quel mur réparer : la file le sait. |
| 5 | **Les logis de la manufacture** (*Manufactory Lodgings*) | 54 | `HOUSE_MANUF` ×1.25 (100→125 logements/niveau) ; plafond ½·cap inchangé | — | La ville d'atelier loge ses bras au lieu de les entasser. |
| 6 | **L'intendance de l'empire** (*The Empire's Stewardship*) | 60 | *NOUVEAU TUNABLE `BUILD_EXTENT_K` déf. 0.15* (extraction du littéral d'`agency_extent_mult`) ×0.70 — le surcoût d'étendue passe de +15 % à +10,5 %/province : un empire vaste bâtit encore | **≥ 4 provinces portant un édifice de 960 j** (Académie·Citadelle·Cathédrale) | L'empire qui sait tenir ses comptes de pierre cesse de payer sa propre taille. |

**Notes (agent) :** zéro or créé — remises et plafond de logement seulement,
la file DÉPENSE via `credit_spend` · les durées ne bougent pas (contrat
180/360/540/960 j) · pièges : quantité lue à TROIS endroits (devis, gate
matière, consommation) ⇒ `BUILD_MAT_MULT` par UN SEUL helper sinon TOCTOU
« or plein / matière partielle » déjà documenté ; formule d'étendue DUPLIQUÉE
en dur dans `scps_api.c:2782` (les 2 sites ou la checklist ment) ;
`HOUSE_MANUF` vit dans une macro partagée moteur+membrane (`ECON_EFFCAP_BODY`)
— le mult DEDANS, sur owner · état de file = un booléen/pays ⇒ bump ·
prix 270 assumé sous la bande : doctrine d'endurance (si trop bon marché,
monter la rampe à +7, pas gonfler les effets).

**Synergies candidates :** Infrastructure×Populaire « Les Grands Travaux »
(`RENOV_SHARE_LAB` ×1.40 : 70 % de l'or des chantiers aux journaliers ·
`W_AGITATION_UNREST` ×0.85 — la truelle occupe les bras qui autrement se
lèvent ; la paire canon) · Infrastructure×Défense « Les Murs de la marche »
(la file sert d'abord les provinces frontalières · `DEF_PER_H` ×1.20) ·
Infrastructure×Production « L'Atelier de pierre » (`HOUSE_MANUF` ×1.15 de
plus · `RAW_BOOST_COST` ×0.80).

---

## Mercantilisme — L'Étape souveraine
*(Mercantilism — The Sovereign Staple — EXCLUSIVE avec Commerce)*

**Identité :** le royaume est un magasin muni d'une porte : ce qui entre passe
par mon Étape, paie mon droit, ou ne passe pas.

*(Gate : Entrepôt bâti + tech Halles · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Le Fond de magasin** (*The Standing Store*) | 30 | `BUILD_RESERVE_BULK` ×1.30 (15→19,5) · `AI_SAFE_STOCK_MONTHS` ×1.30 (6→7,8 mois de coussin visé) | — | On ne vend pas la pierre dont nos maçons manqueront le mois prochain. |
| 2 | **Le Magasin qui veille** (*The Watchful Store*) | 36 | `SPEC_BUY_BAND` ×1.10 (0,80→0,88) · `SPEC_SELL_BAND` ×0.92 (1,25→1,15) — l'entrepôt d'État achète plus tôt, relâche plus tôt | — | Le garde-magasin ne dort plus : il regarde le prix tous les matins. |
| 3 | **Le Blocus** (*The Blockade*) | 42 | RÈGLE élargie : MON embargo traverse le pacte commercial (la route GARANTIE tombe) + le banni perd l'accès à MES Centres (arbitrage + compteur de routes). Exige `intertrade_embargoed_by(cid,target)` — l'accesseur actuel est SYMÉTRIQUE et élargirait aussi l'embargo subi | — | Un traité signé ne vaut pas une chaîne en travers du port. |
| 4 | **Le Droit d'étape** (*The Staple Right*) — **VERBE** | 48 | `CMD_ETAPE_SET(pid, res)` grain PROVINCE : désigner UNE province à Entrepôt comme Étape d'UN bien — servie LA PREMIÈRE au dispatch du pool national (avant le prorata-pop), et le bien tenu est exempté de la part d'export. Une Étape à la fois, permutable, revalidée au drain | **Un embargo décrété par moi tenu ≥ 5 ans** (latch `embargo_since_day`, sérialisé) | Toute la laine du royaume dort trois nuits dans mes halles avant de voir la mer. |
| 5 | **Le Péage de la couronne** (*The Crown's Due*) | 54 | `IMPORT_MARGIN_OWN` ×0.80 (1,30→1,04, PLANCHER DUR 1.00) · `TOLL_STATE_SHARE` ×1.50 (0,50→0,75 — trois quarts du péage à la couronne, 3 sites) | — | L'étranger paie le port ; le port paie le roi, pas la guilde. |
| 6 | **Les Halles du royaume** (*The King's Halls*) | 60 | *NOUVEAUX TUNABLES (promotions) : `STOCK_CAP_ENTREPOT` déf. 500* ×1.30 → 650 · *`STOCK_DECAY_PERISH` déf. 0.85* ×1.06 → 0,90 (périssables −10 %/mois au lieu de −15, CLAMP ≤ 0.98 — une décrue ≥ 1 créerait de la marchandise) | **Magasins pleins : ≥ 4 biens ≥ 80 % du plafond, 12 clôtures consécutives** (compteur sérialisé) | Nos halles tiennent l'hiver, et l'hiver d'après. |

**Notes (agent, vérifiées moteur) :** ⚠ `IMPORT_TOLL_FRAC` est INERTE (entrée
registre orpheline, le site paie toute la marge via `TOLL_STATE_SHARE`) — même
constat que l'agent Commerce, à purger/brancher en ticket séparé · `SPEC_*`
mord bien le pays joueur (`ai_on[]` vrai sous l'IA AUTO) — à revérifier en
début de vague · invariants : marge OWN plancher 1.00, décrue plafond 0.98,
l'Étape ne descend jamais une province sous sa cible vivrière (clamp au site
de dispatch) · l'idée 3 touche 3 sites lus par l'IA ⇒ derrière
`doctrine_has(...)`, faux par défaut · au sweep : σ des prix DOIT baisser
(piège B1), péage = transfert net dans FX_TOLL, kill-switch prouvé.

**Opposition à Commerce :** retenir vs ouvrir · péage levé vs partagé ·
embargo-arme vs embargo-aboli · dispatch d'État vs bourgeois · le stock dort
vs circule. Aucun levier ne se recouvre — l'exclusion est propre.

**Synergies candidates :** Mercantilisme×Production « La Manufacture d'État »
(l'Étape sert la manufacture avant le marché · `RAW_BOOST_PER_TIER` ×1.25 sur
la brute à l'Étape seule) · Mercantilisme×Colonisation « Le Pacte colonial »
(les colonies ne commercent qu'avec la métropole, leur surplus remonte à
l'Étape) · Mercantilisme×Bourgeoise « La Compagnie privilégiée » (l'embargo
exempte nommément mes marchands · `CREDIT_LINE_BASE` ×1.15).

---

## Technologie — Le Concile des lettrés
*(Technology — The Council of Lettered Men)*

**Identité :** la recherche cesse d'être un sous-produit de la richesse pour
devenir une politique — on choisit ce qu'on étudie, on copie ce que le monde
sait, et on refuse d'ouvrir les pages interdites.

*(Gate : Bibliothèque + Scriptorium · adoption ~100 · idées 265)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les Rayonnages** (*The Stacks*) | 30 | `SAVOIR_LIB_PER` ×1.25 · `SAVOIR_LIB_MAX` ×1.30 (0,33→0,43) — site `econ_country_savoir`, patron `decree_savoir_w_mult` mot pour mot | — | On cesse d'empiler les rouleaux : on les range, et on retrouve ce qu'on a écrit. |
| 2 | **Les Écoles de ville** (*The Town Schools*) | 35 | `SAVOIR_W_BOURGEOIS` ×1.30 · `SAVOIR_W_LABORER` ×1.25 ; `SAVOIR_W_ELITE` INTOUCHÉ (exige d'élargir le mult en par-classe) | — | Le fils du drapier apprend à lire : ce n'est plus le privilège d'un salon. |
| 3 | **Le Programme du Concile** (*The Council's Curriculum*) — **VERBE** | 40 | `CMD_DOCTRINE_QUARTIER` (quartier 0-8 = thème×fonction) : le quartier nommé ×0.80, tous les autres ×1.10 — via `doctrine_tech_cost_mult(cid,id)` posé aux DEUX sites de coût (joueur `scps_sim.c:1123` + `ai_effective_cost`), JAMAIS dans `tech_cost()` (pas de cid = le piège tune_set). Changer de quartier : 10 d'influence. État `doctrine_quarter[cid]` sérialisé | — | Le Concile arrête un programme d'étude ; les autres chaires attendent leur siècle. |
| 4 | **Les Copistes voyageurs** (*The Travelling Copyists*) | 45 | `AI_TECH_DIFFUSE_MAX` ×1.30 (0,40→0,52) — ⚠ `tech_diffusion_mult(id)` n'a pas de cid : lui en passer un (2 sites d'appel) | **Chaîne Bibliothèque complète (Bibliothèque+Monastère) sur UNE province** | On n'invente pas ce que le voisin sait déjà : on l'envoie recopier. |
| 5 | **La Dispense** (*The Dispensation*) | 55 | Gate élargi : lève le gate d'âge sur Société 3 et Savoir 4 SEULEMENT, et UNIQUEMENT pour les nœuds NON faustiens — sans le filtre, Savoir 4 ouvrirait L'ÉVEIL ⚠ (déclencheur de crise de fin) : clause structurelle. Société 5 / Savoir 5 restent souverains. Ripple : le test existe en DOUBLE (`scps_events.c:3519` + copie `ai_age_tier_open`) | **≥ 3 nœuds de tier 2 acquis** | Le siècle n'est pas encore venu, mais le Concile a déjà lu le livre. |
| 6 | **Le Serment de sobriété** (*The Sober Oath*) | 60 | Via le même `doctrine_tech_cost_mult` : non-faustien ×0.90, FAUSTIEN ×1.50 — la doctrine ne facilite jamais le faustien, elle le renchérit | — | Le Concile scelle un rayon et jette la clef ; ce qu'on y gagne, on le gagne proprement. |

**Notes (agent) :** composition 3×6 : quartier 0.80 × propre 0.90 = 0.72 sur
un `TECH_COST_MULT` déjà à 0.70 ⇒ **clamper le mult composé à [0.70, 1.60]**
(précédent exact : `ai_tech_tradition_mult` clampe [0.5, 2.0]) · débit total
attendu ~+15 %/mois (~un nœud d'avance sur 30 ans) — mesurer au chronicle
(télémétrie `research_points` déjà instrumentée) · le vrai poids est au verbe
(jouer étalé ≈ neutre, se spécialiser = payant : un choix, pas un bonus) ·
**collision de nom** : l'apex T5 s'appelle déjà « Concile des savants » —
renommer la doctrine (« L'Atelier des lettrés » ?) ou l'apex · save :
`doctrine_quarter` int8 borné [−1, 8].

**Synergies candidates :** Technologie×Connaissances « Le Grand Atlas »
(`SYNC_TRADE_METIER`/`_PROFOND` ×0.80 : chaque route est un copiste) ·
Technologie×Peuple « Les Bancs du Creuset » (`AI_METAB_RES_W` ×1.30) ·
Technologie×Bourgeoise « Les Presses de la guilde » (`SAVOIR_W_BOURGEOIS`
×1.40). **Paire volontairement VIDE : Technologie×Faustien** — l'idée 6 rend
leurs effets contradictoires ; le sous-menu reste vide, c'est l'énoncé de la
doctrine.

---

## Connaissances du monde — Les Cartes et les langues
*(World Knowledge — Charts and Tongues)*

**Identité :** connaître le monde avant de le prendre — lever les cartes,
apprendre les langues, et faire du savoir des autres peuples une entrée de
notre moteur (Technologie cultive le savoir intérieur ; celle-ci va le
chercher au-dehors).

*(Gate : Observatoire OU Amirauté · adoption ~100 · idées 280)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les portulans** (*The Portolans*) | 30 | *NOUVEAU TUNABLE `FOG_SEA_HALO` déf. 8* (promotion du `#define`, `scps_api.c:656`) ×2.0 → 16 cellules d'eau/basse-terre révélées autour du connu — purement façade, golden-neutre par construction | — | Nos pilotes tiennent le trait des côtes bien au-delà de ce que nos armées ont foulé. |
| 2 | **Les truchements** (*The Interpreters*) | 40 | `SYNC_TRADE_METIER` ×0.8 · `SYNC_TRADE_PROFOND` ×0.8 — à volume égal, un contact franchit plus tôt les paliers d'accès aux héritages | — | Un homme qui parle leur langue vaut trois navires de marchandise. |
| 3 | **L'expédition lointaine** (*The Far Expedition*) — **VERBE** | 45 | `CMD_EXPEDITION_LOIN(region)` : cible inconnue d'autrui, Port ou Caravansérail requis ; coût 10 influence + 0,25 an de revenu (motif `FAB_CB_COST_YEARS`, dépense réelle) ; cadence 36 mois ; à l'arrivée (360 j) : `known[moi][owner]=1` + révélation à 2 sauts + un « comptoir d'étude » latché qui compte dans la somme de contact culturel au poids 1.0 (comme une route, sans commerce). PAS de jet d'échec : le coût et la cadence SONT le risque. 6 tunables neufs `EXPEDITION_*` | — | On arme un navire, on choisit un point sur le vide, et l'on revient avec un dictionnaire. |
| 4 | **Le dictionnaire des peuples** (*The Book of Peoples*) | 50 | `METAB_TIER1` ×0.8 · `METAB_TIER2` ×0.8 — ⚠ `METAB_TIER3` INTOUCHÉ : il gate AUSSI la victoire Merveille (y toucher brade une condition de fin) | **3 héritages étrangers à profondeur ≥ métier** | Ce que trois peuples savent faire, nous savons désormais le lire. |
| 5 | **Le creuset des savoirs** (*The Crucible of Learning*) | 55 | `AI_METAB_RES_W` ×1.4 — ne touche PAS `SAVOIR_W_*`/`SAVOIR_LIB_*` (terrain de Technologie) | **1 héritage étranger métabolisé** | Nos académies ne parlent plus une langue : elles en parlent six. |
| 6 | **La langue franque** (*The Lingua Franca*) | 60 | RÈGLE — connaissance TRANSITIVE : au tick annuel de `fog_update`, `known[moi][x] |= known[p][x]` pour tout p en route ouverte ou alliance avec moi (un saut, non récursif, jamais d'effacement). Effet réel : diplo ouverte, verbes joueur qui cessent de buter sur « cible inconnue » | — | Nos alliés parlent, nos marchands écoutent : la carte du monde s'écrit dans nos ports. |

**Notes (agent) :** frontière avec le Dessein « Les éclaireurs » par le CANAL
(le Dessein garde le rayon de région `ages_fog_radius_add` ; la doctrine
prend le halo maritime, l'expédition ciblée, la transitivité — zéro
recouvrement de tunable) · le verbe finance sa suite : 3 expéditions ≈ 9 ans
arment exactement le prérequis de l'idée 4, sans compteur neuf
(`arch_depth[]` est l'état de vérité — à vérifier qu'il est sérialisé) ·
`SYNC_FUSE_RATE` écarté (il homogénéise — l'inverse de l'identité) · l'idée 6
écrit dans un état PARTAGÉ (`known[][]`) et déplace l'avènement de l'Âge des
Découvertes ⇒ sweep apparié dédié sur l'année d'avènement · expédition en vol
= accumulateur sérialisé, cible invalidée ⇒ résolution à vide, jamais d'échec
silencieux.

**Synergies candidates :** Connaissances×Colonisation « Les Grandes
Découvertes » (région révélée par expédition = éligible au chantier colonial
HORS adjacence) · Connaissances×Commerce « La Route des langues »
(`SYNC_TRADE_SEA_W`/`_LAND_W` ×1.25) · Connaissances×Technologie « Le Concile
cosmopolite » (`AI_TECH_DIFFUSE_MAX` ×1.25).

---

## Faustien — Le Pacte

**Identité :** « Tout ce que tu ne peux pas extraire, transmute-le ; tout ce
que tu transmutes, le monde le paiera. » La seule doctrine du catalogue dont
chaque idée rapproche la fin du monde — et qui le dit.

*(Gate : 1 tech ⚠ acquise · adoption 100 · idées 290 · entretien
`DOCFAUST_UPKEEP_RATE` 0.02, haut de bande)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **La Page arrachée** (*The Torn Page*) | 30 | Coût de recherche des nœuds ⚠ ×0.85 — via `ai_tech_tradition_mult(cid,id)` (déjà faustien-only + cid-scopé ; joueur, IA et prix affiché passent par LE MÊME facteur) | — | On a recopié le feuillet qu'il fallait brûler. |
| 2 | **Les Deux Creusets** (*The Two Crucibles*) | 40 | RÈGLE : `scps_manuf_legal` autorise pour ce pays les sorties FLUX et ESSENCE ⇒ l'Alambic et l'Atelier de mage se bâtissent — les INTRANTS des machines | — | Le salpêtre qui ne partira plus en poudre ; le cristal qui ne redeviendra jamais pierre. |
| 3 | **Le Pacte** (*The Pact*) — **VERBE** | 48 | RÈGLE, trois volets : (a) le refus `bld_is_faustian()` tombe pour ce pays ⇒ Foreuse arcanique · Réplicateur ligneux · Corne d'abondance bâtissables ; (b) le feed-check s'élargit à « intrant produit par une manufacture de l'empire » (sans quoi Foreuse/Réplicateur restent illégales) ; (c) `FAUST_BRECHE_CAUTION` ne s'applique JAMAIS au joueur — l'échappatoire est toujours offerte, jamais déclinée à sa place | — | On ne t'empêchera plus. C'est tout ce que le pacte accorde. |
| 4 | **L'Or du Puits** (*The Gold of the Shaft*) | 54 | Panier précieux de la Foreuse ×1.30 — l'or passe par la redevance de frappe existante ⇒ réserve → frappe → IPM : **ta propre monnaie se débase, émergemment** | **Σ essence brûlée du pays ≥ 60** | Le puits rend de l'or. L'or ne vaut plus rien. Le puits rend plus d'or. |
| 5 | **La Terre changée** (*The Changed Earth*) | 58 | `FAUST_MUTATION_K` ×1.25 (croissance démo du Réplicateur, entrée DÉMO) · `CHARGE_DECAY` ×0.65 — dans TOUTES les provinces du pays, la charge ne se lave plus qu'à 65 % de sa vitesse | — | La terre nourrit mieux qu'elle n'a jamais nourri, et ne se lave plus. |
| 6 | **Le Prix consenti** (*The Price Accepted*) | 60 | `FAUST_YIELD_MULT` ×1.25 (sortie des 3 transmuteurs ×2.5) · `FAUST_SPAWN_CHARGE` ×1.50 (0.225 de charge/unité) | **≥ 1 province à charge faustienne ≥ 3.0** | Tu as vu ce que ça coûte. Tu en redemandes. |

**Notes (agent) :** le revers est STRUCTUREL — le hook de charge unique fait
que toute hausse de sortie augmente mécaniquement les deux termes de charge,
et le rare brûlé CHOISIT la fin (essence→EAU · flux→RONCES · fer
céleste→FROID) · la séquence EST la chaîne de production (2 fabrique les
intrants, 3 ouvre les machines, 4 exige qu'elles aient tourné) — effet
heureux : la fin EAU redevient atteignable par décision joueur · « abandon
sans cicatrice » COHÉRENT ici : l'entropie mondiale est un accumulateur
monotone — on peut abandonner la doctrine, on ne peut pas dé-signer · les
malus sortent délibérément de la bande ×0.8-1.3 (l'identité) · leviers
écartés : `FAUST_*_DIV` (lus à la genèse seulement), `ENTROPY_*` (grain
MONDE, pas de cid — une doctrine ne déplace pas la barre de tout le monde,
elle la remplit plus vite) · ⚠ PIÈGE IA : `scps_ai.c:1242` saute les
transmuteurs inconditionnellement — une IA adoptante ne bâtirait jamais rien
⇒ atterrir joueur-seul (recommandé), symétrie IA en vague séparée · 9
tunables `DOCFAUST_*` neufs, kill-switch prouvé au motif FAUSTIEN 2026-07-16.

**Synergies candidates :** Faustien×Technologie « Le Concile noir » (⚠ EN
CONFLIT avec le Serment de sobriété de Technologie — voir harmonisation) ·
Faustien×Colonisation « Le Pain des lointains » (les Cornes d'abondance
nourrissent le front colonial : `COLONY_FOOD_GATE` ×0.70 national, et
`faust_consumed[2]` court vers le FROID) · Faustien×Divin « L'Autel de la
Brèche » (la charge nourrit la Ferveur : `PROVMOD_FERVEUR_K` ×1.30 dans les
provinces à transmuteur — ce qui damne le monde sanctifie ton règne).

---

# LES QUATRE COURANTS POLITIQUES (un seul à la fois)

---

## Aristocratique — Le Sang et la Terre (courant)

**Identité :** l'État n'a pas d'administrés : il a des maisons — on ne
gouverne pas un peuple, on tient des serments, et le prix de cette paix est
payé par ceux qui ne portent pas de nom.

*(Assiette : élites ×0.0025/mois × rang du Conseil · adoption ~100 · idées 275)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Le Serment des bannerets** (*The Bannermen's Oath*) | 30 | `AI_VASSAL_CONTRIB_BASE` ×1.25 · `AI_VASSAL_CONTRIB_GATE` ×0.85 — ouvre un flux plus qu'un chiffre (⚠ même canal que Vassaux idée 2 : voir harmonisation + le bug de création à corriger d'abord) | — | Le vassal ne paie pas un impôt : il tient une promesse. |
| 2 | **Les Offices héréditaires** (*Hereditary Offices*) | 35 | `COUNCIL_PAY_ADJ` ×1.30 (la solde achète plus de loyauté) · REVERS `COUNCIL_DISMISS_GRIEF` ×1.50 (congédier une maison = injure de génération) | — | Le siège appartient à la lignée ; on l'achète cher et on ne le reprend jamais gratuitement. |
| 3 | **L'Adoubement** (*The Accolade*) — **VERBE** | 45 | `CMD_ADOUBE(pid)` : transfert bourgeois→élite via la primitive existante `mobility_move`, volume `ADOUBE_FRAC` (déf. 0.10) de la strate, plancher 30 âmes, REFUSÉ au plafond doux `SHARE_CAP_ELITE` ; prix 8 influence + or = 1 an de panier-élite de la cohorte (sortie de trésor) ; cadence `ADOUBE_CD_DAYS` (déf. 720, sérialisé) | **Conseil de rang moyen ≥ II** | On ne naît pas noble : on le devient un genou à terre, et le royaume s'en souviendra une génération plus tard. |
| 4 | **Le Registre des fiefs** (*The Roll of Fiefs*) | 50 | `EDI_ELITE_JOBS` ×1.35 (100→135 positions par édifice d'élite × tier) — les POSITIONS absorbent les aspirants avant que la rivalité turchinienne ne les jette dans la rue | **≥ 3 provinces à édifice d'élite** | Un cadet sans charge est une guerre civile qui attend son heure. |
| 5 | **Le Ban et l'arrière-ban** (*The Call of the Ban*) | 55 | `ArmyDoctrine.moral_mul` ×1.15 · REVERS `INCOME_TAX_RATE_ELITE` ×0.75 (le trésor perd sa meilleure assiette) | — | Le noble ne paie pas : il meurt à cheval. C'est l'arrangement, et il coûte au royaume plus qu'il ne le croit. |
| 6 | **Le Sang ne se partage pas** (*Blood Is Not Shared*) | 60 | *NOUVEAUX TUNABLES (promotions de `#define`, `scps_econ.c:3672`) : `PROMOTE_BASKET_MULT_ELITE` déf. 2.5* ×0.75 (la noblesse s'ouvre au bourgeois fortuné) · *`PROMOTE_BASKET_MULT` déf. 1.4* ×1.30 REVERS (le journalier ne monte plus) | — | La société se referme par le haut et par le bas à la fois : très peu de portes, très bien gardées. |

**Notes (agent) :** la boucle turchinienne est LE sujet — idée 3 fabrique des
aspirants, idée 4 des positions, idée 6 rouvre le robinet ; 3+6 sans 4 =
élite pléthorique ⇒ 1789 est la facture, pas un bug : le seul courant où l'on
peut se ruiner en réussissant · `SHARE_CAP_ELITE` et `PROMOTE_SAT_GATE`
intouchés (le verbe REFUSE au plafond) · idée 5 : attendre −8 à −15 % de
revenu d'État an 120 — si banqueroutes en série au sweep, ×0.85 plutôt que
compenser (le revers doit mordre).

**Synergies candidates :** Aristocratique×Vassaux « Le Concordat des
maisons » (un adoubement gratuit en influence sur une province d'un vassal
intégré) · Aristocratique×Offense « La Chevalerie » (`CAV_PURSUIT` ×1.15 ·
`CAV_CUREE_CAP` ×1.15 tant qu'un édifice d'élite d'épée est tenu) ·
~~Aristocratique×Divin~~ INVALIDE (deux courants — voir harmonisation).

---

## Bourgeoise — La Charte des villes (courant)

**Identité :** l'État cesse de gouverner ses villes et se met à traiter avec
elles — sa voix politique n'est plus portée par le sang mais par le nombre de
ceux qui ont réussi.

*(Assiette : bourgeois ×0.0006/mois — *NOUVEAU TUNABLE
`INFLUENCE_PER_BOURGEOIS`* · gate : Marché + Banque bâtis · adoption ~100 ·
idées 262)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **La charte urbaine** (*The Town Charter*) | 30 | `ADMIN_BASE` ×0.85 — « coût administratif −15 %/mois : la ville se gouverne elle-même » | — | La couronne vend le droit de se passer d'elle, et y gagne : un échevin coûte moins cher qu'un bailli. |
| 2 | **Le registre des métiers** (*The Register of Trades*) | 35 | `COMMERCE_W_BOURGEOIS` ×1.20 · REVERS `PROMOTE_BASKET_MULT` ×1.10 (la guilde ferme sa porte derrière elle) | — | La jurande codifie la qualité, tient les prix — et ferme sa porte derrière elle. |
| 3 | **L'emprunt intérieur** (*The Domestic Loan*) — **VERBE** | 40 | `CMD_LOAN_DOMESTIC` : 2e tranche de crédit au-dessus de la ligne ordinaire, dimensionnée *`CREDIT_LINE_DOMESTIC` (déf. 1.0)* × effectif bourgeois ; taux ×0.85 si satisfaction bourgeoise ≥ *`CREDIT_DOMESTIC_TRUST` (déf. 0.5)*, sinon ×1.20 (ils tarifent le risque de leur propre État) ; canal crédit existant (dette, intérêts, saisie), compte dans `CREDIT_RATIO_CAP`. RÈGLE-REVERS : banqueroute tranche ouverte ⇒ grief plein du groupe bourgeois + tranche fermée 10 ans | **≥ 500 bourgeois** | On ne mendie plus chez l'étranger : on souscrit chez soi, et l'on doit désormais des comptes à ses propres marchands. |
| 4 | **Le denier des villes** (*The Towns' Penny*) | 45 | `CREDIT_RATE_BASE` ×0.80, toutes lignes | **Un emprunt intérieur remboursé intégralement** (latch) | Un prince qui rembourse emprunte deux fois moins cher que celui qui promet. |
| 5 | **La bourgeoisie de robe** (*The Robe and the Bench*) | 52 | RÈGLE : le vivier des ministres du Conseil s'étend à la strate bourgeoise · REVERS `COUNCIL_ROT_BOOST` ×1.25 (la vénalité rend la capture d'État plus corrosive). ⚠ INCERTITUDE signalée : si le vivier est DÉJÀ ouvert (ministres de la pop v100), repli à iso-coût = un siège de Conseil supplémentaire ouvrable — à trancher avant code | — | Le fils du drapier porte la robe du chancelier ; la vieille noblesse compte les sièges qu'elle a perdus. |
| 6 | **Les clés de la ville** (*The Keys to the City*) | 60 | `PROMOTE_BASKET_MULT` ×0.75 (seuil net 1.4×1.10×0.75 = 1.155) — l'assiette GROSSIT d'elle-même : plus de bourgeois = plus de voix, mais un panier bourgeois plus large à servir et plus d'aspirants face aux élites (conséquence endogène, aucun levier ajouté) | — | La charte s'ouvre à qui peut payer son entrée : la ville se remplit de nouveaux riches, et il faut désormais tous les servir. |

**Notes (agent) :** l'assiette est un pari sur le temps — plus mince que
l'aristocratique en début de partie, elle la dépasse quand l'urbanisation
prend ; l'idée 6 est le vrai moteur (elle ne multiplie pas le taux, elle
multiplie la CLASSE) ; surveiller l'emballement tardif (le frein propre est
`INFLUENCE_CAP`, pas un rabot du taux) · zéro création : l'idée 3 élargit le
plafond du canal crédit en changeant sa contrepartie et son prix · tranche +
latch = sérialisés, savetest.

**Synergies candidates :** Bourgeoise×Commerce « La Hanse des chartes »
(portée du marché +1 saut entre villes chartées · péage vers Centre tiers
−25 %) · Bourgeoise×Production « La Manufacture privilégiée »
(`MANUF_UPKEEP_DAY` ×0.75 · `HOUSE_MANUF` ×1.20) · Bourgeoise×Diplomatie
« Les facteurs de la couronne » (envoi diplomatique −5 d'influence, la
différence payée en couronnes).

---

## Populaire — La Voix du grand nombre (courant)

**Identité :** le pouvoir ne descend plus du sang ni de la bourse : il monte
de la foule qui travaille — et l'État qui la nourrit, l'écoute et l'arme y
puise sa légitimité comme sa force.

*(Assiette : journaliers ×0.00012/mois · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **Les doléances** (*The Grievance Rolls*) | 30 | `POL_SAT_W` ×1.20 (ta politique pèse plus dans la satisfaction du peuple — DANS LES DEUX SENS) · `W_AGITATION_UNREST` ×0.85 | — | On tient registre des plaintes des villages, et le prince est tenu de les lire. |
| 2 | **Le pain avant l'impôt** (*Bread Before Taxes*) | 36 | `TAX_EXEMPT_BASKET_MULT` ×1.30 (le fisc ne mord qu'au-dessus de 1,3× le panier vital) · `POP_SAT_W` ×1.25 | — | Ce qui tient un homme debout n'est pas matière à taxe ; ce qu'on ne prend pas revient en berceaux. |
| 3 | **La levée en masse** (*The Nation in Arms*) — **VERBE** | 42 | `CMD_LEVEE_MASSE` (national, 5 ans, cooldown 10 ans) : (a) la jauge MASSE s'ouvre SANS la tech Conscription ; (b) *`SOLDE_OVER_K` ×0.35* (promotion du `#define` déjà réclamée par le carnet warhost) ; (c) PRIX : *`LEVEE_MASSE_AGIT` déf. 1.5 pt/mois* sur l'agitation des journaliers de CHAQUE province. ⚠ chez le joueur, `warhost_tick` saute la mobilisation auto : le verbe doit mordre au site de la SOLDE/limite de force, jamais sur `wh_levy_batch` — sinon bouton mort | Caserne bâtie | Le ban et l'arrière-ban : le royaume porte plus d'hommes qu'il n'en peut solder, et il le sait. |
| 4 | **Céder à temps** (*Yield Before Blood*) | 48 | Petit verbe `CMD_CONCEDE(pid)` : la concession AVANT la révolte — réemploi EXACT du bloc existant (`scps_revolt.c:955-967`) ; + `CONCEDE_GOLD` ×0.70 | **Une révolte close en CONCESSION, jamais écrasée** | Le sac de blé livré la veille coûte moins cher que la ville reprise le lendemain. |
| 5 | **L'impôt du rang** (*The Tax of Rank*) — **LE REVERS** | 54 | `INCOME_TAX_RATE_ELITE` ×1.20 · `EDI_ELITE_POP_PCT` ×0.85 — le revers ÉMERGE, aucun levier neuf : satisfaction élite ↓ ⇒ loyauté des ministres d'élite ↓ (`COUNCIL_CLASS_SAT_W`), aspirants sans positions ⇒ révolte élitaire (Turchin) | — | On ne ferme pas la porte des rangs sans que ceux qui attendaient dehors ne la poussent. |
| 6 | **La souveraineté du nombre** (*The Sovereignty of Number*) | 60 | RÈGLE : céder cesse d'être une plaie — `C3_K_HOLLOW` ×0.25 · `C3_L_HOLLOW` ×0 (la légitimité ne baisse plus) · *`CONCEDE_CD_DAYS` (promotion) ×0.40* (verrou ~4 ans au lieu de 10) | **Satisfaction moyenne des journaliers ≥ 0.60 tenue 10 ans** (streak — ⚠ `g_lowsat_streak` est un static non sérialisé : à migrer sur la struct) | Ailleurs, plier c'est perdre ; ici, plier EST le titre à régner. |

**Notes (agent) :** l'assiette est LE chiffre à mesurer avant de figer —
×0.00012 vise la parité avec ~1000 élites, il faut 30 000 journaliers ; si
l'empire moyen n'en tient que ~10 000, monter vers 0.0003 : MESURER au
chronicle, pas deviner · idée 5 = point de rupture candidat (deux mults dans
le même sens sur le mécontentement élitaire) — premier nerf :
`EDI_ELITE_POP_PCT` ×0.92 · écarté : `WILD_DEFECT_YEARS` (défaut registre
0.0 ≠ les 8 ans de LEVIERS.md — levier mort ou doc périmée, à trancher en
ticket).

**Synergies candidates :** Populaire×Infrastructure « Les Grands Travaux »
(`RENOV_COST_FRAC` ×0.75 tant que la satisfaction du peuple tient) ·
Populaire×Offense « La Nation en armes » (en guerre DÉFENSIVE, la levée ne
verse plus son agitation) · Populaire×Production « Les Bras de l'atelier »
(`EXTRACT_LABOR_SHARE` ×1.10).

---

## Divin — Le Trône et l'Autel (courant)

**Identité :** le seul courant où la voix politique ne monte pas d'une classe
mais de la pierre consacrée : ton influence est ce que ton peuple bâtit et
brûle pour son dieu — et tu paies cette puissance en fragilité.

*(Assiette : `INFLUENCE_PER_FAITH` (déf. 0.08) × Σ foi bâtie ×
(1 + ferveur moyenne) × rang du Conseil — un empire médian ≈ 3,9/mois, 6,6
en sortie d'Appel à la foi puis retombée : une assiette VOLATILE, pas une
rente · gate : un Temple debout · adoption ~100 · idées 270)*

| # | Idée FR (EN) | Coût | Effet mécanique exact | Prérequis d'usage | Saveur |
|---|---|---|---|---|---|
| 1 | **L'Onction** (*The Anointing*) | 30 | *NOUVEAU TUNABLE `LEGIT_K_FAITH` déf. 0.70* (promotion du `#define K_FAITH`, `scps_legitimacy.c:21`) ×1.25 — la foi bâtie contre l'ombre coercitive dans L | — | Le prêtre pose la couronne : ce que le temple sanctifie, la garnison n'a plus à le tenir. |
| 2 | **La Ferveur entretenue** (*Tended Fervor*) | 36 | `PROVMOD_FERVEUR_K` ×1.20 · `PROVMOD_FERVEUR_DECAY` ×0.75 (~20 ans au lieu de ~15) | — | Une flamme qu'on alimente ne s'éteint pas. |
| 3 | **Le Sacerdoce** (*The Priesthood*) | 42 | RÈGLE double : (a) le porteur recrute un Missionnaire quel que soit son crédo ; (b) le plafond mondial ⌈N/2⌉ de fondation ne le borne plus (il borne toujours les autres) | — | L'État ne demande pas la permission du siècle : il fait ses prêtres et, s'il le faut, son dieu. |
| 4 | **L'Appel à la foi** (*The Call to Faith*) — **VERBE** | 48 | `CMD_FAITH_CALL` (pays, bras au choix) : ferveur 1.0 posée sur toutes les provinces ORTHODOXES (les hétérodoxes n'entendent pas l'appel). Bras Concorde : `W_AGITATION_UNREST` ×0.70 pendant 5 ans. Bras Zélotes : plancher de levée +1 niveau 5 ans ET `RELIG_MINORITY_SAT` ×1.50 (le prix). Cadence `FAITH_CALL_CD_DAYS` 3650 | **Foi fondée + ≥ 1 Temple** | Du haut de la chaire, le même mot : « Priez » ou « Prenez les armes » — et le royaume entier se lève. |
| 5 | **Le Clergé d'État** (*The State Clergy*) | 54 | RÈGLE + mult : DEUX lettrés simultanés (2e slot sérialisé, section RELG change) · *`SCHOLAR_DURATION_DAYS` (promotion, déf. 1825)* ×1.40 (~7 ans de mission) | **≥ 6 régions professant la foi d'État** | Le royaume a un corps de clercs : deux routes de mission ouvertes, et le temps de les mener. |
| 6 | **L'Orthodoxie** (*The Orthodoxy*) | 60 | `AI_DERIVE_ODDS` ×2.00 (la Réforme ne mûrit presque plus sur tes marches) · REVERS `RELIG_MINORITY_SAT` ×1.60 (le culte minoritaire gronde deux fois plus fort) | **Une Cathédrale bâtie** | Une seule vérité sous le ciel : plus rien ne dérive, mais tout ce qui n'est pas elle devient une poudrière. |

**Notes (agent) :** le revers est structurel — (1) l'assiette est VOLATILE
(un Temple saccagé, un schisme, et le courant devient MUET en diplomatie au
pire moment, puisque l'influence paie les émissaires) ; (2) l'axe religion
des 5 axes de culture pénalise déjà l'hétérodoxe (plancher
`FAITH_BRANCH_PEN` 3.5) et l'idée 6 empile dessus : l'orthodoxie n'achète pas
la paix, elle achète l'absence de dérive au prix d'une fracture plus dure ;
(3) l'Appel ne touche que les orthodoxes — un empire à moitié hétérodoxe rend
moitié moins · tension de grain ASSUMÉE : la religion vit au grain région,
mais les deux contacts (assiette par province, `RELIG_MINORITY_SAT` sur
owner) sont conformes ; verbe et idée 5 visent des pid · 3 `#define` à
promouvoir avant code · sweep : part de monde converti an 180, schismes,
influence/mois médiane, révoltes hétérodoxes.

**Synergies candidates :** Divin×Offense « La Guerre sainte » (le bras
Zélotes arme un CB religieux GRATUIT contre toute branche de foi différente ·
moral ×1.15 tant que la ferveur de la capitale > 0.5) · Divin×Peuple « La
Conversion des âmes » (`ASSIM_K_INST_AMP` ×1.25 sur les régions passées à la
foi d'État — rembourse une part du revers de l'Orthodoxie) ·
Divin×Infrastructure « L'Œuvre de pierre » (*`VETUSTE_RATE_FAITH`* ×0.5 : la
famille Sanctuaire→Temple→Cathédrale cesse de vieillir — l'assiette cesse
d'être rongée).

---

# HARMONISATION (passe d'orchestrateur, 2026-09-01)

## H1. Prix

Totaux d'idées par doctrine : 262-293 (moy. ~275) + ~100 d'adoption. Bande
homogène, rampes cohérentes (30→60). Rien à retoucher — les deux doctrines
« bon marché » (Infrastructure 270, Bourgeoise 262) l'assument par design
(endurance vs assiette).

## H2. Collisions de leviers entre doctrines CUMULABLES

Les courants étant exclusifs entre eux, leurs collisions internes
(`INCOME_TAX_RATE_ELITE` Aristocratique ×0.75 vs Populaire ×1.20,
`PROMOTE_BASKET_MULT` en sens contraires) sont impossibles — saines. Restent
les vraies compositions :

| Clé | Sources | Composé | Verdict |
|---|---|---|---|
| `BUILD_RESERVE_BULK` | Infra ×1.30 · Mercantilisme ×1.30 | ×1.69 | à clamper (H2bis) |
| `METAB_TIER1/2` | Peuple ×0.75 · Connaissances ×0.80 | ×0.60 | accepté (identité « nation-monde » cumulée) — MAIS voir H3.1 |
| `AI_METAB_RES_W` | Peuple ×1.20 · Connaissances ×1.40 · 2 synergies | ×2.3+ | à clamper |
| `AI_VASSAL_CONTRIB_BASE` | Vassaux ×1.20 · Aristocratique ×1.25 | ×1.50 | à clamper + BLOQUÉ par le ticket création (H4.1) |
| `SIEGE_LOOT_FRAC` | Offense ×1.30 (attaquant) · Défense ×0.60 (victime) | ×0.78 émergent | SAIN — portées opposées, c'est le duel voulu |
| `W_AGITATION_UNREST` | Populaire ×0.85 · Appel à la foi ×0.70 (5 ans) · Grands Travaux ×0.85 | ×0.51 | à clamper |
| `EXTRACT_LABOR_SHARE` | Production ×1.12 · Bras de l'atelier ×1.10 | ×1.23 | OK sous clamp |
| `HOUSE_MANUF` | Infra ×1.25 · 2 synergies ×1.15/×1.20 | ×1.7+ | à clamper |

**H2bis — RÈGLE GÉNÉRALE proposée** : le multiplicateur composé de doctrine
par clé (`doctrine_mult` × synergies actives) est **clampé [0.60, 1.60]** au
site de lecture — précédent exact : `ai_tech_tradition_mult` clampe
[0.5, 2.0]. Exception : la famille `DOCFAUST_*` sort de bande PAR IDENTITÉ
(`CHARGE_DECAY` ×0.65, `SPAWN_CHARGE` ×1.50) — ses malus ne se composent
avec rien.

## H3. Décisions d'harmonisation (contradictions entre agents)

1. **`METAB_TIER3`** : Peuple idée 6 le touche (×0.75), Connaissances
   l'épargne EXPRÈS (il gate la victoire Merveille). **Décision : Peuple
   idée 6 restreinte à TIER1/2** — personne ne brade une condition de fin.
2. **Faustien × Technologie** : le « Concile noir » (Faustien) contredit
   frontalement le « Serment de sobriété » (Technologie idée 6). **Décision :
   la paire reste VIDE** — c'est l'énoncé des deux doctrines.
3. **Aristocratique × Divin « L'Onction »** : INVALIDE — deux courants ne se
   cumulent jamais. Supprimée (le nom vit déjà comme idée 1 du Divin).
4. **Nom « Concile des lettrés »** vs apex T5 « Concile des savants » :
   collision d'écrans voisins — à trancher (renommer la doctrine
   « L'Atelier des lettrés » ? ou l'apex ?).
5. **Doubles propositions sur la même paire** (chaque agent proposait de son
   côté) : fusionnées dans la table H5.

## H4. Tickets HORS-VAGUE (trouvailles des agents, indépendantes des doctrines)

1. ~~Contribution vassale : création de grain et d'armes~~ — **CORRIGÉ
   2026-09-01** : miroir M3f porté sur les canaux agraire et martial
   (`scps_diplo.c`, débit réel borné au stock du vassal) + banc de
   conservation dans `diplo_demo.c` (Σ-monde grain/mil conservées). Golden
   intact (la contribution mûrie démarre après l'an 13, hors fenêtre 12 ans).
2. ~~`IMPORT_TOLL_FRAC` mort~~ — **PURGÉ 2026-09-01** : entrée registre +
   `#define` orphelin supprimés (aucun site de lecture, golden-neutre) ;
   LEVIERS.md documente le péage réel (toute la marge, répartie par
   `TOLL_STATE_SHARE`).
3. ~~`WILD_DEFECT_YEARS` 0.0 vs doc~~ — **TRANCHÉ 2026-09-01** : le 0 est une
   décision joueur documentée au site (`scps_sim.c:86` : les Peuples libres ne
   rallient jamais seuls ; >0 ré-arme). C'était LEVIERS.md qui était périmé —
   corrigé.
4. À vérifier en début de vague : `arch_depth[]` sérialisé ? vivier des
   ministres déjà ouvert aux bourgeois (v100) ? `SPEC_*` bien lu par-pays
   sous IA AUTO joueur ?

## H5. TABLE FINALE DES SYNERGIES (fusionnées — le sous-menu §4.4)

Entretien fibonaccien : 1re active 2/mois, puis 3 · 5 · 8… Une paire absente
de cette table n'affiche rien.

| Paire | Synergie | Effet (1 ligne) |
|---|---|---|
| Commerce × Aristocratique | **Maison de commerce** (canon joueur) | chaque comptoir franc debout siège : `INFLUENCE_PER_NOBLE` ×1.15 |
| Commerce × Diplomatie | **Les Traités de commerce** | `TRADE_LEVY` ×0.80 sur les routes vers un partenaire de pacte/alliance |
| Commerce × Connaissances | **La Route des langues** | `SYNC_TRADE_SEA_W`/`_LAND_W` ×1.25 — nos comptoirs deviennent des ambassades |
| Commerce × Bourgeoise | **La Ligue des villes** | `COMMERCE_BLD_MAX` ×1.40 · péage versé à un Centre tiers −25 % |
| Mercantilisme × Production | **La Manufacture d'État** | le dispatch préempte la sortie des manufactures à parité fixe · `RAW_BOOST_PER_TIER` ×1.25 sur la brute d'Étape seule |
| Mercantilisme × Colonisation | **Le Pacte colonial** | les colonies ne commercent qu'avec la métropole, leur surplus remonte à l'Étape · `COLONY_WEALTH_SHARE` ×0.6 |
| Mercantilisme × Bourgeoise | **La Compagnie privilégiée** | mon embargo exempte nommément mes marchands · `CREDIT_LINE_BASE` ×1.15 |
| Offense × Vassaux | **Les Marches d'épée** | inféoder une région occupée à la paix (cicatrice d'annexion sautée) · le vassal appelé verse sa contribution martiale sans gate |
| Offense × Défense | **Le Marteau et l'Enclume** | un corps ami relevant un siège tenu ≥ 6 mois entre au choc en DÉFENSEUR |
| Offense × Divin | **La Guerre sainte** | CB religieux gratuit contre toute branche de foi différente · moral ×1.15 tant que la ferveur de la capitale > 0.5 |
| Offense × Aristocratique | **La Noblesse d'épée** | un siège mené à terme déclenche l'adoubement sans coût d'influence |
| Offense × Populaire | **La Nation en armes** | en guerre défensive, la levée en masse ne verse plus son agitation |
| Offense × Production | **L'Arsenal du royaume** | `MANUF_ARMS_MULT` ×1.3 — l'industrie devient directement de l'ost |
| Défense × Infrastructure | **Les Marches de pierre** | la file de rénovation sert d'abord la frontière · `VETUSTE_RATE` ×0.5 sur la famille fortifiée |
| Défense × Populaire | **La Levée des paroisses** | le ban lève des gardes-escorte (ancre défensive) au lieu de milice, contre `W_AGITATION_UNREST` ×1.15 en guerre |
| Diplomatie × Vassaux | **Le Concert des princes** | `AI_OFFER_SUZ_OPINION` ×0.8 · `AI_VASSAL_CONTRIB_GATE` ×0.85 · une ligue ne se noue pas tant qu'un serment frais court chez le meneur |
| Diplomatie × Peuple | **La Cour cosmopolite** | `OPINION_PACT` 15→22 · flux de pacte d'alliés ×1.2 — chaque diaspora est une ambassade |
| Vassaux × Peuple | **Les Noces des maisons** | sous pacte migratoire avec un vassal, son intégration lit `0.5+0.5·prox` au lieu de `0.3+0.7·prox` |
| Vassaux × Aristocratique | **Le Concordat des maisons** | un adoubement gratuit en influence sur une province d'un vassal intégré |
| Colonisation × Connaissances | **Les Grandes Découvertes** | le chantier colonial devient légal HORS adjacence sur toute zone révélée par expédition · portée navale ×1.5 |
| Colonisation × Peuple | **Les Terres promises** | `MIG_ATTRACT_INST_W` ×1.3 · la ferveur fondatrice dure ~21 ans au lieu de 15 |
| Colonisation × Faustien | **Le Pain des lointains** | `COLONY_FOOD_GATE` ×0.70 tant qu'une Corne d'abondance tourne — le FROID approche |
| Technologie × Connaissances | **Le Grand Atlas** | `SYNC_TRADE_METIER`/`_PROFOND` ×0.80 — chaque route est un copiste |
| Technologie × Peuple | **Le Collège des langues** | `AI_METAB_RES_W` ×1.15 · `AI_TECH_DIFFUSE_MAX` 0.40→0.50 |
| Technologie × Bourgeoise | **Les Presses de la guilde** | `SAVOIR_W_BOURGEOIS` ×1.40 — la ville produit plus de recherche que la cour |
| Production × Infrastructure | **L'Atelier de pierre** | `HOUSE_MANUF` ×1.15 · `RAW_BOOST_COST` ×0.80 |
| Production × Populaire | **Les Bras de l'atelier** | `EXTRACT_LABOR_SHARE` ×1.10 — le choix se paie côté manufactures |
| Infrastructure × Populaire | **Les Grands Travaux** | `RENOV_SHARE_LAB` ×1.40 (l'or des chantiers aux journaliers) · `W_AGITATION_UNREST` ×0.85 |
| Peuple × Divin | **La Conversion des âmes** | `ASSIM_K_INST_AMP` ×1.25 sur toute région passée à la foi d'État |
| Infrastructure × Divin | **L'Œuvre de pierre** | la famille Sanctuaire→Temple→Cathédrale cesse de vieillir (`VETUSTE_RATE_FAITH` ×0.5) |
| Faustien × Divin | **L'Autel de la Brèche** | `PROVMOD_FERVEUR_K` ×1.30 dans les provinces à transmuteur — ce qui damne le monde sanctifie ton règne |

31 paires écrites sur C(17,2) = 136 possibles ; à élaguer si le joueur veut
plus rare (~15-20).

## H6. Tunables neufs consolidés

~35 propositions en 3 familles : (a) **promotions de `#define` existants aux
valeurs actuelles** — byte-neutres par construction (SIEGE_FOOD_MONTHS_FULL,
TOOLS_PER_LABORER, SOLDE_FL_PER_REG/OVER_K, CONCEDE_CD_DAYS, LEGIT_K_FAITH,
SCHOLAR_DURATION_DAYS, PROMOTE_BASKET_MULT[/_ELITE], STOCK_CAP_ENTREPOT,
STOCK_DECAY_PERISH, BUILD_EXTENT_K, CLIM_LEARN_INTEG, COLONY_YIELD_HREF,
FOG_SEA_HALO, OFF_CULTURE_SAT/SOC_PEN) ; (b) **paramètres de l'économie
d'influence** qui devront exister en P1 de toute façon (INFLUENCE_PER_NOBLE/
_BOURGEOIS/_FAITH, INFLUENCE_COST_ENVOY, DIPLO_ENVOY_SLOTS/FLOOR_DAYS) ;
(c) **paramètres de verbes neufs** (BAN_MILICE_*, ADOUBE_*, EXPEDITION_*,
FAITH_CALL_*, LEVEE_MASSE_AGIT, RENOV_MASS_SLOTS, BUILD_MAT_MULT,
BUILD_COST_MULT_FORT, COMPTOIR_TOLL_SHARE, CREDIT_LINE_DOMESTIC/TRUST,
AI_OFFER_SUZ_OPINION, DOCFAUST_* ×9). Chaque famille porte son kill-switch ;
la preuve « tous à neutre = golden byte-identique » est LE gate d'entrée de
la vague P3.

## H7. À valider par le joueur

1. La règle de clamp composé [0.60, 1.60] (H2bis).
2. Peuple idée 6 restreinte à `METAB_TIER1/2` (H3.1).
3. Paire Faustien×Technologie vide (H3.2).
4. Le renommage (doctrine « Concile des lettrés » vs apex, H3.4).
5. Le volume de synergies (31 écrites — garder tout, ou élaguer ?).
6. Les 3 tickets hors-vague (H4) à lancer quand tu veux.
