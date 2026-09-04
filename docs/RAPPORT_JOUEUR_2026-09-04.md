# RAPPORT JOUEUR — une partie, graine 7, 104 ans

> Partie jouée à travers le VRAI shell (`Main.tscn`), probe `godot/project/player_session.gd`,
> 24 captures dans `godot/project/shots_player/`. Je juge ce que l'ÉCRAN montre.
> Pays joueur : **Ligue Gualredor** (pays 5), capitale **Bois Profond** (prov. 23),
> 1 province et 4 750 âmes à l'an 0 → 10 provinces et 19 427 âmes à l'an 104.

---

## 1. La partie, en 15 lignes

J'ouvre sur un monde noir : une tache de terre grande comme une pièce au milieu de l'écran
(`01_carte_an0.png`). Je suis une ligue de 4 750 âmes, « Submergée », « Misère », « Usurpée ».
An 6, le jeu m'interrompt : des cousins d'un autre monde demandent l'ambassade — je renoue
(`02_decision.png`). Je clique ma capitale : un grenier tropical de collines, 4 724 âmes
(`03_…`). Je bâtis un **Tribunal** (6 couronnes, 180 jours) — deux tours plus tard la fiche
affiche « Capacité admin. 10 », mon premier effet visible (`04_`/`05_`). Je monte une
**Scierie navale** (`06_`). Je pousse mes journaliers à 90 % sur les céréales : la fiche
répond « Céréales 100 % · Bois 0 % » et la classe tombe à 38 % employée (`07_`). An 26 :
mes greniers débordent (64 911 céréales) pendant que le poisson, le sel, les outils et les
armes sont à zéro (`08_`) — et le marché me propose de tout acheter à **0,00 couronnes**
(`09_`). Le Trésor confirme : 2 456 couronnes, **+0/mois**, toutes les lignes vides (`10_`).
An 36, j'adopte **Offense** et j'achète l'idée « Arsenaux » : le panneau affiche « 1/6 » et
rien d'autre (`11_`/`12_`). L'arbre de technologie, lui, me parle vraiment : Scriptorium à
35 %, 265 points, trois couloirs illustrés (`13_`). An 55, deux voisins me déclarent la
guerre sans que je l'aie vu venir ; je propose un pacte au Clan Wyntonor — accepté, aucune
trace à l'écran (`14_`/`15_`). Je recrute, je lève le Corps #5 (200 hommes) et je gagne des
batailles (`16_`/`17_`). J'active « Rations mesurées » (`18_`), je colonise la Tourbière
Verdoyante (`19_`). An 96 : 10 provinces, mais stabilité 18, prospérité 10, corruption 46, et
un Trésor où **chaque ligne de rentrée et de sortie affiche 0 couronnes/mois** (`20_`)
pendant que mon or tombe de 2 766 à 803. J'ouvre les Annales pour savoir ce que j'ai vécu :
**cinq lignes**, dont quatre disent seulement qu'un âge a commencé (`22_`).

---

## 2. Impressions par écran

### Topbar — `01_carte_an0.png`, `20_empire_economie.png` — **2/5**
Sept cellules, **aucun mot** : je dois deviner à l'icône que « 129 515 » est du grain et
« 236 » du bois. Trois cellules portent une ligne « /mois », quatre n'en ont pas — sans
règle apparente. Deux cellules restent **à 0 toute la partie avec un flux négatif** (« 0 ·
−830/mois », « 0 · −1034/mois ») : on ne peut pas vider un stock vide, le chiffre est faux
et il ne bouge jamais. La **date est illisible** : brun sombre sur brun sombre, en haut à
droite, je n'ai pas pu lire l'année sans zoomer sur le PNG. C'est la valeur la plus consultée
d'un jeu temps réel. Enfin la cellule « Influence » affiche **0** en permanence alors que le
panneau Doctrines dépense sur un stock de 1 821 : le même mot, deux nombres.

### Fiche province — `03_`, `07_province_apres_allocation.png` — **3/5**
Dense et bien rangée : terrain, âmes, loyauté, agitation, logement, et le pied Réprimer /
Assimiler / Purger / Détail. Mais l'en-tête dit « **tier 5** · Collines » — « tier » est un
mot de moteur, alors que le moteur me donne déjà « Cité ». La ligne « **Croissance** » est
**vide** toute la partie. « Impôts **−0 couronnes/mois** » : un zéro négatif, c'est la
signature d'un chiffre cassé. « Services **−471** / 5 563 » me montre un manque en négatif
sans me dire quoi faire. Et il y a **deux boutons « CONSTRUIRE… » identiques** dans le même
panneau, à 60 px l'un de l'autre, rien ne les distingue. Les chips ne sont pas cohérentes :
la manufacture bourgeoise a `[−][+]`, les deux chips d'élite n'ont que `[−]`.

### Menu Construction — `04_`, `05_`, `06_construction_manufactures.png` — **4/5**
La meilleure surface après l'arbre. Une carte par bâtiment : icône, prix, effet, entretien,
matières, « Déverrouille : Chancellerie ». Exactement la doctrine. Trois réserves : le prix
s'écrit « **6 · 180 j** » sans unité (6 quoi ?) ; **toutes les manufactures coûtent 51**, le
même nombre pour les six, donc le prix ne m'apprend rien ; et quand un bâtiment est refusé,
la raison est « **✕ indisponible ici (palier/déjà bâti)** » — le jeu me donne deux raisons
possibles parce qu'il ne sait pas laquelle. À côté, un **Marché** m'est proposé dans une
province qui en a déjà un, sans mention. Une recette dit « Bois ×2 + **Cuivre ×0.2** » : une
fraction d'unité, ça sent le flottant moteur.

### Doctrines / Influence — `11_`, `12_doctrines_apres.png` — **2/5**
Six cases, un « + » dans cinq d'entre elles, une illustration et « Offense · 1/6 » dans la
sixième. **C'est tout l'écran.** J'ai adopté une doctrine et acheté une idée : le seul retour
est « 1/6 ». Nulle part le coût de la prochaine idée (24), nulle part mon stock d'influence,
nulle part ce que fait « Arsenaux » — alors que le moteur me donne les six idées nommées avec
leur bonus. Le panneau ne dit ni ce que je viens de payer, ni ce que je peux payer ensuite.

### Pays / Diplomatie — `14_diplomatie_pays.png` — **3/5**
Très bon cadre : Habitants, Éthos, Statut politique, une jauge d'opinion −100/+100 avec sa
tendance, « Alliances : nous 0/2 · eux 0/2 », et surtout « Proposer une alliance ⚠ » suivi de
« **Refus probable** » — un retour honnête, exactement ce qu'il faut. Deux problèmes. Le
bouton « ⊕ Voir la capitale — **Prov.6** » affiche un identifiant de moteur en pleine
étiquette d'action. Et quand mon pacte est accepté, **aucune trace** : pas de ligne, pas de
délai d'émissaire, l'écran est identique avant et après. Les deux actions grisées disent
« Indisponible » — un non-motif, à côté du très bon « Refus probable ».

### Armée — `16_`, `17_panneau_armee.png` — **2/5**
Le tiroir dit « force mobilisée : 2 régiments · armée de campagne — **région 18** · Au repos ·
inf 200 · arch 0 · cav 0 · mages 0 (Σ 200) ». Un index de région et un sigma. « Composer
l'armée » aligne **huit icônes d'unité sans nom ni coût**, alors que le moteur porte pour
chacune son nom, son prix (« 100 Armes légères »), ses forces et ses faiblesses. Et le
panneau Corps #5 contient **un grand rectangle vert plat** de ~340×125 px, avec une petite
marque rouge dedans : ça ressemble à une texture manquante, pas à un dessin.

### Stocks / Marché — `08_tiroir_stocks.png`, `09_tiroir_marche.png` — **1/5**
Le point noir de la partie. **Stocks** : quatre colonnes « bien · stock · **net/j** · couv. » —
un débit **par JOUR** quand la doctrine impose le mois, et « couv. » qu'aucun joueur ne
déplie. La colonne « bien » ne contient **qu'une icône** : dix-huit lignes de pictogrammes
sans un seul nom, alors que le moteur donne « Céréales », « Poisson »… Les chiffres sont des
flottants bruts (+378.0, −13.0, +0.1). **Marché** : chaque ligne affiche « **0.00
couronnes** », les quinze. Et le prix **déborde sur la colonne d'état** : on lit
« 0.00 couronn**p**énurie sévère », « 0.00 couronne**s**gorgé ». Un marché où rien n'a de nom
et où tout vaut zéro, avec deux boutons « Acheter 10 / Vendre 10 » sous chaque ligne.

### Trésor / Empire — `10_tresor_balance.png`, `20_empire_economie.png` — **1/5**
Beau cadre, contenu mort. Le Trésor affiche « 2 456 · **+0/mois** », une courbe plate, et
sous RENTRÉES : Laboureurs 0/mois, Artisans 0/mois, Noblesse 0/mois, Export 0/mois, Péages
0/mois. Sous SORTIES, **huit étiquettes sans aucune valeur en face** — pas « 0 », rien du
tout, ce qui se lit comme un panneau cassé. La Fenêtre Empire, elle, remplit bien ces mêmes
lignes… avec « 0 couronnes/mois » partout. Or à ce moment la topbar affiche « 2 503
**−164/mois** » : **le grand livre dit zéro pendant que l'or bouge sous mes yeux.**

### Annales / évènements — `02_decision.png`, `22_annales.png` — **2/5**
La boîte de décision est belle (grande peinture, trois cartes de choix, flavor soigné) mais
elle **répète les deux mêmes phrases de contexte dans chacune des trois cartes** : je lis le
même paragraphe trois fois, et la seule ligne qui distingue les choix est la dernière. Un
effet s'affiche « légitimité **+0.2** » — un flottant. Le popup OYEZ, lui, met le **titre en
double** : titre « La forêt brûle jusqu'à l'horizon », corps « La forêt brûle jusqu'à
l'horizon — **région 21** (an 4) ». Les Annales enfin : « Règne de Ligue Gualredor, dit le
Discret · **5 fait(s) retenu(s)** » après 104 ans, trois guerres, des batailles gagnées, une
place prise, une colonie fondée — et quatre de ces cinq lignes disent seulement qu'un âge a
commencé. Le panneau est en plus posé à (0,0) : il recouvre la moitié gauche de la topbar et
coupe sa propre cinquième ligne.

### Bonus — Arbre de technologie — `13_technologie.png` — **4/5**
Le meilleur écran du jeu, à citer en modèle : légende de couleurs, « Recherche :
Scriptorium » avec sa barre à 35 %, « Points : 265 », trois couloirs illustrés, des cartes
nommées, une bande de métabolisation, et un état vide qui **explique quoi faire**
(« Sélectionnez une carte : effets, coût, prérequis et débouchés resteront affichés ici »).
Réserves : les cartes de droite sont coupées sans indice de défilement, et un « To » orphelin
traîne en haut.

---

## 3. Frictions classées

### BLOQUANT

**F1 — Tous les prix du marché valent 0,00 couronnes.**
*Écran* : Marché (`09_tiroir_marche.png`), Stocks (`08_`).
*J'attendais* : un prix par bien, qui monte en pénurie.
*J'ai eu* : « 0.00 couronnes » sur les quinze lignes, toute la partie.
*Ce que j'ai pu vérifier* : `market_quote()` renvoie bien un prix (0,20) et le marché de la
tuile aussi (0,0044) — c'est **le champ `price` de `country_stocks` qui reste à 0**, et c'est
lui que lit `sidebar_drawer.gd:580`.
*Proposition* : remplir ce champ depuis la même source que `market_quote` ; aucun composant
nouveau, la colonne existe déjà.

**F2 — Le grand livre affiche 0 partout alors que l'or bouge.**
*Écran* : Trésor → Balance (`10_`), Fenêtre Empire → Économie (`20_`).
*J'attendais* : mes rentrées et mes dépenses par mois.
*J'ai eu* : toutes les rentrées à 0/mois, toutes les sorties à 0/mois — pendant que la topbar
affiche « −164/mois » et que l'or passe de 2 766 à 803.
*Vérifié* : `budget_summary()` renvoie `income 0.0 / expense 0.0`, et **`country_budget()`
renvoie une liste vide**, ce qui explique les huit étiquettes sans valeur du Trésor.
*Proposition* : c'est un trou de lecture, pas d'affichage — les deux panneaux sont prêts.

**F3 — Pousser une allocation à 90 % en met une à 100 et met la classe au chômage.**
*Écran* : fiche province (`07_province_apres_allocation.png`).
*J'attendais* : Céréales 90 %, Bois 10 %.
*J'ai eu* : « Céréales 100 % · Bois 0 % », et l'en-tête de classe passé de 100 % à **38 %** —
62 % des journaliers ont disparu de l'écran sans explication.
*Proposition* : que le poids demandé et le pourcentage affiché coïncident, et que le reliquat
de main-d'œuvre soit nommé quelque part sur la fiche.

**F4 — Une ligne sur deux du rail droit est illisible.**
*Écran* : NOTIFICATIONS, VILLES, JOURNAL (`17_panneau_armee.png`, `07_`).
*J'attendais* : de lire mes 18 notifications.
*J'ai eu* : les lignes paires lisibles, les impaires quasi invisibles — le zébrage
(`empire_sidebar.gd:559`, `VKit.list_row_bg`) alterne un bandeau clair et le fond sombre du
panneau, mais **la couleur du texte ne suit pas** : le même brun foncé sur les deux. Le
JOURNAL, lui, tombe entièrement sur le fond sombre : **ses 9 lignes sont toutes illisibles**,
alors que c'est la seule trace durable des évènements.
*Proposition* : basculer la couleur du texte avec le bandeau (une condition sur la parité,
au même endroit), ou retirer le zébrage.

### GÊNANT

**F5 — « région 21 », « Prov.6 », « région 18 » : des identifiants moteur en face joueur.**
*Écrans* : popup OYEZ (`02_decision.png`), fenêtre diplomatique (`14_`), tiroir Armée (`16_`).
*Origine trouvée* : `ui/alerts.gd:160` — `.replace("{r}", str(int(ev["region"])))` colle
l'index brut dans le texte de tout évènement du fil. « Prov.N » est le nom de repli d'une
province jamais nommée, et il apparaît **dans une étiquette de bouton** (« Voir la capitale —
Prov.6 »).
*Proposition* : passer par le nom (`region_city_name` / `province_info().nom`) au point de
substitution — un seul endroit pour le fil.

**F6 — Le panneau Doctrines ne dit ni le coût, ni l'influence, ni ce que fait l'idée.**
*Écran* : `12_doctrines_apres.png`. Six cases et « 1/6 » pour tout retour d'un achat.
*Proposition* : afficher sur le panneau le stock d'influence et le coût de la prochaine idée
(les deux sont déjà dans `influence_info` et `doctrine_detail`) — la place est libre, cinq
cases sur six sont vides.

**F7 — L'action diplomatique réussie ne laisse aucune trace.**
*Écran* : `14_diplomatie_pays.png`. Pacte proposé et accepté, écran identique avant/après,
« Émissaire : disponible » inchangé.
*Proposition* : réutiliser la ligne « En cours : … » déjà présente pour y écrire l'envoi.

**F8 — Le titre de l'évènement est répété mot pour mot dans son corps.**
*Écran* : `02_decision.png` (popup) et les trois cartes de la boîte de décision, qui répètent
chacune le même paragraphe de contexte.
*Proposition* : sortir le contexte des cartes et le poser une fois au-dessus ; ne laisser
dans chaque carte que l'action, ses effets et sa ligne de flavor propre.

**F9 — « Une place est TOMBÉE » ×7, sans dire laquelle.**
*Écran* : `17_panneau_armee.png`. `_short()` (`alerts.gd:392`) coupe le texte au premier
« — », ce qui jette précisément le nom et l'année. Sept lignes identiques.

**F10 — Deux boutons « CONSTRUIRE… » identiques sur la fiche province.**
*Écran* : `03_`, `07_`. Rien ne dit lequel ouvre les édifices et lequel les manufactures.
*Proposition* : reprendre les mots des onglets du menu Construction (« Édifices… » /
« Manufactures… »).

**F11 — Le panneau Armée n'est pas fermable au clavier et recouvre le tiroir gauche.**
*Écran* : `18_conseil_orientations.png` (première passe). Il occupe la même bande que le
tiroir et n'est pas dans la pile Échap ; le tiroir Conseil se dessine dessous.
*Proposition* : l'ajouter à la même exclusivité que fiche province ↔ tiroir.

**F12 — Aucune recherche relancée pendant 50 ans, signalée en gris pâle.**
*Écran* : `13_`, `17_`. Le Scriptorium se termine, la cible retombe à −1, et le seul rappel
est la ligne « Aucune RECHERCHE en cours » — sur le bandeau sombre, donc à moitié illisible
(cf. F4). J'ai fini la partie avec « savoir : 5 ».

### COSMÉTIQUE

**F13** — La **date est illisible** (brun sur brun, topbar à droite, `01_`). C'est la valeur
la plus regardée du jeu.
**F14** — Le prix **déborde sur l'état** dans le Marché : « 0.00 couronn**p**énurie sévère ».
Deux abscisses fixes (`x+140` et `x+212`, `sidebar_drawer.gd:580-583`) sans ajustement, alors
que la colonne du nom, elle, utilise déjà `_fit_text`.
**F15** — Un **grand rectangle vert plat** dans le panneau Corps (`17_`).
**F16** — Les **Annales sont posées à (0,0)** : elles recouvrent la moitié gauche de la topbar
et coupent leur propre dernière ligne (`22_`).
**F17** — Flottants qui fuient : « légitimité +0.2 », « 0.7 prospérité », « Cuivre ×0.2 »,
« net/j +378.0 ».
**F18** — Deux notifications tronquées **au milieu d'un mot** : « Un ÉDIFICE est constructible
(ex. Tribun… », « Poisson : rupture dans 0 jours au rythme… ».
**F19** — Une ligne de guerre **sans nom** dans le rail droit (`14_`, `17_`) : icône, barre,
« +50 », et rien entre les deux.
**F20** — « **BIEN INTROUVABLE** » en capitales dans les notifications : un message d'erreur
de moteur montré au joueur.
**F21** — Deux manufactures s'appellent « **?** » (`manuf_name(27)`/`(29)`, qui produisent
« Registres scellés » et « Ouvrages d'agrément »).
**F22** — Colonnes « **net/j** » et « **couv.** » dans les Stocks : par jour, et abrégé.

---

## 4. Ce qui marche — à ne pas casser

- **L'arbre de technologie** (`13_`) : trois couloirs illustrés, la cible et sa barre en
  en-tête, les points, la métabolisation, et un état vide qui explique quoi faire. C'est le
  modèle des autres panneaux.
- **Le menu Construction** (`04_`) : la carte par bâtiment tient sa promesse — effet,
  entretien/mois, matières exactes, « Déverrouille : … ». Et l'effet du Tribunal apparaît
  vraiment deux tours plus tard sur la fiche (« Capacité admin. 10 »).
- **« Refus probable »** sous le bouton d'alliance (`14_`) : le meilleur mot de toute l'UI.
  À généraliser partout où il y a « Indisponible ».
- **La carte parchemin** (`20_`, `23_`) : fleuves, reliefs, vignettes de bourgs, routes,
  lavis politique — elle est belle et elle se lit.
- **Les décrets** (`18_`) : « Rations mesurées — actif · ⊥ exclusif avec : Primes aux
  foyers · [Désactiver] ». Clair, et l'UI a bien traduit la formule moteur du reader.
- **La fenêtre diplomatique** : « Alliances : nous 0/2 · eux 0/2 », « Aucune liaison
  commerciale directe » — des phrases franches.
- **L'épithète du règne** : « Ligue Gualredor, **dit le Discret** ».
- **Le mot « couronnes »**, employé partout de la même façon.

---

## 5. Bugs vus (nombres faux, panneaux vides, mots moteur)

| # | Ce que l'écran montre | Fichier probable |
|---|---|---|
| B1 | Tous les prix à `0.00 couronnes` — `country_stocks[].price` reste 0 | `ui/sidebar_drawer.gd:580` (lecteur `country_stocks`) |
| B2 | `country_budget()` renvoie une liste vide → 8 étiquettes SORTIES sans valeur | `ui/budget_panel_v2.gd` (onglet Balance) |
| B3 | `budget_summary` : `income 0.0 / expense 0.0` alors que l'or varie de −164/mois | lecteur façade |
| B4 | Allocation 90 % → affichée 100 %, classe employée 100 % → 38 % | `ui/province_panel_v2.gd` + `player_alloc_raw` |
| B5 | Texte du rail droit invisible une ligne sur deux (zébrage sans bascule de couleur) | `ui/empire_sidebar.gd:559-567` |
| B6 | `« région 21 »` dans tout évènement du fil | `ui/alerts.gd:160` |
| B7 | `« Prov.6 »` dans une étiquette de bouton | `ui/country_actions.gd` (nom de repli province) |
| B8 | Titre de l'évènement répété dans son corps | `ui/alerts.gd::_popup_of`, cas 10 |
| B9 | Nom de la place perdu par la troncature au premier « — » | `ui/alerts.gd:392` (`_short`) |
| B10 | Rectangle vert plat dans le panneau Corps | `ui/army_panel.gd` |
| B11 | Ligne de guerre sans nom dans le rail droit | `ui/empire_sidebar.gd` (section GUERRES) |
| B12 | `manuf_name(27)` et `manuf_name(29)` renvoient `« ? »` | table des manufactures (moteur) |
| B13 | Toutes les manufactures à 51 : `manuf_cost()` ne prend aucun argument | façade `scps_sim_node.cpp:223` |
| B14 | Deux cellules de topbar à 0 avec un flux de −830 et −1034/mois | lecteur des stocks nationaux |
| B15 | `« BIEN INTROUVABLE »` affiché au joueur | `ui/alerts.gd` (collecte des conditions) |
| B16 | 4 lignes d'annales sur 104 ans : guerres, batailles, colonie non enregistrées | `scps_events.c` / `annals()` |
| B17 | Prix qui déborde sur la colonne d'état | `ui/sidebar_drawer.gd:580-583` |
| B18 | Annales posées à (0,0), recouvrent la topbar et coupent leur dernière ligne | `ui/chronique.gd` |

**Deux réserves d'honnêteté sur la méthode.** (a) La topbar calcule ses deltas comme
(valeur − valeur au dernier `month_ticked`) : une probe qui saute dix ans d'un coup affiche
le saut entier. J'ai corrigé en terminant chaque étape par deux vrais mois — les captures
livrées sont honnêtes, mais c'était un piège. (b) Le menu Construction retombe sur une
position calculée pour une fiche de 348 px alors que la fiche fait ~400 px : hors du chemin
`main.gd:331`, il passe sous la fiche et **la première lettre de chaque ligne est mangée**
(« onstruction », « DIFICES », « ntretien »). En jeu réel `main.gd` le repositionne ; la
constante par défaut est néanmoins périmée.
