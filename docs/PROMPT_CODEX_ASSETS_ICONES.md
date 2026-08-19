# PROMPT CODEX — GÉNÉRATION D'IMAGES : icônes & portraits SCPS (2026-08-19)

Tu es chargé de GÉNÉRER LES IMAGES uniquement. Aucun code, aucune intégration —
l'implémentation est faite par ailleurs. Tu livres des PNG conformes à la spec
ci-dessous, rien d'autre.

## Direction artistique (commune à TOUTES les images)

- **Style** : gravure à l'encre sur carte ancienne — trait sépia brun foncé
  (#2a2419 à #4a3624), hachures de graveur, rehauts crème parchemin (#efe6cc)
  en aplat léger. Aucun dégradé moderne, aucun style flat/emoji, aucun néon.
  Référence d'esprit : icônes de manuscrit/atlas XVIIe, chips de jeu de plateau.
- **Fond TRANSPARENT (alpha)** — jamais de fond magenta, jamais de fond blanc.
  (L'historique du projet a payé cher le détourage de planches magenta.)
- **Trait** : épaisseur cohérente sur tout le lot (~3 px à l'échelle 128) ;
  chaque icône doit rester LISIBLE réduite à 24 px (tester mentalement : la
  silhouette prime, le détail intérieur est secondaire).
- **Cadrage** : sujet centré, marge de 12 % sur chaque bord, pas d'ombre portée,
  pas de texte, pas de chiffres dans l'image.
- **Un PNG par icône** (pas de planche/spritesheet), nommé EXACTEMENT comme
  indiqué, en minuscules. Dimensions : 128×128 sauf mention contraire.

## Lot 1 — Icônes SYSTÈME topbar (128×128)

| fichier | sujet |
|---|---|
| top_or.png | pièces d'or empilées (2-3 pièces frappées) |
| top_population.png | trois silhouettes humaines serrées (foule) |
| top_construction.png | pierre taillée + brique + rondin assemblés (le trio matériaux) |
| top_nourriture.png | gerbe de blé sur miche de pain |
| top_armes.png | deux épées croisées |
| top_manufactures.png | tonneau + étoffe pliée (biens ouvrés) |
| top_satisfaction.png | visage serein de médaille (profil gravé, bouche neutre-souriante) |

## Lot 2 — Rail gauche : 8 onglets (128×128, silhouettes FRANCHES — affichés 52 px)

| fichier | sujet |
|---|---|
| rail_economie.png | balance de changeur |
| rail_demographie.png | famille (deux grandes silhouettes + une petite) |
| rail_stocks.png | jarres et sacs entassés |
| rail_marche.png | étal à auvent |
| rail_armee.png | heaume de profil |
| rail_filtres.png | lanterne de cartographe (œil sur la carte) |
| rail_diplomatie.png | parchemin scellé (sceau de cire) |
| rail_conseil.png | trois sièges autour d'une table ronde |

## Lot 3 — Modes carte : 6 médaillons (128×128, dans un CERCLE gravé)

| fichier | sujet |
|---|---|
| mode_defaut.png | rose des vents simple |
| mode_politique.png | couronne sur territoire hachuré |
| mode_nature.png | arbre seul sans aucun symbole humain |
| mode_marche.png | pièce + caducée de Mercure (ou bourse nouée) |
| mode_religion.png | flamme votive sur autel (symbole de foi NEUTRE — aucun credo réel) |
| mode_culture.png | deux visages de profil face à face (l'échange des cultures) |

## Lot 4 — RESSOURCES : 49 icônes (128×128, préfixe `res_`)

Brutes agricoles : res_grain (gerbe de blé) · res_livestock (bœuf de profil) ·
res_wool (toison bouclée) · res_fish (poisson arqué) · res_fur (peau tendue) ·
res_salt (cristaux en tas) · res_cotton (fleur de coton) · res_sugar (canne à
sucre) · res_wood (rondins liés) · res_fruit (grappe + pomme) · res_med_herbs
(brin d'herbe en croix de simples).
Minerais & stratégiques : res_copper (lingot vert-de-gris) · res_iron (lingot
gris strié) · res_coal (morceaux charbonneux) · res_sulfur (pierre jaune
fumante) · res_saltpeter (poudre en coupelle) · res_gold_ore (pépite sur roche) ·
res_precious_metal (lingot à reflet étoilé — mithril) · res_pearl (perle sur
coquille) · res_arcane_crystal (cristal à facettes lumineux) ·
res_celestial_iron (météorite striée) · res_murex (coquillage épineux pourpre) ·
res_indigo (feuille + pigment bleu) · res_clay (motte + main de potier) ·
res_stone (bloc taillé).
Biens de production : res_cloth (rouleau d'étoffe) · res_naval_supplies
(cordage + tonnelet de goudron) · res_eau_de_vie (alambic + fiole) · res_beer
(chope mousseuse) · res_precious_ware (vase de porcelaine) · res_precious_cloth
(étoffe brodée d'or) · res_paper (feuille + plume) · res_tools (marteau +
tenailles) · res_essence (fiole d'essence irisée) · res_enchanted_arms (épée
runique luisante) · res_arms (épée simple) · res_gunpowder (baril + poudre) ·
res_remede (mortier + pilon) · res_tunique (tunique pliée) ·
res_essence_purifiee (fiole distillée à double col) · res_flux (spirale de flux
en fiole) · res_alchemist_kit (sacoche + fioles) · res_arms_heavy (hache
lourde + plastron) · res_arms_ranged (arc + carquois) · res_firearm
(arquebuse) · res_mage_staff (bâton à orbe) · res_pottery (cruche + bol) ·
res_statue (statuette sur socle).
Signatures d'éthos : res_heaumes (heaume orné) · res_parures (collier +
diadème) · res_horloges (horloge de table) · res_registres (registre scellé) ·
res_colifichets (breloques exotiques) — et res_talismans si demandé plus tard.

## Lot 5 — TROUPES : 22 glyphes (128×128, préfixe `unit_`)

SILHOUETTES de plateau (buste ou figure entière stylisée), distinctes au
premier coup d'œil entre elles — c'est LE critère :
unit_piquier (pique verticale) · unit_lancier (lance + bouclier rond) ·
unit_epeiste (épée + bouclier long) · unit_archer (arc bandé) · unit_arbalete
(arbalète horizontale) · unit_cav_legere (cavalier au galop, ligne fluide) ·
unit_cav_lourde (cavalier cuirassé au pas, masse) · unit_mage (robe + bâton à
orbe) · unit_hallebardier (hallebarde à crochet) · unit_arquebusier (arquebuse
épaulée, mèche fumante) · unit_alchimiste (fiole brandie + sacoche) ·
unit_garde_runique (armure gravée de runes, hache) · unit_arbalete_lourde
(pavois planté + arbalète à treuil) · unit_berserker (torse nu, double hache,
posture furieuse) · unit_lancier_choc (lance couchée, épaule en avant) ·
unit_milice (fourche + faux, chapeau de paysan) · unit_harceleur (javelot
lancé, jambe en fuite) · unit_traqueur (capuche + arc bas, à demi accroupi) ·
unit_lame_franche (rapière + bourse à la ceinture) · unit_garde_escorte
(grand bouclier planté, immobile) · unit_cav_cuirassee (cavalier tout de
plates, lance de choc) · unit_cav_raid (cavalier léger, torche + sac de
butin).

## Lot 6 — PORTRAITS politiques : REPROMPT 2026-08-19 (portraits COMPLETS)

⚠ La v1 « par couches composables » est ABANDONNÉE (décision joueur : les
couches ne s'alignent pas — cols, épaules, coiffes). On repart en portraits
COMPLETS : UNE image finie par personnage, aucune superposition à faire.

- **Format** : 256×256, alpha, préfixe `pt_`. Le portrait est ENTIÈREMENT
  CONTENU dans un médaillon ovale gravé (le cadre fait partie de l'image ;
  RIEN ne dépasse de l'ovale — pas d'épaules hors cadre, l'extérieur de
  l'ovale est 100 % transparent).
- **Gabarit identique** pour tous : même ovale, même échelle de buste, même
  angle trois quarts — seuls le visage, l'habit et la coiffe changent.
- **Style** : gravure au burin, hachures de modelé, teint parchemin (pas de
  couleur de peau réaliste) — identique à la v1.
- **Inventaire (18 portraits, 6 par classe, varier âge/genre/pilosité/coiffe
  À L'INTÉRIEUR de chaque classe) :**
  - `pt_journalier_01..06` — habit simple : tunique lacée, capuche ou tête
    nue, visages burinés de travailleurs.
  - `pt_bourgeois_01..06` — pourpoint boutonné, chaperon/toque de drap,
    visages de marchands et maîtres de guilde.
  - `pt_elite_01..06` — manteau à fourrure, chaîne d'office, couronne fine ou
    toque noble, visages d'aristocrates.
- La sélection en jeu est déterministe (hash % 6 dans la classe) : les 6 d'une
  classe doivent être NETTEMENT différenciables entre eux au premier regard.

## Lot 7 — NATURE : tampons d'habillage de carte (512×512, préfixe `nat_`)

But : habiller la carte-parchemin — des TAMPONS de gravure posés sur le terrain
(même famille que les gravures existantes : arbres de canopée, herbes, écume).
Style identique au reste (trait sépia, hachures), mais posés SUR la carte :
- **Ancrage au PIED** : le sujet touche le bas de son cadre (marge basse 4 %),
  l'alpha fait le reste — c'est la base qui se pose sur le sol.
- Chaque variante DIFFÉRENTE en silhouette (on en sème des dizaines : deux
  jumelles côte à côte se voient tout de suite).

| fichiers | sujet |
|---|---|
| nat_jungle_01..04 | arbres de JUNGLE : large feuillu tropical à contreforts, palmier haut, fougère arborescente, canopée dense à lianes |
| nat_rocher_01..03 | CAILLOUX : bloc erratique moussu, chaos de deux-trois blocs, aiguille rocheuse |
| nat_buisson_01..03 | BUISSONS : buisson rond touffu, fourré épineux, buisson bas fleuri |
| nat_arbre_mort_01..02 | arbre mort tordu (steppe/froid), tronc foudroyé |
| nat_roseaux_01..02 | roseaux/joncs en touffe (marais, berges) |
| nat_cactus_01..02 | cactus cierge, agave (désert) |
| nat_souche_01 | souche à anneaux (lisières, clairières) |

NB : les LAPINS d'apocalypse (marginalia façon KCD) ont été retirés du jeu —
n'en générer AUCUN, ni aucune marginalia animalière signée d'un autre jeu.
Serpents de mer et épaves existent déjà, ne pas en refaire.

## Livraison

Une archive (ou un dossier) avec les 7 lots, PNG alpha aux noms EXACTS
ci-dessus. Toute icône que tu ne peux pas produire : NE PAS improviser un
autre style — livre la liste des manquantes à part.
