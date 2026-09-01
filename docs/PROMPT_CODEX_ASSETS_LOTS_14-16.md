# PROMPT CODEX — GÉNÉRATION D'IMAGES, 3e CAMPAGNE : lots 14-16 (2026-09-01)

Tu es chargé de GÉNÉRER LES IMAGES uniquement. Aucun code, aucune intégration,
aucun bouton, aucun texte dans les images — l'implémentation est faite par
ailleurs. Tu livres des PNG conformes à la spec, rien d'autre. (Les campagnes
1 et 2 — lots 1-13 : topbar, rail, modes, ressources, troupes, portraits,
nature, foi/éthos, chrome, édifices, système, titre, encarts tech — sont DÉJÀ
LIVRÉES et intégrées ; tes images doivent se fondre dans cette famille.)

## Direction artistique (IDENTIQUE aux campagnes précédentes)

- **Style** : gravure à l'encre sur carte ancienne — trait sépia brun foncé
  (#2a2419 à #4a3624), hachures de graveur, rehauts crème parchemin (#efe6cc)
  en aplat léger. Aucun dégradé moderne, aucun flat/emoji, aucun néon.
  Référence : icônes de manuscrit/atlas XVIIe, chips de jeu de plateau.
- **Fond TRANSPARENT (alpha)** pour toutes les icônes — jamais de magenta,
  jamais de blanc. Les FONDS d'encart (lot 14) sont OPAQUES (fond parchemin).
- **Trait** : épaisseur cohérente sur tout le lot (~3 px à l'échelle 128) ;
  chaque icône lisible réduite à 24 px (la silhouette prime).
- **Cadrage** : sujet centré, marge 12 %, pas d'ombre portée, pas de texte,
  pas de chiffres.
- **Un PNG par image**, nommé EXACTEMENT comme indiqué, en minuscules.

## Lot 14 — FONDS DE DOCTRINE (512×256, opaque, préfixe `doct_`)

17 encarts horizontaux, MÊME famille que les encarts tech du lot 13 (cadre
gravé intégré : filet double + coins ornés discrets, fond parchemin, une
SCÈNE gravée — des mains, des lieux, des gestes, pas un pictogramme). Une
contrainte de composition NOUVELLE : **le TIERS DROIT de chaque scène reste
calme et peu contrasté** (l'interface y posera la colonne des 6 idées) — la
scène vit sur les deux tiers gauches, le tiers droit s'éteint en fond de
parchemin travaillé.

| doct_offense_bg.png | colonne en marche, lances inclinées vers l'avant, poussière |
| doct_defense_bg.png | rempart tenu, échelles repoussées, ciel bas |
| doct_commerce_bg.png | quai animé : ballots, balance, voile au fond |
| doct_mercantilisme_bg.png | halle-entrepôt du roi : étagères pleines, garde au registre |
| doct_peuple_bg.png | porte de ville ouverte, foule mêlée qui entre, baluchons |
| doct_colonisation_bg.png | débarquement sur rivage vierge : barque tirée, première tente |
| doct_diplomatie_bg.png | antichambre : émissaires attendant, dépêches scellées |
| doct_vassaux_bg.png | hommage : genou à terre devant un trône, bannières inclinées |
| doct_production_bg.png | mine et atelier en coupe : galerie, treuil, établi |
| doct_infrastructure_bg.png | chantier d'aqueduc : échafaudages, arches en cours |
| doct_technologie_bg.png | scriptorium : pupitres alignés, copistes à l'ouvrage |
| doct_connaissances_bg.png | cabinet de cartographe : globe, compas, cartes déroulées |
| doct_faustien_bg.png | machine sombre au cœur rougeoyant, silhouettes en retrait |
| doct_aristocratie_bg.png | cour de château : chasse au faucon, étendards |
| doct_bourgeoisie_bg.png | place de guilde : enseignes suspendues, poinçons, comptoirs |
| doct_populaire_bg.png | place du marché en assemblée, mains levées |
| doct_divin_bg.png | procession aux flambeaux montant vers un temple |

## Lot 15 — ICÔNES D'IDÉES (128×128, alpha, préfixe `idea_`)

102 icônes : 6 par doctrine. Même famille que les icônes système du lot 11
(un OBJET/GESTE net, silhouette d'abord). Chaque ligne = nom exact + motif.

**Offense** :
| idea_offense_arsenaux.png | râteliers de lances, piles de boucliers |
| idea_offense_discipline.png | rang de piques alignées au cordeau |
| idea_offense_ost.png | tente de commandement, bannière plantée |
| idea_offense_butin.png | coffre ouvert devant une muraille brisée |
| idea_offense_pretextes.png | parchemin scellé posé sur un poignard |
| idea_offense_levee.png | cor de guerre sonné |

**Défense** :
| idea_defense_remparts.png | créneaux à merlons épais |
| idea_defense_magasins.png | sacs de grain empilés sous voûte |
| idea_defense_ban.png | cloche renversée + fourche dressée |
| idea_defense_corvees.png | hotte de pierres portée à dos |
| idea_defense_terre_brulee.png | champ moissonné ras, torche abaissée |
| idea_defense_genie.png | compas d'architecte sur plan de bastion |

**Commerce** :
| idea_commerce_franchises.png | barrière de péage levée |
| idea_commerce_routes_longues.png | route jalonnée serpentant vers l'horizon |
| idea_commerce_comptoir.png | façade d'échoppe, enseigne à balance |
| idea_commerce_negoce.png | deux mains échangeant une bourse |
| idea_commerce_guildes.png | sceau de guilde sur ballot cordé |
| idea_commerce_libre_echange.png | chaîne de port brisée |

**Mercantilisme** :
| idea_mercantilisme_reserves.png | greniers cerclés, échelle posée |
| idea_mercantilisme_regie.png | registre ouvert + balance du contrôleur |
| idea_mercantilisme_blocus.png | chaîne tendue en travers d'une passe de port |
| idea_mercantilisme_etape.png | borne d'étape, ballots consignés au pied |
| idea_mercantilisme_peages.png | herse de péage, coffre frappé d'une couronne |
| idea_mercantilisme_halles.png | halle charpentée aux étals pleins |

**Peuple** :
| idea_peuple_accueil.png | porte de ville ouverte, baluchons posés |
| idea_peuple_ecoles.png | enfant à l'ardoise devant un maître |
| idea_peuple_asile.png | toile tendue abritant une famille en marche |
| idea_peuple_affranchissement.png | chaîne tombée au sol, maillons ouverts |
| idea_peuple_tolerance.png | deux autels différents sous un même toit |
| idea_peuple_metissage.png | deux fils noués en un seul cordage |

**Colonisation** :
| idea_colonisation_colons.png | chariot bâché en route |
| idea_colonisation_ravitaillement.png | tonneaux chargés sur une barque |
| idea_colonisation_acclimatation.png | jeune plant tuteuré en sol aride |
| idea_colonisation_double_chantier.png | deux coques en construction côte à côte |
| idea_colonisation_climats.png | palmier et sapin sur la même ligne d'horizon |
| idea_colonisation_grand_large.png | proue fendant une haute vague |

**Diplomatie** :
| idea_diplomatie_prestige.png | couronne de laurier posée sur une lettre |
| idea_diplomatie_chancellerie.png | pupitre de scribe, dépêches empilées |
| idea_diplomatie_oubli.png | page raturée, grattoir posé dessus |
| idea_diplomatie_second_emissaire.png | deux cavaliers partant en sens opposés |
| idea_diplomatie_persuasion.png | main ouverte tendant un traité |
| idea_diplomatie_congres.png | table ronde, sceaux multiples pendants |

**Vassaux** :
| idea_vassaux_serments.png | main posée sur un reliquaire |
| idea_vassaux_tribut_vassal.png | petit coffre porté vers un trône |
| idea_vassaux_contrats.png | parchemin à trois sceaux pendants |
| idea_vassaux_leviers.png | main gantée tenant des rubans liés à trois petits blasons |
| idea_vassaux_annexion.png | petit blason fondu dans un grand |
| idea_vassaux_suzerainete.png | couronne tendue vers un genou à terre |

**Production** :
| idea_production_extraction.png | pioche plantée en taille de mine |
| idea_production_outillage.png | établi, outils suspendus alignés |
| idea_production_exploitation.png | galerie étagée s'enfonçant |
| idea_production_manufactures.png | métiers à tisser en batterie |
| idea_production_gages.png | bourse versée dans des mains ouvrières |
| idea_production_rendement.png | benne pleine remontée au treuil |

**Infrastructure** :
| idea_infrastructure_macons.png | truelle croisée d'un fil à plomb |
| idea_infrastructure_carrieres.png | front de taille, blocs équarris |
| idea_infrastructure_entretien.png | toit rechargé de tuiles neuves |
| idea_infrastructure_renovation.png | échafaudage sur façade ancienne |
| idea_infrastructure_logements.png | rangée de maisons mitoyennes |
| idea_infrastructure_intendance.png | rouleau d'arpenteur + jalons plantés |

**Technologie** :
| idea_technologie_bibliotheques.png | rayonnage de rouleaux étiquetés |
| idea_technologie_ecoles_ville.png | fronton d'école de quartier |
| idea_technologie_programme.png | main désignant un rayon précis d'une armoire |
| idea_technologie_copistes.png | copiste en chemin, sacoche de manuscrits |
| idea_technologie_dispense.png | cadenas ouvert sur un tome |
| idea_technologie_sobriete.png | lampe éclairant un livre ouvert, un autre fermé au fer |

**Connaissances du monde** :
| idea_connaissances_portulans.png | carte côtière aux lignes de rhumb |
| idea_connaissances_truchements.png | deux profils face à face, un ruban entre eux |
| idea_connaissances_expedition.png | navire pointant vers une terre esquissée |
| idea_connaissances_dictionnaires.png | lexique ouvert à deux colonnes |
| idea_connaissances_colleges.png | pupitres disposés en cercle |
| idea_connaissances_langue_franque.png | carte passée de main en main |

**Faustien** :
| idea_faustien_pages_interdites.png | feuillet arraché, bord fumant |
| idea_faustien_creusets.png | deux cornues au feu |
| idea_faustien_pacte.png | plume trempée dans une coupe sombre |
| idea_faustien_or_du_puits.png | pièces tombant d'une machine de forage |
| idea_faustien_terre_changee.png | épi démesuré aux racines noires |
| idea_faustien_prix_consenti.png | main tendue vers une lueur, ombre très longue |

**Aristocratie** :
| idea_aristocratie_bannerets.png | bannière inclinée en hommage |
| idea_aristocratie_offices.png | clef d'office sur coussin |
| idea_aristocratie_adoubement.png | épée posée sur une épaule |
| idea_aristocratie_fiefs.png | borne armoriée plantée en terre |
| idea_aristocratie_ban_feodal.png | heaume à cimier sur lance |
| idea_aristocratie_cloture.png | grille de manoir fermée |

**Bourgeoisie** :
| idea_bourgeoisie_chartes.png | charte scellée clouée sur porte de ville |
| idea_bourgeoisie_jurandes.png | poinçon de maître frappé sur un ouvrage |
| idea_bourgeoisie_emprunt.png | coffre prêté contre billet signé |
| idea_bourgeoisie_credit.png | billet à ordre + plume |
| idea_bourgeoisie_robe.png | robe de magistrat sur dossier de siège |
| idea_bourgeoisie_cles_de_la_ville.png | clefs remises sur coussin |

**Populaire** :
| idea_populaire_doleances.png | cahier ouvert couvert de marques |
| idea_populaire_pain.png | miche rompue, partagée |
| idea_populaire_levee_en_masse.png | faux redressées en faisceau |
| idea_populaire_concession.png | sac de grain tendu par-dessus une barricade |
| idea_populaire_impot_du_rang.png | bague versée dans un tronc commun |
| idea_populaire_souverainete.png | silhouettes en foule levant une couronne |

**Divin** :
| idea_divin_onction.png | fiole versée sur un front couronné |
| idea_divin_ferveur.png | brasier votif qu'une main nourrit |
| idea_divin_sacerdoce.png | étole remise entre deux mains |
| idea_divin_appel.png | cloche en pleine volée |
| idea_divin_clerge.png | deux clercs en chemin, bourdon à la main |
| idea_divin_orthodoxie.png | un seul livre sur l'autel, d'autres enchaînés dessous |

## Lot 16 — INFLUENCE POLITIQUE (dessin seul, alpha)

La ressource politique du jeu : la voix des grands portée par le Conseil.
Motif retenu : **le sceau de cire au ruban** (l'acte qui engage). Aucun
chiffre, aucune jauge, aucun élément d'interface — le dessin seul.

| influence.png | 128×128 | sceau de cire à motif de couronne stylisée INVENTÉE, ruban court à deux pans — l'icône ressource (topbar/boutons), silhouette lisible à 20 px |
| influence_grand.png | 256×256 | le même sceau en version d'apparat : ruban déployé, léger réseau de filigranes autour (pour l'en-tête du Conseil), même trait, même encre |

## Livraison

- Arborescence : `lot14_doctrines/`, `lot15_idees/`, `lot16_influence/`.
- 17 + 102 + 2 = **121 PNG**. Un zip unique.
- Rappels éliminatoires : noms EXACTS · alpha (sauf lot 14 opaque) · aucun
  texte/chiffre · aucun bouton · même encre, même lumière, même densité de
  hachures que les lots 1-13.
