# PROMPT CODEX — GÉNÉRATION D'IMAGES, 2e CAMPAGNE : lots 8-13 (2026-08-19)

Tu es chargé de GÉNÉRER LES IMAGES uniquement. Aucun code, aucune intégration
— l'implémentation est faite par ailleurs. Tu livres des PNG conformes à la
spec ci-dessous, rien d'autre. (La 1re campagne — lots 1-7 : topbar, rail,
modes, ressources, troupes, portraits, nature — est DÉJÀ LIVRÉE et intégrée ;
tes images doivent se fondre dans cette famille existante.)

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

## Lot 8 — FOI, ÉTHOS & CRÉATEUR DE CULTURE (128×128)

Tous les symboles religieux sont NEUTRES et inventés — aucun symbole d'une
religion réelle, aucune croix/croissant/étoile existante.

**Credos (préfixe `foi_`)** — la POSTURE de la foi, déclinée autour d'une même
flamme votive (la famille se lit) :
| foi_pluraliste.png | flamme entourée de plusieurs mains ouvertes (tolère, syncrétise) |
| foi_proselyte.png | flamme portée en avant sur un bâton de procession (convertit) |
| foi_loyaliste.png | flamme gardée derrière un bouclier (protège, purifie) |

**Branches de foi (préfixe `foi_`)** — l'arbre généalogique, symboles inventés :
| foi_animiste.png | arbre-esprit aux yeux dans le feuillage |
| foi_scripturaire.png | livre scellé rayonnant |
| foi_roue.png | roue à huit rayons sur lotus stylisé |
| foi_celeste.png | balance sous une voûte d'étoiles |

**Éthos (préfixe `ethos_`, 6)** :
| ethos_dominateur.png | poing ganté fermé |
| ethos_honneur.png | épée levée sur un serment (main sur la garde) |
| ethos_ordre.png | clef de voûte + équerre |
| ethos_bureaucrate.png | sceau sur registre |
| ethos_mercantile.png | balance + pièce |
| ethos_pacifiste.png | rameau d'olivier noué |

**Héritages (préfixe `her_`, 6)** :
| her_esoterique.png | œil dans un cercle runique |
| her_metallurgiste.png | enclume au marteau levé |
| her_mecaniste.png | engrenage à double roue |
| her_adaptatif.png | roseau plié par le vent (qui ne rompt pas) |
| her_agraire.png | charrue sur sillon |
| her_clanique.png | nœud de bannières entrelacées |

**Modes de vie (préfixe `life_`, 7)** :
| life_hunter.png | arc + trace de gibier |
| life_pastoral.png | houlette + mouton |
| life_horticulture.png | houe + jeune pousse |
| life_miner.png | pic + galerie |
| life_farmer.png | faucille + épis |
| life_seafarer.png | barque à voile carrée |
| life_intensive.png | canaux d'irrigation en damier |

**Structures de parenté (préfixe `struct_`, 4)** :
| struct_bilateral.png | deux mains qui se serrent |
| struct_lignager.png | arbre généalogique à trois étages |
| struct_clanique.png | cercle de tentes autour d'un feu |
| struct_tribal.png | anneau fermé de silhouettes |

**Axes de traditions (préfixe `axe_`, 3)** — les 3 axes du créateur :
| axe_physique.png | bras bandé |
| axe_social.png | deux visages en dialogue |
| axe_intellect.png | lampe à huile allumée |

## Lot 9 — CHROME UI : fonds de barres et de panneau (⚠ SANS AUCUN BOUTON)

RÈGLE ABSOLUE : ces images sont des FONDS NUS. AUCUN bouton, AUCUNE icône,
AUCUN texte, AUCUNE cellule dessinée dedans — les contrôles sont posés par le
code par-dessus. Un fond avec des boutons peints est INUTILISABLE.

| fichier | dimensions | sujet |
|---|---|---|
| chrome_topbar_bg.png | 2048×96 (affiché 48 px de haut, pleine largeur) | bandeau de parchemin foncé, grain léger, LISERÉ d'encre au bord inférieur seulement ; TUILABLE horizontalement (les 32 px des deux bouts = bordure 9-slice, le centre se répète sans couture) |
| chrome_sidebar_bg.png | 128×1024 (affiché 64 px de large, pleine hauteur) | même famille, liseré d'encre au bord DROIT seulement ; tuilable verticalement (32 px haut/bas = bordure) |
| chrome_panel_armee_bg.png | 1024×1280 (affiché ~512×640) | plaque de panneau parchemin : cadre gravé aux coins ornés (volutes discrètes), fond uni granuleux au centre — le panneau Armée posera colonnes et formations dessus |

## Lot 10 — ÉDIFICES : 26 icônes (128×128, préfixe `edi_`)

La liste des bâtiments (l'enum moteur, ordre indifférent — le NOM de fichier
fait foi). Chaque édifice = une SILHOUETTE architecturale distincte :
edi_tribunal (balance sur fronton) · edi_chancellerie (plume + sceau sur
pupitre) · edi_academie (fronton à colonnes) · edi_garnison (tour crénelée) ·
edi_forteresse (donjon à double enceinte) · edi_citadelle (muraille en étoile) ·
edi_port (ancre sur quai) · edi_caravanserail (arche + dromadaire) · edi_marche
(étal à auvent) · edi_entrepot (caisses empilées) · edi_grenier (silo sur
pilotis) · edi_irrigation (vanne + canal) · edi_aqueduc (double rangée
d'arches) · edi_sanctuaire (pierre levée fleurie) · edi_temple (fronton à
flamme votive) · edi_cathedrale (flèche à rosace) · edi_bibliotheque (rayonnage
de rouleaux) · edi_monastere (cloître à puits) · edi_comptoir (comptoir +
balance) · edi_banque (coffre cerclé) · edi_arsenal (râtelier d'armes) ·
edi_amiraute (proue + compas) · edi_port_marchand (grue de quai en bois) ·
edi_biblio_mil (livre + épée croisés) · edi_observatoire (lunette vers les
étoiles) · edi_trade_center (grande halle aux bannières).

## Lot 11 — SYSTÈME & VERBES (128×128)

**Classes sociales (préfixe `cls_`, 4)** — partout (démographie, budget, satisfaction) :
| cls_journalier.png | bêche sur l'épaule |
| cls_bourgeois.png | bourse + clef |
| cls_elite.png | couronne fine sur coussin |
| cls_servile.png | chaîne brisée à demi (le fer au poignet) |

**Journal / notifications (préfixe `jrn_`, 12)** — un genre, un glyphe :
jrn_guerre (deux bannières croisées) · jrn_bataille (épées entrechoquées,
étincelle) · jrn_siege (tour sous échelle) · jrn_revolte (torche levée) ·
jrn_secession (bannière déchirée en deux) · jrn_colonisation (charrette +
fanion planté) · jrn_decouverte (lampe rayonnante) · jrn_conseil (sceau
brisé — la trahison) · jrn_foi (flamme votive) · jrn_commerce (balance +
flèches d'échange) · jrn_catastrophe (nuée + éclair) · jrn_tresor (coffre
ouvert, pièces).

**Verbes diplomatiques (préfixe `dip_`, 10)** :
dip_alliance (deux mains scellées sur parchemin) · dip_guerre (gant jeté) ·
dip_paix (rameau sur épée baissée) · dip_tribut (coffret tendu) ·
dip_vassal (genou à terre devant bannière) · dip_embargo (ancre enchaînée) ·
dip_migration (famille en marche, balluchon) · dip_emissaire (cavalier au
rouleau scellé) · dip_intrigue (dague derrière un masque) · dip_revendication
(sceau posé sur une carte).

**Phases d'armée (préfixe `pha_`, 7)** :
pha_lever (cor sonné) · pha_marche (bottes + poussière) · pha_siege (bélier) ·
pha_bataille (choc de boucliers) · pha_pillage (torche sur gerbe) ·
pha_repli (bouclier porté en arrière) · pha_renfort (colonne qui rejoint).

**Âges (préfixe `age_`, 8, médaillons dans un cercle gravé — famille des modes)** :
age_decouvertes (caravelle/astrolabe) · age_empires (couronne sur globe) ·
age_echanges (caducée + balance) · age_soulevements (poing + torche) ·
age_tyrans (trône à pointes) · age_lumieres (lampe sur livre) ·
age_heros (épée plantée au tertre) · age_breche (fissure irradiante).

**Retardataires ressources** : res_ouvrages (livre d'agrément ouvert à
enluminure) · res_talismans (amulette à cordon).

**Verbes système (préfixe `act_`, 4)** — remplacent les derniers anciens :
act_construire (truelle + mur naissant) · act_recruter (cor + bannière) ·
act_rechercher (loupe sur rouleau) · act_decret (sceau qui frappe la cire).

## Lot 12 — ÉCRAN TITRE & FINITIONS

| fichier | dimensions | sujet |
|---|---|---|
| title_screen.png | 1920×1080 (opaque, pas d'alpha) | LA pièce maîtresse : une carte-parchemin en scène — table de cartographe vue de biais, la carte du monde à demi déroulée (continents, routes, mer au lavis), compas, encrier, une bougie ; lumière chaude latérale ; DE LA PLACE À GAUCHE pour le menu (le tiers gauche reste calme/sombre) ; aucun texte, aucun logo |
| chrome_rightbar_bg.png | 512×1024 (affiché ~380 px de large) | fond du panneau de DROITE (ère/empire) : parchemin foncé, liseré d'encre au bord GAUCHE seulement, tuilable verticalement (32 px haut/bas = bordure 9-slice) — SANS AUCUN bouton/texte |
| cursor_fleche.png | 64×64 | curseur : plume d'encre effilée (pointe en haut-gauche) |
| cursor_cible.png | 64×64 | curseur : épée pointée (ciblage) |
| cursor_loupe.png | 64×64 | curseur : loupe de cartographe |
| her_charge_01..12.png | 128×128 | CHARGES héraldiques pour blasons génératifs : tour crénelée · bête dressée (griffon inventé) · croissant inventé à trois pointes · gerbe · ancre · clef · étoile à huit rais · arbre · pont · main gantée · soleil à visage · serpent noué — trait ÉPAIS lisible à 16 px, monochrome sombre (le code les teinte) |

## Lot 13 — ENCARTS TECH façon Civ 6 (512×256, préfixe `tech_`)

Note joueur : « les techs devraient être des encarts, façon Civ 6 » — chaque
technologie = une CARTE illustrée horizontale : une SCÈNE gravée (des mains,
des ateliers, des lieux — pas un pictogramme), dans un cadre gravé intégré
(filet double + coins ornés discrets), fond parchemin. Aucun texte. Même
lumière et même densité de hachures sur toute la série.

**Savoir** : tech_bibliotheque (scribe entre rayonnages) · tech_scriptorium
(copistes à l'ouvrage) · tech_academie (maître et disciples au fronton) ·
tech_universite (amphithéâtre en gradins) · tech_savoir_guerre (officiers sur
carte d'état-major) · tech_magie_bataille (mage traçant un glyphe ardent) ·
tech_invocation (cercle rituel, silhouette évoquée) · tech_eveil (méditant
irradiant) · tech_wards (glyphes de garde sur porte) · tech_scrying (bassin
de vision) · tech_communion (cercle de mains jointes) · tech_savoir_interdit
(livre enchaîné entrouvert).
**Forge & industrie** : tech_collecte_bois (bûcherons à l'abattage) ·
tech_collecte_argile (fosse des potiers) · tech_fonderie (coulée au creuset) ·
tech_outillage (établi aux outils alignés) · tech_manufacture (rangée de
métiers) · tech_industrie (halle aux cheminées) · tech_foreuse (foreuse
arcanique mordant la roche) · tech_armurerie (plastrons au râtelier) ·
tech_poudriere (tonneaux + mèche) · tech_forge_runes (lame gravée sur enclume
luisante) · tech_oeuvre_noire (creuset scellé, fumée noire) · tech_atelier
(artisan au tour) · tech_qualite_materiaux (maître jaugeant une poutre) ·
tech_fortifications (maçons sur rempart) · tech_automates (pantin mécanique
debout à l'établi).
**Société & subsistance** : tech_collecte_nourriture (cueillette en panier) ·
tech_irrigation (vannes ouvertes sur sillons) · tech_commerce (poignée de main
sur étal) · tech_cadastre (arpenteur à la chaîne) · tech_abondance (corne
débordante) · tech_comptoirs (comptoir au port) · tech_halles (halle aux sacs
empilés) · tech_caserne (recrues à l'exercice) · tech_conscription (rôle
d'appel lu au village) · tech_organisation (colonne en ordre de marche) ·
tech_esclavage (marché sombre aux fers — scène DURE assumée, sans
complaisance) · tech_caste_martiale (lignée d'armes, père et fils) ·
tech_chancellerie (chancelier au sceau) · tech_foi (procession à la flamme) ·
tech_integration (deux cortèges qui se mêlent) · tech_culte_imperial (statue
du souverain encensée) · tech_alchimie (alambic au feu doux) ·
tech_transmutation (main changeant le plomb en éclat).
**Héritages (paires)** : tech_glyphes_etheres (glyphes flottants) ·
tech_communion_etheree (esprits en cercle) · tech_alliages_nains (lingot
bicolore au marteau) · tech_gravure_runes (burin sur métal) ·
tech_mecaniste_rouages (train d'engrenages) · tech_mecaniste_horlogerie
(horloge ouverte) · tech_droit_coutumier (anciens sous l'arbre à palabres) ·
tech_langue_franque (marchands aux gestes croisés) · tech_vergers_etages
(terrasses plantées) · tech_paturages_integres (troupeaux entre cultures) ·
tech_rites_guerriers (danse d'armes au feu) · tech_hordes_conquerantes
(cavaliers à l'horizon).
**Combos (14)** : tech_combo_poudre (arquebuse de précision à l'essai) ·
tech_combo_automates_arc (automate aux yeux luisants) · tech_combo_academie
(savants de plusieurs peuples) · tech_combo_druide (druide aux moissons) ·
tech_combo_chaman (chaman devant les guerriers) · tech_combo_guildes (maîtres
au blason de guilde) · tech_combo_charrues (charrue lourde à deux bœufs) ·
tech_combo_poliorcetique (trébuchet en construction) · tech_combo_horloge_march
(horloge sur comptoir) · tech_combo_machines_agri (moissonneuse à rouages) ·
tech_combo_siege (tour de siège roulée) · tech_combo_grenier_colon (grenier
au bout du monde) · tech_combo_foederati (serment de deux armées) ·
tech_combo_horde_eco (campement-caravane).
**Apex (3, cadre légèrement DORÉ — les seuls)** : tech_apex_arquebuse
(arquebuse aux runes ardentes) · tech_apex_concile (concile en amphithéâtre) ·
tech_apex_legion (légion bigarrée sous un aigle).

## Livraison

Une archive (ou un dossier) avec les 6 lots (8 à 13), un sous-dossier par lot,
PNG alpha aux noms EXACTS ci-dessus (title_screen.png : opaque). Toute image
que tu ne peux pas produire : NE PAS improviser un autre style — livre la
liste des manquantes à part.
