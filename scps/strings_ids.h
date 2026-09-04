#ifndef SCPS_STRINGS_IDS_H
#define SCPS_STRINGS_IDS_H
/*
 * strings_ids.h — LA LISTE MAÎTRESSE (X-macro) : ids + texte FRANÇAIS de
 * référence, sur UNE liste — l'enum et TABLE_FR en sortent par construction,
 * donc ne peuvent pas diverger. L'anglais vit dans strings_en.h (même liste,
 * textes jumeaux) ; sa complétude est vérifiée à la COMPILATION (table
 * positionnelle + assert de taille : une ligne manquante casse le build).
 *
 * RÈGLE (CLAUDE.md §langue) : aucune chaîne littérale face-joueur hors des
 * tables. Tout panneau/journal/bande/tooltip futur naît en STR_*.
 * Les PLAGES (bandes, tuto) sont contiguës par construction (tr_band).
 */
#define SCPS_STRINGS(X) \
    X(STR_BANDE_STAB_0, "Submergée") \
    X(STR_BANDE_STAB_1, "Vacillante") \
    X(STR_BANDE_STAB_2, "Tenue") \
    X(STR_BANDE_STAB_3, "Assurée") \
    X(STR_BANDE_STAB_4, "Inébranlable") \
    X(STR_BANDE_ASSISE_0, "Consentie") \
    X(STR_BANDE_ASSISE_1, "Partagée") \
    X(STR_BANDE_ASSISE_2, "Contrainte") \
    X(STR_BANDE_ASSISE_3, "Tyrannique") \
    X(STR_BANDE_LEGIT_0, "Usurpée") \
    X(STR_BANDE_LEGIT_1, "Contestée") \
    X(STR_BANDE_LEGIT_2, "Tolérée") \
    X(STR_BANDE_LEGIT_3, "Reconnue") \
    X(STR_BANDE_LEGIT_4, "Sacrée") \
    X(STR_BANDE_CONCORDE_0, "Unie") \
    X(STR_BANDE_CONCORDE_1, "Murmurante") \
    X(STR_BANDE_CONCORDE_2, "Fracturée") \
    X(STR_BANDE_CONCORDE_3, "Sécession") \
    X(STR_BANDE_PROSP_0, "Misère") \
    X(STR_BANDE_PROSP_1, "Disette") \
    X(STR_BANDE_PROSP_2, "Suffisance") \
    X(STR_BANDE_PROSP_3, "Aisance") \
    X(STR_BANDE_PROSP_4, "Opulence") \
    X(STR_BANDE_SAVOIR_0, "Obscurité") \
    X(STR_BANDE_SAVOIR_1, "Lueur") \
    X(STR_BANDE_SAVOIR_2, "Foyer") \
    X(STR_BANDE_SAVOIR_3, "Phare") \
    X(STR_BANDE_PRESAGE_0, "Calme") \
    X(STR_BANDE_PRESAGE_1, "Frémissement") \
    X(STR_BANDE_PRESAGE_2, "Ombre grandissante") \
    X(STR_BANDE_PRESAGE_3, "Le seuil") \
    X(STR_BANDE_STATURE_0, "Désert") \
    X(STR_BANDE_STATURE_1, "Hameau") \
    X(STR_BANDE_STATURE_2, "Bourg") \
    X(STR_BANDE_STATURE_3, "Cité") \
    X(STR_BANDE_STATURE_4, "Métropole") \
    X(STR_BANDE_FLUX_0, "Exode") \
    X(STR_BANDE_FLUX_1, "Saignée") \
    X(STR_BANDE_FLUX_2, "Stable") \
    X(STR_BANDE_FLUX_3, "Afflux") \
    X(STR_BANDE_FLUX_4, "Ruée") \
    X(STR_BANDE_AISANCE_0, "Misère") \
    X(STR_BANDE_AISANCE_1, "Suffisance") \
    X(STR_BANDE_AISANCE_2, "Aisance") \
    X(STR_BANDE_AISANCE_3, "Faste") \
    X(STR_BANDE_CARREFOUR_0, "—") \
    X(STR_BANDE_CARREFOUR_1, "Florissante") \
    X(STR_BANDE_CARREFOUR_2, "Bouillonnante") \
    X(STR_BANDE_CARREFOUR_3, "En surchauffe") \
    X(STR_BANDE_HUMEUR_0, "Révoltée") \
    X(STR_BANDE_HUMEUR_1, "Frondeuse") \
    X(STR_BANDE_HUMEUR_2, "Tiède") \
    X(STR_BANDE_HUMEUR_3, "Loyale") \
    X(STR_BANDE_HUMEUR_4, "Dévouée") \
    X(STR_BANDE_LIGNEE_0, "Du même sang") \
    X(STR_BANDE_LIGNEE_1, "Cousine") \
    X(STR_BANDE_LIGNEE_2, "Sœur lointaine") \
    X(STR_BANDE_LIGNEE_3, "Étrangère") \
    X(STR_BANDE_LIGNEE_4, "Hérétique proche") \
    X(STR_BANDE_LIGNEE_5, "Inassimilable") \
    X(STR_BANDE_AGITATION_0, "Calme") \
    X(STR_BANDE_AGITATION_1, "Frémissante") \
    X(STR_BANDE_AGITATION_2, "Agitée") \
    X(STR_BANDE_AGITATION_3, "Insurgée") \
    X(STR_BANDE_FOI_0, "Dévote") \
    X(STR_BANDE_FOI_1, "Tiède") \
    X(STR_BANDE_FOI_2, "Hérétique") \
    X(STR_BANDE_SEDITION_0, "Concorde") \
    X(STR_BANDE_SEDITION_1, "Murmures") \
    X(STR_BANDE_SEDITION_2, "Tendue") \
    X(STR_BANDE_SEDITION_3, "Séditieuse") \
    X(STR_FORGE_0, "Forge rudimentaire") \
    X(STR_FORGE_1, "Forge artisanale") \
    X(STR_FORGE_2, "Manufacture") \
    X(STR_FORGE_3, "Industrie") \
    X(STR_PROF_0, "hors de portée") \
    X(STR_PROF_1, "savoir de surface") \
    X(STR_PROF_2, "savoir-faire d'atelier") \
    X(STR_PROF_3, "art profond") \
    X(STR_PROF_4, "secret jalousement gardé") \
    X(STR_ACCES_0, "lointain") \
    X(STR_ACCES_1, "à portée") \
    X(STR_ACCES_2, "imminent") \
    X(STR_ACCES_3, "acquis") \
    X(STR_MORAL_0, "ferme") \
    X(STR_MORAL_1, "éprouvé") \
    X(STR_MORAL_2, "vacillant") \
    X(STR_MORAL_3, "rompu") \
    X(STR_FIDELITE_0, "fidèle") \
    X(STR_FIDELITE_1, "tiède") \
    X(STR_FIDELITE_2, "frondeur") \
    X(STR_FIDELITE_3, "ligueur") \
    X(STR_MARCHE_0, "marché mort") \
    X(STR_MARCHE_1, "pénurie sévère") \
    X(STR_MARCHE_2, "tendu") \
    X(STR_MARCHE_3, "sain") \
    X(STR_MARCHE_4, "engorgé") \
    X(STR_LENS_0, "—") \
    X(STR_LENS_1, "Prospérité") \
    X(STR_LENS_2, "Humeur") \
    X(STR_LENS_3, "Marché") \
    X(STR_HOVER_STAB, "La solidité de l'ordre : un royaume assuré encaisse les chocs, un royaume vacillant cède au premier vent.") \
    X(STR_HOVER_ASSISE, "Sur quoi repose l'obéissance : l'adhésion des cœurs, ou le seul poids des armes.") \
    X(STR_HOVER_LEGIT, "Le droit reconnu au trône de régner ; sacrée, nul ne la conteste — usurpée, chacun guette la chute.") \
    X(STR_HOVER_CONCORDE, "L'unité des peuples sous une même couronne ; quand les coutures lâchent, les marges rêvent d'indépendance.") \
    X(STR_HOVER_PROSP, "La richesse qui circule et qu'on parvient à lever ; un royaume opulent rayonne, une disette le vide. Bâtie dans une province (infrastructure, ouverture au négoce), elle y nourrit la croissance, la satisfaction des classes et l'assiette de l'impôt.") \
    X(STR_HOVER_SAVOIR, "Le savoir né aux carrefours des cultures ; il nourrit les arts et les arcanes. Ce sont ces points de recherche, produits chaque mois, qui paient les technologies — rien d'autre ne les paie.") \
    X(STR_HOVER_PRESAGE, "Ce que la quête de puissance attire ; plus on force l'arcane, plus l'ombre s'épaissit.") \
    X(STR_HOVER_STATURE, "L'ampleur de l'établissement humain, du hameau perdu à la cité grouillante.") \
    X(STR_HOVER_FLUX, "Le mouvement des âmes : un afflux gonfle la province, un exode la vide.") \
    X(STR_HOVER_AISANCE, "La richesse qui circule ici ; les carrefours prospèrent, les culs-de-sac s'étiolent.") \
    X(STR_HOVER_CARREFOUR, "Quand des cultures se croisent ici, la richesse afflue — jusqu'à ce que le flux déborde et que la ville-monde se déchire.") \
    X(STR_HOVER_HUMEUR, "Le cœur de la province envers la couronne ; loyale, elle paie sans broncher — frondeuse, elle attend l'étincelle.") \
    X(STR_HOVER_LIGNEE, "Ce qui la lie à la culture du trône ; le même sang se gouverne aisément, l'inassimilable jamais sans la force.") \
    X(STR_HOVER_AGITATION, "La colère qui monte dans la province ; soutenue, elle vire à la révolte — qu'apaisent la stabilité du royaume, la garnison et la légitimité.") \
    X(STR_HOVER_FOI, "L'adhésion de la province à l'idéologie du trône ; convaincue, elle nourrit la légitimité — dissidente, elle couve le schisme.") \
    X(STR_HOVER_SEDITION, "La tension d'une faction forte dont les valeurs s'opposent à la direction du régime ; séditieuse, elle complote le coup d'État pour imposer son éthos.") \
    X(STR_AGIT_CAUSE_COERCION,  "Coercition") \
    X(STR_AGIT_CAUSE_CULTURE,   "Culture étrangère") \
    X(STR_AGIT_CAUSE_CHOC,      "Conquête récente") \
    X(STR_AGIT_CAUSE_GARNISON,  "Garnison") \
    X(STR_DEV_CAUSE_OUTIL,      "Outillage") \
    X(STR_DEV_CAUSE_VILLE,      "Ville") \
    X(STR_DEV_CAUSE_TECH,       "Techniques") \
    X(STR_DEV_CAUSE_PILLAGE,    "Pillage") \
    X(STR_DEV_CAUSE_FRICHE,     "Friche") \
    X(STR_DEV_CAUSE_TERRE,      "Terre rude") \
    X(STR_CAPADM_CAUSE_INST,    "Institutions") \
    X(STR_CAPADM_CAUSE_COERC,   "Coercition bâtie") \
    X(STR_SERV_CAUSE_SAVOIR,    "Savoir bâti") \
    X(STR_SERV_CAUSE_FOI,       "Foi bâtie") \
    X(STR_AGE_FX_EXCHANGE,      "Commerce +%d (durable) · Prospérité +%d · pactes migratoires +%d %%\nDébloque Société III · Marchands +%d (routes vivantes)") \
    X(STR_AGE_FX_DISCOVERY,     "Commerce +%d (durable) · recherche +%d %%\nDébloque Savoir IV · Transgresseurs +%d · Marchands +%d (mondes connus)") \
    X(STR_AGE_FX_EMPIRES,       "Intégration +%d %% (durable)\nDébloque Société V · Conquérants +%d (provinces tenues %d ans)") \
    X(STR_AGE_FX_HEROES,        "Des héros se lèvent — leurs hauts faits marquent les annales") \
    X(STR_AGE_FX_BREACH,        "Brèche +%d — nourrit l'Entropie\nDébloque Savoir V · Transgresseurs +%d (charge faustienne)") \
    X(STR_AGE_FX_LUMIERES,      "Savoir +%d · Légitimité jusqu'à −%d (lettrés contre trône coercitif)\nLégistes +%d · Communautaires +%d") \
    X(STR_AGE_FX_SOULEVEMENTS,  "Légitimité −%d\nCommunautaires +%d (pays révolutionnaires)") \
    X(STR_AGE_FX_TYRANS,        "Sécurité +%d · Diversité −%d\nConquérants +%d · Légistes +%d") \
    X(STR_JLOG_CHOC_EFF, "Agitation accrue, se résorbe avec le temps") \
    X(STR_JLOG_POP,    "Population") \
    X(STR_JLOG_PROD,   "Production") \
    X(STR_JLOG_TRESOR, "Trésor") \
    X(STR_TUTO_TITLE_0, "1 · Ce monde se lit.") \
    X(STR_TUTO_TITLE_1, "2 · Le temps coule en jours.") \
    X(STR_TUTO_TITLE_2, "3 · Ton empire.") \
    X(STR_TUTO_TITLE_3, "4 · Décider coûte.") \
    X(STR_TUTO_TITLE_4, "5 · Les autres.") \
    X(STR_TUTO_TITLE_5, "6 · Le savoir voyage.") \
    X(STR_TUTO_TITLE_6, "7 · La Brèche.") \
    X(STR_TUTO_PAGE_0, "Ici, pas de pourcentages cachés : l'état des choses se dit en MOTS.\nUne province est Unie ou Fracturée, un peuple Loyal ou Frondeur,\nun marché Sain ou En pénurie. Survole : tout se définit en bas d'écran.") \
    X(STR_TUTO_PAGE_1, "En haut à droite : la date, l'âge du monde, la vitesse.\nESPACE met en pause — et en pause, tu peux tout consulter,\ntout ordonner. Rien ne presse jamais que toi.") \
    X(STR_TUTO_PAGE_2, "En haut : tes couronnes, tes vivres, tes matériaux, et la santé de ta couronne —\nStabilité, Légitimité, Cohésion, Prospérité. Clique une province pour la voir\nde près ; ouvre la BARRE DE GAUCHE pour l'empire entier :\nÉconomie, Démographie, Stocks, Armée, Filtres.") \
    X(STR_TUTO_PAGE_3, "Tout ordre — bâtir, exploiter, déplacer, lever — entre dans une FILE\net prend des JOURS. Le prix s'affiche AVANT. Certains leviers rapportent\nvite et coûtent longtemps : mater une révolte tait la rue, pas la colère.") \
    X(STR_TUTO_PAGE_4, "Tes voisins vivent : ils commercent, s'allient, jalousent.\nOn peut les lier — l'allié, le protégé, le serf, la cité marchande —\net chaque lien a son prix. Un embargo est une arme ; une guerre se gagne\nsur le champ, au MORAL, pas au nombre.") \
    X(STR_TUTO_PAGE_5, "Ton arbre a un cœur et un cercle : le cœur se recherche,\nle CERCLE se gagne par le contact — commerce, frontières, peuples gouvernés.\nUne culture qu'on assimile est un savoir qu'on tarit.\nChoisis ce que tu fonds et ce que tu gardes distinct.") \
    X(STR_TUTO_PAGE_6, "Certaines voies sont plus que puissantes — elles sont AVIDES.\nChaque pacte sombre, chaque forge interdite, chaque culte imposé CHARGE le monde.\nLa Brèche n'interdit rien : elle attend. Ton empire tombera — ils tombent tous.\nLa seule question est COMMENT, et ce que tu laisseras debout.") \
    X(STR_MENU_SOUS_TITRE, "un monde qui ne vous attend pas — et qui se lit") \
    X(STR_MENU_JOUER, "Jouer") \
    X(STR_MENU_CHARGER, "Charger") \
    X(STR_MENU_TUTORIEL, "Tutoriel") \
    X(STR_MENU_QUITTER, "Quitter") \
    X(STR_MENU_LANGUE, "Langue : {0}") \
    X(STR_SETUP_TITRE, "FORGER UN MONDE") \
    X(STR_PAUSE_TITRE, "PAUSE") \
    X(STR_PM_REPRENDRE, "Reprendre") \
    X(STR_PM_SAUVER, "Sauver") \
    X(STR_PM_TUTORIEL, "Tutoriel") \
    X(STR_PM_MENU, "Menu principal") \
    X(STR_PM_QUITTER, "Quitter") \
    X(STR_PICK_SAUVER, "SAUVER — choisir un slot") \
    X(STR_PICK_CHARGER, "CHARGER — choisir un slot") \
    X(STR_SLOT_LINE, "Slot {0} — {1}") \
    X(STR_SLOT_ANCIEN, "Slot {0} — sauvegarde d'une ère antérieure") \
    X(STR_SLOT_VIDE, "Slot {0} — vide") \
    X(STR_TUTO_PREC, "◀ préc.") \
    X(STR_TUTO_SUIV, "suiv. ▶") \
    X(STR_TUTO_PAGEFMT, "{0} / 7") \
    X(STR_OCCUPEE_PAR, "Occupée par {0}") \
    X(STR_RAIL_DIPLO, "Diplomatie (G) — pays vivants · guerres · casus belli") \
    X(STR_DIPLO_TITRE, "DIPLOMATIE") \
    X(STR_DIPLO_NEUTRE, "Neutre") \
    X(STR_DIPLO_ALLIE, "Allié") \
    X(STR_DIPLO_GUERRE, "En guerre") \
    X(STR_DIPLO_VASSAL, "Vassal") \
    X(STR_DIPLO_SUZERAIN, "Suzerain") \
    X(STR_DIPLO_DECLARER, "Déclarer la guerre") \
    X(STR_DIPLO_NEGOCIER, "Négocier la paix") \
    X(STR_DIPLO_SANS_CB, "Aucune raison de guerre ne tient (pas de casus belli)") \
    X(STR_DIPLO_SCORE_FMT, "score {0}") \
    X(STR_DIPLO_PAIX_FMT, "Paix : score {0}/50 ou {1}/10 ans") \
    X(STR_DIPLO_RANCUNE, "rancune vive") \
    X(STR_PACT_ACTIF,  "pacte commercial \xe2\x9c\x93") \
    X(STR_PACT_GLOBAL, "pacte \xe2\x9c\x93 \xc2\xb7 acc\xc3\xa8s march\xc3\xa9 global") \
    X(STR_PACT_AUCUN,  "pas de pacte commercial") \
    X(STR_PACT_SIGN,   "Signer un pacte") \
    X(STR_PACT_BREAK,  "Rompre le pacte") \
    X(STR_PACT_HOV,    "Pacte commercial : acc\xc3\xa8s R\xc3\x89""CIPROQUE au march\xc3\xa9 GLOBAL du partenaire si l'un tient un Centre. R\xc3\xa9vocable.") \
    X(STR_DIPLO_MOTIF_FMT, "motif : {0}") \
    X(STR_DIPLO_RACE_FMT, "H\xc3\xa9ritage : {0}") \
    X(STR_DIPLO_STATUT_FMT, "Statut : {0}") \
    X(STR_DIPLO_MENACE_FMT, "Menace : {0}%") \
    X(STR_DIPLO_ACTIONS, "Actions — onglet Diplomatie (G)") \
    X(STR_CB_TERRITORIAL, "frontière revendiquée") \
    X(STR_CB_RELIGIOUS, "schisme idéologique") \
    X(STR_CB_ECONOMIC, "bien monopolisé") \
    X(STR_CB_SUBJUGATION, "assujettissement") \
    X(STR_CB_ANTIPIRATERIE, "course à réprimer") \
    X(STR_PAIX_REFUS, "L'ennemi refuse : la guerre n'a pas assez saigné") \
    X(STR_JRN_GUERRE_PAR, "Guerre déclarée par {0}") \
    X(STR_JRN_GUERRE_CONTRE, "Guerre déclarée à {0}") \
    X(STR_JRN_PAIX, "Paix signée avec {0}") \
    X(STR_JRN_CAPITULE, "Capitulation devant {0}") \
    X(STR_JRN_MORT, "{0} a disparu de la carte") \
    X(STR_DEFAITE_TITRE, "DÉFAITE") \
    X(STR_DEFAITE_LIGNE, "An {0} — votre royaume n'est plus") \
    X(STR_DEFAITE_OBSERVER, "Observer le monde") \
    X(STR_DEFAITE_MENU, "Menu principal") \
    X(STR_ARMEE_DEMOB, "[démobiliser]  {0} régiments rentrent au foyer") \
    X(STR_ARMEE_DEMOB_HOV, "Dissoudre l'armée : les ARMES sont consommées (aucun matériau rendu) ; les hommes RENTRENT à leur point d'origine et redeviennent main-d'œuvre.") \
    X(STR_ARMEE_LEVY_LOCK_GUERRE, "Verrouillé : bâtir la CASERNE (arbre de tech) ouvre le pied de guerre.") \
    X(STR_ARMEE_LEVY_LOCK_MASSE, "Verrouillé : la CONSCRIPTION (après la Caserne) ouvre la levée en masse.") \
    X(STR_FACTION_ETHOS_0, "la voie de la force — armée, guerre, expansion") \
    X(STR_FACTION_ETHOS_1, "la voie de l'or — routes, marchés, profit") \
    X(STR_FACTION_ETHOS_2, "la voie de l'ordre — lois, administration, stabilité") \
    X(STR_FACTION_ETHOS_3, "la voie de la tradition — terre, idéologie, continuité") \
    X(STR_FACTION_ETHOS_4, "la voie de l'interdit — arcane, risque, tabou franchi") \
    X(STR_FACTION_ETHOS_5, "le petit peuple — pain, paix, sécurité du quotidien") \
    X(STR_FACTION_HOV_FMT, "{0} · {1}. Satisfaction {2}% = leur adhésion au régime ; part {3}% = leur poids politique.") \
    X(STR_FACTION_HOV_COUP, " — ALIÉNÉE & PUISSANTE : le coup couve.") \
    X(STR_CENTRE_RESEAU_OUVERT, "Réseau inter-pays OUVERT (un Centre commercial tenu)") \
    X(STR_CENTRE_RESEAU_FERME, "Réseau inter-pays FERMÉ — aucun Centre commercial (en conquérir un)") \
    X(STR_CENTRE_COMMERCIAL, "Centre commercial — hub du réseau inter-régional (le tenir = commercer)") \
    X(STR_PAN_MARCHE, "MARCHÉ") \
    X(STR_MARCHE_PRIX_FMT, "prix courant {0} couronnes l'unité") \
    X(STR_MARCHE_PRIX_HOV, "Le prix du marché intérieur : la demande le tire, l'offre et la vente le détendent. Acheter et vendre se font à CE prix.") \
    X(STR_MARCHE_ROW_HOV, "{0} — en réserve {1}. Acheter ou vendre par lots de 10 au prix courant (les couronnes sortent et entrent par le trésor).") \
    X(STR_MARCHE_HDR_LOCAL,  "bien            stock      prix r\xc3\xa9""f.") \
    X(STR_MARCHE_HDR_MARCHE, "bien          prix    dispo") \
    X(STR_MARCHE_BUY_HOV,    "Acheter (pompe le tr\xc3\xa9sor) \xe2\x80\x94 palier 10, Maj = 100.") \
    X(STR_MARCHE_SELL_HOV,   "Vendre au march\xc3\xa9 \xe2\x80\x94 palier 10, Maj = 100.") \
    X(STR_SLOT_VERROU_FMT, "{0} — verrouillé ({1})") \
    X(STR_BTN_COMPTOIR_FMT, "Bâtir un Comptoir ici  ({0} couronnes)") \
    X(STR_COMPTOIR_HOV, "Le Comptoir branche la province au Centre commercial le plus proche : la marge de transport tombe d'un tiers à ce bout des routes marchandes.") \
    X(STR_BTN_CENTER_FMT, "B\xc3\xa2tir un Centre commercial  ({0} couronnes)") \
    X(STR_CENTER_HOV, "Le Centre commercial fait de cette province un HUB du r\xc3\xa9seau GLOBAL : on y ach\xc3\xa8te/vend au march\xc3\xa9 mondial. C\xc3\xb4tier/estuaire + vocation marchande requis.") \
    X(STR_ENTREPOT_CAP_FMT, "stock {0}/{1} — Entrepôts ×{2}") \
    X(STR_ROW_ENTREPOTS, "Entrepôts") \
    X(STR_TOPBAR_MATERIAUX, "Matériaux") \
    X(STR_RES_BOIS, "Bois") \
    X(STR_RES_ARGILE, "Argile") \
    X(STR_RES_PIERRE, "Pierre") \
    X(STR_RES_OUTILS, "Outils") \
    X(STR_ENTREPOT_HOV, "Sans Entrepôt, le stock régional sature à 200 par ressource (le surplus se perd) ; chaque Entrepôt bâti ajoute +500. Stocker bas, vendre haut.") \
    X(STR_MER_CABOTAGE, "cabotage · vitesse fixe") \
    X(STR_MER_MORTE,    "eaux mortes · ×3 temps") \
    X(STR_MER_VIVE,     "eaux vives") \
    X(STR_MER_COURANT,  "courant · porté ÷2,2 · contre ×2,5") \
    X(STR_MER_DIR_FMT,  "{0} · {1}") \
    X(STR_MER_DIR_EST,  "vers le levant") \
    X(STR_MER_DIR_OUEST,"vers le couchant") \
    X(STR_MER_DIR_SUD,  "vers le sud") \
    X(STR_MER_DIR_NORD, "vers le nord") \
    X(STR_EDI_TRIBUNAL,     "Tribunal") \
    X(STR_EDI_CHANCELLERIE, "Chancellerie") \
    X(STR_EDI_ACADEMIE,     "Académie") \
    X(STR_EDI_GARNISON,     "Garnison") \
    X(STR_EDI_FORTERESSE,   "Forteresse") \
    X(STR_EDI_CITADELLE,    "Citadelle") \
    X(STR_EDI_PORT,         "Port") \
    X(STR_EDI_CARAVANSERAIL,"Caravansérail") \
    X(STR_EDI_MARCHE,       "Marché") \
    X(STR_EDI_ENTREPOT,     "Entrepôt") \
    X(STR_EDI_GRENIER,      "Grenier") \
    X(STR_EDI_IRRIGATION,   "Irrigation") \
    X(STR_EDI_AQUEDUC,      "Aqueduc") \
    X(STR_EDI_SANCTUAIRE,   "Sanctuaire") \
    X(STR_EDI_TEMPLE,       "Temple") \
    X(STR_EDI_CATHEDRALE,   "Cathédrale") \
    X(STR_EDI_BIBLIOTHEQUE, "Bibliothèque") \
    X(STR_EDI_MONASTERE,    "Monastère") \
    X(STR_EDI_ARSENAL,      "Arsenal") \
    X(STR_EDI_AMIRAUTE,     "Amirauté") \
    X(STR_EDI_PORT_MARCHAND,"Port marchand") \
    X(STR_EDI_BIBLIO_MIL,   "Bibliothèque militaire") \
    X(STR_EDI_OBSERVATOIRE, "Observatoire") \
    /* M7 (forks §25) — la CHRONIQUE des fourches : 3 variantes par fork, {0}=lieu
     * (tirage au seed — le monde ne radote pas ; lignes de causalité, pas de prose). */ \
    X(STR_FORK_ARSENAL_0,      "Les maîtres de guerre de {0} transforment les quais en Arsenal.") \
    X(STR_FORK_ARSENAL_1,      "{0} arme ses quais : l'Arsenal s'élève.") \
    X(STR_FORK_ARSENAL_2,      "À {0}, la rade devient Arsenal — la flotte avant le négoce.") \
    X(STR_FORK_AMIRAUTE_0,     "La Chancellerie de {0} impose une doctrine maritime : l'Amirauté naît.") \
    X(STR_FORK_AMIRAUTE_1,     "{0} dote sa rade d'une Amirauté — la mer entre aux registres.") \
    X(STR_FORK_AMIRAUTE_2,     "L'Amirauté de {0} prend la mer en main.") \
    X(STR_FORK_PORT_MARCH_0,   "Les marchands de {0} obtiennent privilèges et entrepôts : le Port marchand devient le cœur de la cité.") \
    X(STR_FORK_PORT_MARCH_1,   "{0} ouvre ses quais au négoce : le Port marchand l'emporte.") \
    X(STR_FORK_PORT_MARCH_2,   "Au Port marchand de {0}, tout s'achète — même la paix.") \
    X(STR_FORK_FORGE_0,        "Le Fer céleste est confié aux forgerons-runiers de {0}. La victoire se rapproche ; le réel, moins stable.") \
    X(STR_FORK_FORGE_1,        "{0} allume sa Forge de Runes — le flux s'épaissit.") \
    X(STR_FORK_FORGE_2,        "À {0}, le métal tombé du ciel devient arme. Le flux s'en souvient.") \
    X(STR_FORK_ALAMBIC_0,      "Le Salpêtre distillé à {0} réduit les accidents de flux — les guildes réclament leur part.") \
    X(STR_FORK_ALAMBIC_1,      "{0} distille la stabilité : l'Alambic apaise le flux.") \
    X(STR_FORK_ALAMBIC_2,      "L'Alambic de {0} vend le calme — au prix du salpêtre.") \
    X(STR_EDI_COMPTOIR,     "Comptoir") \
    X(STR_EDI_BANQUE,       "Banque") \
    X(STR_EDI_TRADE_CENTER, "Centre commercial") \
    X(STR_FAC_CONQUERANT,    "Conquérants") \
    X(STR_FAC_MARCHAND,      "Marchands") \
    X(STR_FAC_LEGISTE,       "Légistes") \
    X(STR_FAC_GARDIEN,       "Gardiens") \
    X(STR_FAC_TRANSGRESSEUR, "Transgresseurs") \
    X(STR_FAC_COMMUNAUTAIRE, "Communautaires") \
    /* Échec fatal au démarrage (boîte SDL native, avant toute fenêtre de jeu) — {0}=détail SDL. */ \
    X(STR_FATAL_TITRE, "SCPS — échec au démarrage") \
    X(STR_FATAL_SDL,   "SCPS n'a pas pu initialiser l'affichage.\n\n{0}") \
    /* Écran de chargement (genèse + amorce sur un thread — l'UI reste réactive). */ \
    X(STR_LOADING_MONDE, "Façonnage du monde…") \
    X(STR_LOADING_EVEIL, "Le monde s'éveille — des années passent…") \
    /* Q1 — Le Conseil (I7) : sièges, effets, candidats (maisons), libellés UI. */ \
    X(STR_COUNCIL_TITRE, "CONSEIL") \
    X(STR_COUNCIL_SEAT_0, "Savoir") \
    X(STR_COUNCIL_SEAT_1, "Société") \
    X(STR_COUNCIL_SEAT_2, "Industrie") \
    X(STR_COUNCIL_EFF_0, "recherche") \
    X(STR_COUNCIL_EFF_1, "promotion") \
    X(STR_COUNCIL_EFF_2, "manufactures") \
    X(STR_COUNCIL_VACANT, "— siège vacant —") \
    X(STR_COUNCIL_NOMMER, "Nommer") \
    X(STR_COUNCIL_RENVOYER, "Renvoyer") \
    X(STR_COUNCIL_SEAT_FMT, "{0} — +{1}% {2}") \
    X(STR_COUNCIL_SEATED_FMT, "{0} · tier {1} · {2} couronnes/mois") \
    X(STR_COUNCIL_CAND_FMT, "{0} · tier {1} · {2} couronnes") \
    X(STR_COUNCIL_NAME_0, "Maison Vœrn") \
    X(STR_COUNCIL_NAME_1, "Comptoir Aldric") \
    X(STR_COUNCIL_NAME_2, "Guilde Harmel") \
    X(STR_COUNCIL_NAME_3, "Banque Orlec") \
    X(STR_COUNCIL_NAME_4, "Maison Tessari") \
    X(STR_COUNCIL_NAME_5, "Cercle Velmor") \
    X(STR_COUNCIL_NAME_6, "Loge Brask") \
    X(STR_COUNCIL_NAME_7, "Syndic Dovric") \
    /* V2a — LE CONSEIL VIVANT : l'ambiance du ministre (mot dérivé de la loyauté 0-100). */ \
    X(STR_COUNCIL_MOOD_DEVOUE, "dévoué") \
    X(STR_COUNCIL_MOOD_LOYAL, "loyal") \
    X(STR_COUNCIL_MOOD_TIEDE, "tiède") \
    X(STR_COUNCIL_MOOD_AIGRI, "aigri") \
    X(STR_COUNCIL_MOOD_TRAHISON, "AU BORD DE LA TRAHISON") \
    X(STR_COUNCIL_PAY_LABEL, "Paie") \
    /* CAPSTONE §27 — Entropie mondiale (destin partagé, pas par-pays). */ \
    X(STR_BANDE_ENTROPIE_0, "Stable") \
    X(STR_BANDE_ENTROPIE_1, "Frémissante") \
    X(STR_BANDE_ENTROPIE_2, "Instable") \
    X(STR_BANDE_ENTROPIE_3, "Au bord") \
    X(STR_HOVER_ENTROPIE, "La dérive du monde vers la Brèche : le savoir faustien et la transmutation l'attisent. Au seuil, le réel cède.") \
    X(STR_AUGURE_ENTROPIE_0, "Le ciel se voile d'une teinte qu'aucun almanach ne nomme.") \
    X(STR_AUGURE_ENTROPIE_1, "Les aiguilles s'affolent ; la matière hésite sur ses propres lois.") \
    X(STR_AUGURE_ENTROPIE_2, "Le réel s'amincit. Le seuil de la Brèche n'attend plus que sa forme.") \
    /* MODIFICATEURS PROVINCIAUX (diégétiques) — slot UI province (multiple). */ \
    X(STR_PMOD_SECTION,        "MODIFICATEURS") \
    X(STR_PMOD_FAVEUR,         "Faveur") \
    X(STR_PMOD_FLEAU,          "Fléau") \
    X(STR_PMOD_CICATRICE_NOM,  "Cicatrice de révolte") \
    X(STR_PMOD_CICATRICE_EFF,  "Une province récemment soulevée ou saccagée se développe mal : croissance et production entaillées tant que la plaie ne s'est pas refermée.") \
    X(STR_PMOD_ABONDANCE_NOM,  "Terre d'abondance") \
    X(STR_PMOD_ABONDANCE_EFF,  "Une terre vaste, nourrie et en paix appelle les familles : la natalité s'envole tant que ses champs ne sont pas pleins.") \
    X(STR_PMOD_FERVEUR_NOM,    "Ferveur fondatrice") \
    X(STR_PMOD_FERVEUR_EFF,    "Une colonie fraîchement fondée a faim d'avenir : ses premières années portent un élan de natalité qui s'apaise à mesure qu'elle s'enracine.") \
    X(STR_PMOD_RECONSTRUCTION_NOM, "Reconstruction") \
    X(STR_PMOD_RECONSTRUCTION_EFF, "Une fois la plaie d'une révolte ou d'un sac refermée, la province se relève d'un bond : la reconstruction d'après-choc presse la natalité.") \
    X(STR_PMOD_LIMON_NOM,      "Limon fertile") \
    X(STR_PMOD_LIMON_EFF,      "L'embouchure d'un grand fleuve dépose un limon gras : les champs du delta nourrissent une population dense.") \
    X(STR_PMOD_GIBIER_NOM,     "Gibier abondant") \
    X(STR_PMOD_GIBIER_EFF,     "Les bois fourmillent de gibier : la chasse garnit les tables et soutient une population plus dense.") \
    X(STR_PMOD_HALIEU_NOM,     "Manne halieutique") \
    X(STR_PMOD_HALIEU_EFF,     "Des bancs de poissons grouillent au large : la pêche nourrit une côte populeuse.") \
    X(STR_PMOD_ADMIN_NOM,      "Bonne administration") \
    X(STR_PMOD_ADMIN_EFF,      "Des institutions solides tiennent l'ordre et les services : à l'abri du désordre, les familles prospèrent.") \
    X(STR_PMOD_ANNEX_NOM,      "Annexion récente") \
    X(STR_PMOD_ANNEX_EFF,      "Une province arrachée à son ancien maître par l'annexion porte une plaie de fierté : la stabilité reste fragile et l'humeur boude tant que les esprits ne se sont pas faits à la nouvelle bannière.") \
    X(STR_PMOD_MUTATION_NOM,   "Mutations") \
    X(STR_PMOD_MUTATION_EFF,   "Le Réplicateur transmute le flux en bois à gros rendement — mais quelque chose d'autre transmute aussi : les corps s'adaptent, se multiplient plus vite. Une bénédiction qui n'est pas sans prix.") \
    /* GLOSSAIRE des concepts (hover_*) — TITRES des fiches (la définition réutilise
     * les STR_HOVER_* existants). Catégorie & alias vivent dans la table C
     * (scps_lang.c) ; ici, seul le mot-titre face-joueur. */ \
    X(STR_GLOSS_STAB,      "Stabilité") \
    X(STR_GLOSS_LEGIT,     "Légitimité") \
    X(STR_GLOSS_CONCORDE,  "Cohésion") \
    X(STR_GLOSS_ASSISE,    "Assise") \
    X(STR_GLOSS_PROSP,     "Prospérité") \
    X(STR_GLOSS_MARCHE,    "Marché") \
    X(STR_GLOSS_AISANCE,   "Aisance") \
    X(STR_GLOSS_HUMEUR,    "Humeur") \
    X(STR_GLOSS_LIGNEE,    "Lignée") \
    X(STR_GLOSS_AGITATION, "Agitation") \
    X(STR_GLOSS_SAVOIR,    "Savoir") \
    X(STR_GLOSS_PRESAGE,   "Présage") \
    /* MANUFACTURES SIGNATURE D'ÉTHOS (désir croisé, docs/DESIGN_manufactures_ethos.md) —
     * 6 biens + 6 ateliers, un par éthos. */ \
    X(STR_RES_HEAUMES,       "Heaumes de guerre") \
    X(STR_RES_PARURES,       "Parures de gloire") \
    X(STR_RES_HORLOGES,      "Horloges réglées") \
    X(STR_RES_REGISTRES,     "Registres scellés") \
    X(STR_RES_COLIFICHETS,   "Colifichets exotiques") \
    X(STR_RES_OUVRAGES,      "Ouvrages d'agrément") \
    X(STR_BLD_HEAUMERIE,         "Heaumerie") \
    X(STR_BLD_PARURIER,          "Atelier de parurier") \
    X(STR_BLD_HORLOGER,          "Atelier d'horloger") \
    X(STR_BLD_CHANCELLERIE_LUX,  "Chancellerie de luxe") \
    X(STR_BLD_COMPTOIR_ARTISAN,  "Comptoir d'artisan") \
    X(STR_BLD_ATELIER_SEREIN,    "Atelier serein") \
    X(STR_CULTURE_PARENTS, "Parents : ") \
    X(STR_CULTURE_RACINES, "Racines : ") \
    X(STR_CULTURE_SUBSTRAT,"Substrat : ") \
    X(STR_CULTURE_PARENTE, "Parenté") \
    /* MONNAIE M9 — V2 : LA DEMANDE D'EMPRUNT DIPLOMATIQUE, l'état en MOTS (jamais un
     * flottant, doctrine membrane). La résolution est SYNCHRONE au drain (ai_consider_
     * offer tranche dans le MÊME tick que la demande) — il n'existe pas d'état
     * intermédiaire persistant côté moteur, seulement AUCUNE/ACCORDÉ/REFUSÉ. */ \
    X(STR_LOAN_AUCUNE,  "Aucune demande") \
    X(STR_LOAN_ACCORDE, "L'État accorde le prêt") \
    X(STR_LOAN_REFUSE,  "L'État refuse le prêt") \
    /* VAGUE STR_* (2026-08-15, audit 2026-08-12) — littéraux face-joueur de
     * scps_api.c/scps_readout.c aux sites audités : marche/ravitaillement,
     * bataille (Choc/Accalmie), religion (Sans foi), fusion culturelle,
     * commerce inter-pays, identités de conseillers, relations/gates diplo,
     * flavors héritage+éthos, lecture du savoir (chemin), augures, vocation
     * de province. Reste hors vague : le solde du fichier (baseline ratchet,
     * cf. Makefile lang-check). */ \
    X(STR_MARCH_REASON_DEFAULT, "Aperçu indisponible") \
    X(STR_MARCH_REASON_0, "Route praticable") \
    X(STR_MARCH_REASON_1, "Corps invalide") \
    X(STR_MARCH_REASON_2, "Corps engagé en bataille") \
    X(STR_MARCH_REASON_3, "Corps en mer ou en débarquement") \
    X(STR_MARCH_REASON_4, "Corps brisé en déroute") \
    X(STR_MARCH_REASON_5, "Destination invalide") \
    X(STR_MARCH_REASON_6, "Corps sans effectif") \
    X(STR_MARCH_REASON_7, "Aucune route terrestre") \
    X(STR_MARCH_ARRIVAL_0, "Rester sur place") \
    X(STR_MARCH_ARRIVAL_1, "Repositionnement") \
    X(STR_MARCH_ARRIVAL_2, "Marche vers un siège") \
    X(STR_REFILL_REASON_INVALID,      "Corps invalide") \
    X(STR_REFILL_REASON_NOT_NATIONAL, "Ravitaillement possible uniquement sur une région nationale") \
    X(STR_REFILL_REASON_NO_LINES,     "Aucune ligne d'unité à renforcer") \
    X(STR_REFILL_REASON_FULL,         "Corps déjà à pleine force (nominal atteint)") \
    X(STR_REFILL_REASON_NO_POP,       "Aucune population de la bonne classe n'est mobilisable") \
    X(STR_REFILL_REASON_COVERED,      "Renfort couvert par la population et l'arsenal national") \
    X(STR_REFILL_REASON_PARTIAL,      "Renfort partiel garanti ; le marché peut fournir les armes manquantes") \
    X(STR_BATTLE_STAGE_CHOC,     "Choc") \
    X(STR_BATTLE_STAGE_ACCALMIE, "Accalmie") \
    X(STR_FOI_SANS, "Sans foi") \
    X(STR_FUSION_AUCUN_CONTACT,   "Aucun contact commercial soutenu") \
    X(STR_FUSION_PIVOT_TRANSFORME,"Le contact commercial transforme la province-pivot de la région") \
    X(STR_FUSION_NON_SEDENTARISE, "Culture locale non sédentarisée") \
    X(STR_FUSION_PORTE_OUVERTE,   "Porte de fusion ouverte") \
    X(STR_FUSION_PORTE_FERMEE,    "Porte fermée : contact ou institutions insuffisants") \
    X(STR_TRADE_STATUT_GUERRE,     "guerre") \
    X(STR_TRADE_STATUT_EMBARGO,    "embargo") \
    X(STR_TRADE_STATUT_FLORISSANT, "florissant") \
    X(STR_TRADE_STATUT_MODESTE,    "modeste") \
    X(STR_CONS_NOM_0, "Rigoriste") \
    X(STR_CONS_NOM_1, "Courtisan") \
    X(STR_CONS_NOM_2, "Austère") \
    X(STR_CONS_NOM_3, "Réformateur") \
    X(STR_CONS_NOM_4, "Vétéran") \
    X(STR_CONS_NOM_5, "Ambitieux") \
    X(STR_CONS_NOM_6, "Loyaliste") \
    X(STR_CONS_NOM_7, "Vénal") \
    X(STR_CONS_FLAVOR_0, "Chaque exception lui paraît être la première pierre d'une ruine.") \
    X(STR_CONS_FLAVOR_1, "Il sait qui doit être salué, qui doit être payé et qui doit croire que les deux gestes se valent.") \
    X(STR_CONS_FLAVOR_2, "Son train de maison tient dans deux coffres. Sa reconnaissance aussi.") \
    X(STR_CONS_FLAVOR_3, "Aucune institution ne lui semble achevée tant qu'il reste possible de la démonter.") \
    X(STR_CONS_FLAVOR_4, "Il a servi trois règnes et appris à ne confondre aucun d'eux avec l'État.") \
    X(STR_CONS_FLAVOR_5, "Il appelle service la distance qui le sépare encore du pouvoir.") \
    X(STR_CONS_FLAVOR_6, "Il sert la couronne avec assez de ferveur pour inquiéter celui qui la porte.") \
    X(STR_CONS_FLAVOR_7, "Il connaît le prix de chaque secret, sauf celui du dernier.") \
    X(STR_RELATION_GUERRE, "Guerre") \
    X(STR_DIPLO_TERRITOIRE_INCONNU, "territoire inconnu") \
    X(STR_GATE_EMISSAIRE_DISPO,      "Émissaire disponible") \
    X(STR_GATE_PAS_DEJA_GUERRE,      "Pas déjà en guerre") \
    X(STR_GATE_AUCUNE_TREVE,         "Aucune trêve") \
    X(STR_GATE_CASUS_BELLI,          "Casus belli utilisable") \
    X(STR_GATE_EN_GUERRE_CIBLE,      "En guerre avec la cible") \
    X(STR_GATE_PAS_GUERRE,           "Pas en guerre") \
    X(STR_GATE_PAS_ALLIES,           "Pas déjà alliés") \
    X(STR_GATE_CRENEAU_ALLIANCE,     "Créneau d'alliance libre") \
    X(STR_GATE_PAS_PACTE_COMMERCIAL, "Pas de pacte commercial en cours") \
    X(STR_GATE_PAS_PACTE_MIGRATOIRE, "Pas de pacte migratoire en cours") \
    X(STR_GATE_RELATION_COMMERCABLE, "Relation commerçable") \
    X(STR_GATE_OR_SUFFISANT,         "Couronnes suffisantes") \
    X(STR_GATE_AUCUNE_INTRIGUE,      "Aucune intrigue déjà lancée") \
    X(STR_GATE_INFLUENCE_SUFFISANTE, "Influence politique suffisante") \
    X(STR_DIPLO_REASON_INVALID_TARGET,        "Cible diplomatique invalide ou inconnue") \
    X(STR_DIPLO_REASON_EMISSARY_BUSY,         "Émissaire en tournée") \
    X(STR_DIPLO_REASON_OK,                    "Action disponible") \
    X(STR_DIPLO_REASON_ALREADY_WAR,           "Déjà en guerre avec ce pays") \
    X(STR_DIPLO_REASON_TRUCE_ACTIVE,          "Trêve en cours") \
    X(STR_DIPLO_REASON_NO_CB,                 "Aucun casus belli utilisable") \
    X(STR_DIPLO_REASON_NOT_AT_WAR,            "Vous n'êtes pas en guerre avec ce pays") \
    X(STR_DIPLO_REASON_AT_WAR,                "Impossible pendant la guerre") \
    X(STR_DIPLO_REASON_ALREADY_ALLIED,        "Alliance déjà conclue") \
    X(STR_DIPLO_REASON_NO_ALLIANCE_SLOT,      "Aucun créneau d'alliance libre") \
    X(STR_DIPLO_REASON_PACT_EXISTS,           "Pacte commercial déjà conclu") \
    X(STR_DIPLO_REASON_MIGRATION_PACT_EXISTS, "Pacte migratoire déjà conclu") \
    X(STR_DIPLO_REASON_EMBARGO_UNAVAILABLE,   "Embargo indisponible") \
    X(STR_DIPLO_REASON_INTRIGUE_IN_PROGRESS,  "Revendication en fabrication") \
    X(STR_DIPLO_REASON_CLAIM_READY,           "Une revendication est déjà prête") \
    X(STR_DIPLO_REASON_INSUFFICIENT_GOLD,     "Couronnes insuffisantes pour financer l'intrigue") \
    X(STR_DIPLO_REASON_INSUFFICIENT_INFLUENCE,"Influence politique insuffisante pour cet acte") \
    X(STR_DIPLO_REASON_FABRICATION_UNAVAILABLE,"Intrigue indisponible pour l'instant") \
    X(STR_INFLUENCE_HOVER,      "%d nobles · %d bourgeois · %d journaliers × le Conseil (rang moyen %s sur 3 sièges — un siège vide compte pour un rang I)") \
    X(STR_INFLUENCE_HOVER_VIDE, "%d nobles · %d bourgeois · %d journaliers × le Conseil (aucun ministre en siège — plancher)") \
    X(STR_INFLUENCE_RANK_I,     "I") \
    X(STR_INFLUENCE_RANK_II,    "II") \
    X(STR_INFLUENCE_RANK_III,   "III") \
    X(STR_INFLUENCE_COURANT_ARISTO,    " — Aristocratie relève les nobles") \
    X(STR_INFLUENCE_COURANT_BOURGEOIS, " — Bourgeoisie relève les bourgeois") \
    X(STR_INFLUENCE_COURANT_LABORER,   " — Populaire relève les journaliers") \
    X(STR_INFLUENCE_COURANT_DIVIN,     " · %d fidèles") \
    X(STR_HERITAGE_FLAVOR_0, "Leurs généalogies commencent avant les premiers calendriers, dans des siècles dont les ruines seules se souviennent.") \
    X(STR_HERITAGE_FLAVOR_1, "Ils disent que tout serment ressemble à un métal : il révèle sa valeur seulement lorsqu'on le chauffe assez pour le briser.") \
    X(STR_HERITAGE_FLAVOR_2, "Leur première horloge mesurait les saisons. La seconde mesura le travail. La troisième apprit aux deux à rapporter de l'or.") \
    X(STR_HERITAGE_FLAVOR_3, "Ils ont porté tant de lois, de langues et de couronnes qu'ils appellent désormais tradition l'art de changer sans disparaître.") \
    X(STR_HERITAGE_FLAVOR_4, "Leurs frontières suivent les canaux, leurs fêtes les moissons et leurs souvenirs les champs que leurs ancêtres ont refusé d'abandonner.") \
    X(STR_HERITAGE_FLAVOR_5, "Un étranger leur demanda où finissait la famille. On lui montra les tombes, les troupeaux, les guerriers et enfin l'horizon.") \
    X(STR_ETHOS_EPITHETE_0, "Horde") \
    X(STR_ETHOS_EPITHETE_1, "Clans") \
    X(STR_ETHOS_EPITHETE_2, "Ordre") \
    X(STR_ETHOS_EPITHETE_3, "Couronne") \
    X(STR_ETHOS_EPITHETE_4, "Ligue") \
    X(STR_ETHOS_EPITHETE_5, "Havre") \
    X(STR_ETHOS_HINT_0, "Conquête : pousse la coercition, mauvais intégrateur.") \
    X(STR_ETHOS_HINT_1, "Gloire & razzia : honneur martial, digère mal.") \
    X(STR_ETHOS_HINT_2, "Hiérarchie & discipline : l'État qui tient l'ordre.") \
    X(STR_ETHOS_HINT_3, "Bâtisseur d'institutions : tient la diversité.") \
    X(STR_ETHOS_HINT_4, "Profit & carrefours : prospère par le commerce.") \
    X(STR_ETHOS_HINT_5, "Consentement seul : ne fracture jamais, pacifique.") \
    X(STR_ETHOS_FLAVOR_0, "Ils ne demandent pas si la frontière peut être franchie, seulement combien d'hommes il faudra pour qu'elle cesse d'exister.") \
    X(STR_ETHOS_FLAVOR_1, "Une dette peut être oubliée, une défaite réparée. Une honte, elle, attend patiemment les petits-fils.") \
    X(STR_ETHOS_FLAVOR_2, "Chaque personne connaît sa place, chaque place son devoir et chaque devoir le sceau qui le rend incontestable.") \
    X(STR_ETHOS_FLAVOR_3, "Le royaume ne repose pas sur la volonté d'un seul homme, mais sur mille registres qui refusent obstinément de se contredire.") \
    X(STR_ETHOS_FLAVOR_4, "Ils ne conquièrent pas les ports. Ils y prêtent de l'or jusqu'à ce que les clés deviennent une modalité de remboursement.") \
    X(STR_ETHOS_FLAVOR_5, "Ils ont juré de ne prendre aucune vie. Leurs voisins débattent encore pour savoir si cette promesse est une vertu ou une invitation.") \
    X(STR_LEVIER_NOM_0, "Croissance de la population") \
    X(STR_LEVIER_NOM_1, "Production") \
    X(STR_LEVIER_NOM_2, "Rayonnement diplomatique") \
    X(STR_LEVIER_NOM_3, "Coercition") \
    X(STR_LEVIER_NOM_4, "Capacité de l'État") \
    X(STR_LEVIER_NOM_5, "Assimilation des minorités") \
    X(STR_LEVIER_NOM_6, "Magie faustienne") \
    X(STR_LEVIER_NOM_7, "Dérive culturelle") \
    X(STR_LEVIER_NOM_8, "Fracture") \
    X(STR_SYNC_CHEMIN_ACQUIS,  "acquis — diffusé par le contact, et gardé même si la source s'est fondue") \
    X(STR_SYNC_CHEMIN_JAMAIS,  "tradition jamais côtoyée — il faut entrer en contact avec ses porteurs") \
    X(STR_SYNC_CHEMIN_SURFACE, "savoir de surface : le comptoir ne transmet pas l'art profond — gouverne ou voisine cette culture, et légitime le sol") \
    X(STR_SYNC_CHEMIN_PORTEE,  "à portée — il manque le socle (recherche le nœud parent du cercle)") \
    X(STR_AUGURE_SECESSION,           "Les marges parlent de se gouverner seules.") \
    X(STR_AUGURE_REVOLTE,             "La rue gronde contre le trône.") \
    X(STR_AUGURE_COERCITION_FRAGILE,  "L'ordre tient — mais par la peur seule.") \
    X(STR_VOC_GRENIER,    "Grenier") \
    X(STR_VOC_PATURES,    "Pâtures") \
    X(STR_VOC_PECHERIES,  "Pêcheries") \
    X(STR_VOC_MINE,       "Mine") \
    X(STR_VOC_ATELIER,    "Atelier") \
    X(STR_VOC_COMPTOIR,   "Comptoir") \
    X(STR_VOC_SANCTUAIRE, "Sanctuaire") \
    X(STR_VOC_MARCHE,     "Marche") \
    X(STR_MODE_DEFAUT,    "Carte : terrain et frontières") \
    X(STR_MODE_POLITIQUE, "Carte : territoires par pays") \
    X(STR_MODE_NATURE,    "Carte : terrain seul, sans frontières") \
    X(STR_MODE_MARCHE,    "Carte : bassins de marché") \
    X(STR_MARCHE_DE,      "Marché de") \
    X(STR_MARCHE_AUCUN,   "Aucun marché atteignable") \
    X(STR_MODE_RELIGION,  "Carte : foi dominante") \
    X(STR_MODE_CULTURE,   "Carte : culture dominante") \
    /* ── LES DESSEINS — branche du SOL. Quatre bandes PARALLÈLES de 12 slots
     * (dessein_display_slot) : 0-3 le tronc + le pivot · 4-7 la voie Conquête ·
     * 8-11 la voie Vassalisation. N = nom court (D6 : un mot, une action, une
     * situation) · O = l'objectif, gabarit {0} = le lieu ou la couronne visée ·
     * R = la récompense en UNE ligne (jamais un levier moteur face joueur) ·
     * F = la saveur, une phrase. L'ORDRE DES QUATRE BANDES EST LE CONTRAT :
     * scps_api.c les lit par tr_band(base + slot). */ \
    X(STR_DESS_SOL,        "Le Sol") \
    X(STR_DESS_SOL_VOIE_A, "Conquête") \
    X(STR_DESS_SOL_VOIE_B, "Vassalisation") \
    X(STR_DESS_ATTENTE,    "aucune terre à portée") \
    X(STR_DESS_SOL_N0,  "Unification") \
    X(STR_DESS_SOL_N1,  "Expansion") \
    X(STR_DESS_SOL_N2,  "Le rival") \
    X(STR_DESS_SOL_N3,  "Le choix du sol") \
    X(STR_DESS_SOL_N4,  "Pacification") \
    X(STR_DESS_SOL_N5,  "Les marches") \
    X(STR_DESS_SOL_N6,  "Capitale rivale") \
    X(STR_DESS_SOL_N7,  "Hégémonie") \
    X(STR_DESS_SOL_N8,  "Premier vassal") \
    X(STR_DESS_SOL_N9,  "Trois vassaux") \
    X(STR_DESS_SOL_N10, "Intégration") \
    X(STR_DESS_SOL_N11, "Hégémonie") \
    X(STR_DESS_SOL_O0,  "Tenir et peupler toute la vallée de {0}") \
    X(STR_DESS_SOL_O1,  "Prendre {0}, par les armes ou par la charrue") \
    X(STR_DESS_SOL_O2,  "Arracher {0} au rival, ou lui faire plier le genou") \
    X(STR_DESS_SOL_O3,  "Choisir comment tenir le sol : l'épée ou le serment") \
    X(STR_DESS_SOL_O4,  "Refermer la plaie de {0}") \
    X(STR_DESS_SOL_O5,  "Tenir les trois marches, à commencer par {0}") \
    X(STR_DESS_SOL_O6,  "Posséder {0}, la capitale du rival") \
    X(STR_DESS_SOL_O7,  "Tenir les deux cinquièmes du continent") \
    X(STR_DESS_SOL_O8,  "Faire de {0} un vassal") \
    X(STR_DESS_SOL_O9,  "Lier trois couronnes, à commencer par {0}") \
    X(STR_DESS_SOL_O10, "Voir {0} ne plus se distinguer de nous") \
    X(STR_DESS_SOL_O11, "Tenir ou faire jurer les deux cinquièmes du continent") \
    X(STR_DESS_SOL_R0,  "Institutions renforcées dans la capitale") \
    X(STR_DESS_SOL_R1,  "Revendication sur la marche suivante · reconstruction") \
    X(STR_DESS_SOL_R2,  "Les torts consignés : griefs valides 8 ans au lieu de 5, pendant 20 ans") \
    X(STR_DESS_SOL_R3,  "Le choix EST la récompense — et il ne se reprend pas") \
    X(STR_DESS_SOL_R4,  "Annexions moins douloureuses pendant 20 ans · reconstruction") \
    X(STR_DESS_SOL_R5,  "Revendication sur la capitale rivale · digestion plus rapide 20 ans") \
    X(STR_DESS_SOL_R6,  "Le meilleur ministre du Royaume entre au Conseil, sans bourse") \
    X(STR_DESS_SOL_R7,  "La Porte du sol : institutions et garde bâties dans la capitale") \
    X(STR_DESS_SOL_R8,  "Le crédit du serment : vassalité mieux vue pendant 20 ans") \
    X(STR_DESS_SOL_R9,  "Vassaux intégrables plus tôt pendant 20 ans") \
    X(STR_DESS_SOL_R10, "L'héritier du vassal entre au Conseil, sans bourse") \
    X(STR_DESS_SOL_R11, "La Chambre des serments : institutions bâties dans la capitale") \
    X(STR_DESS_SOL_F0,  "On a fini de compter les hameaux : chaque feu répond au même ban.") \
    X(STR_DESS_SOL_F1,  "La borne est déplacée d'un cran. C'est la première fois.") \
    X(STR_DESS_SOL_F2,  "Il a un nom, et ce nom est dans un registre qui ne se ferme plus.") \
    X(STR_DESS_SOL_F3,  "On tiendra le sol par l'épée, ou par le serment. Pas les deux.") \
    X(STR_DESS_SOL_F4,  "Prendre est l'affaire d'un été ; faire oublier, celle d'un règne.") \
    X(STR_DESS_SOL_F5,  "Une marche est un accident ; trois marches sont une frontière.") \
    X(STR_DESS_SOL_F6,  "Sa couronne est dans ta chapelle et son chancelier à ta table.") \
    X(STR_DESS_SOL_F7,  "On ne dit plus le royaume et le continent : on dit le même mot.") \
    X(STR_DESS_SOL_F8,  "Un homme a plié le genou, et trois autres ont regardé.") \
    X(STR_DESS_SOL_F9,  "Trois fils tendus ne font pas trois liens : ils font une surface.") \
    X(STR_DESS_SOL_F10, "Son fils parle notre langue sans accent.") \
    X(STR_DESS_SOL_F11, "Tu ne possèdes pas le continent. Il te répond, et c'est moins cher.") \
    /* ═══ LES DOCTRINES (docs/DESIGN_DOCTRINES_ANNEXE.md) — 17 doctrines ×
     * (nom + hover rapide) + 102 idées × (nom + LA ligne de bonus). Aucun
     * littéral face-joueur hors de cette table (CLAUDE.md §langue). ═══ */ \
    X(STR_DOCT_OFFENSE_NAME,  "Offense") \
    X(STR_DOCT_OFFENSE_HOVER, "Le fer d'abord : armes, discipline, butin et prétextes. La doctrine de qui frappe le premier.") \
    X(STR_IDEA_OFFENSE_ARSENAUX_NAME,  "Arsenaux") \
    X(STR_IDEA_OFFENSE_ARSENAUX_BONUS, "+25 % d'armes produites, rouille ÷2") \
    X(STR_IDEA_OFFENSE_DISCIPLINE_NAME,  "Discipline") \
    X(STR_IDEA_OFFENSE_DISCIPLINE_BONUS, "+10 % de dégâts au combat") \
    X(STR_IDEA_OFFENSE_OST_NAME,  "Ost permanent") \
    X(STR_IDEA_OFFENSE_OST_BONUS, "Renfort automatique des armées, solde de guerre payée en paix") \
    X(STR_IDEA_OFFENSE_BUTIN_NAME,  "Butin") \
    X(STR_IDEA_OFFENSE_BUTIN_BONUS, "+30 % de butin de siège, +15 % au sac") \
    X(STR_IDEA_OFFENSE_PRETEXTES_NAME,  "Prétextes") \
    X(STR_IDEA_OFFENSE_PRETEXTES_BONUS, "−40 % de coût de revendication, maturation −30 %") \
    X(STR_IDEA_OFFENSE_LEVEE_NAME,  "Grande levée") \
    X(STR_IDEA_OFFENSE_LEVEE_BONUS, "+30 % de limite de force") \
    X(STR_DOCT_DEFENSE_NAME,  "Défense") \
    X(STR_DOCT_DEFENSE_HOVER, "On ne gagne pas la guerre : on la fait perdre. Remparts, magasins, terre brûlée.") \
    X(STR_IDEA_DEFENSE_REMPARTS_NAME,  "Remparts") \
    X(STR_IDEA_DEFENSE_REMPARTS_BONUS, "+30 % de défense des places") \
    X(STR_IDEA_DEFENSE_MAGASINS_NAME,  "Magasins") \
    X(STR_IDEA_DEFENSE_MAGASINS_BONUS, "+25 % de vivres de siège") \
    X(STR_IDEA_DEFENSE_BAN_NAME,  "Ban") \
    X(STR_IDEA_DEFENSE_BAN_BONUS, "Milice levée sur-le-champ dans une province envahie") \
    X(STR_IDEA_DEFENSE_CORVEES_NAME,  "Corvées") \
    X(STR_IDEA_DEFENSE_CORVEES_BONUS, "−20 % de coût des fortifications") \
    X(STR_IDEA_DEFENSE_TERRE_BRULEE_NAME,  "Terre brûlée") \
    X(STR_IDEA_DEFENSE_TERRE_BRULEE_BONUS, "−40 % de butin pris par l'envahisseur") \
    X(STR_IDEA_DEFENSE_GENIE_NAME,  "Génie") \
    X(STR_IDEA_DEFENSE_GENIE_BONUS, "Fortifications un palier de tech en avance") \
    X(STR_DOCT_COMMERCE_NAME,  "Commerce") \
    X(STR_DOCT_COMMERCE_HOVER, "Ce qui circule librement enrichit plus que ce qu'on enferme. Exclusive du Mercantilisme.") \
    X(STR_IDEA_COMMERCE_FRANCHISES_NAME,  "Franchises") \
    X(STR_IDEA_COMMERCE_FRANCHISES_BONUS, "−25 % de droits de douane") \
    X(STR_IDEA_COMMERCE_ROUTES_LONGUES_NAME,  "Routes longues") \
    X(STR_IDEA_COMMERCE_ROUTES_LONGUES_BONUS, "+30 % de portée du marché") \
    X(STR_IDEA_COMMERCE_COMPTOIR_NAME,  "Comptoir") \
    X(STR_IDEA_COMMERCE_COMPTOIR_BONUS, "Fonder un comptoir chez une cité-état, péage partagé") \
    X(STR_IDEA_COMMERCE_NEGOCE_NAME,  "Négoce") \
    X(STR_IDEA_COMMERCE_NEGOCE_BONUS, "−15 % de marge d'import chez les tiers") \
    X(STR_IDEA_COMMERCE_GUILDES_NAME,  "Guildes marchandes") \
    X(STR_IDEA_COMMERCE_GUILDES_BONUS, "+30 % de volume marchand bourgeois") \
    X(STR_IDEA_COMMERCE_LIBRE_ECHANGE_NAME,  "Libre-échange") \
    X(STR_IDEA_COMMERCE_LIBRE_ECHANGE_BONUS, "Immunisé aux embargos — et ne peut plus en décréter") \
    X(STR_DOCT_MERCANTILISME_NAME,  "Mercantilisme") \
    X(STR_DOCT_MERCANTILISME_HOVER, "Ce qui entre passe par mon étape, paie mon droit, ou ne passe pas. Exclusive du Commerce.") \
    X(STR_IDEA_MERCANTILISME_RESERVES_NAME,  "Réserves") \
    X(STR_IDEA_MERCANTILISME_RESERVES_BONUS, "+30 % de réserves de chantier et de coussin d'État") \
    X(STR_IDEA_MERCANTILISME_REGIE_NAME,  "Régie") \
    X(STR_IDEA_MERCANTILISME_REGIE_BONUS, "Le stockeur d'État achète et revend plus tôt") \
    X(STR_IDEA_MERCANTILISME_BLOCUS_NAME,  "Blocus") \
    X(STR_IDEA_MERCANTILISME_BLOCUS_BONUS, "Mon embargo traverse les pactes et ferme mes Centres") \
    X(STR_IDEA_MERCANTILISME_ETAPE_NAME,  "Étape") \
    X(STR_IDEA_MERCANTILISME_ETAPE_BONUS, "Désigner une province-étape servie la première, hors export") \
    X(STR_IDEA_MERCANTILISME_PEAGES_NAME,  "Péages") \
    X(STR_IDEA_MERCANTILISME_PEAGES_BONUS, "Import au pair chez soi, 75 % du péage à la couronne") \
    X(STR_IDEA_MERCANTILISME_HALLES_NAME,  "Halles") \
    X(STR_IDEA_MERCANTILISME_HALLES_BONUS, "+30 % de capacité d'entrepôt, moins de pertes de stock") \
    X(STR_DOCT_PEUPLE_NAME,  "Peuple") \
    X(STR_DOCT_PEUPLE_HOVER, "L'étranger devient un bras, un métier, une tech. Accueil, intégration, métissage.") \
    X(STR_IDEA_PEUPLE_ACCUEIL_NAME,  "Accueil") \
    X(STR_IDEA_PEUPLE_ACCUEIL_BONUS, "Pactes migratoires acceptés plus facilement") \
    X(STR_IDEA_PEUPLE_ECOLES_NAME,  "Écoles") \
    X(STR_IDEA_PEUPLE_ECOLES_BONUS, "+6 %/mois d'intégration") \
    X(STR_IDEA_PEUPLE_ASILE_NAME,  "Asile") \
    X(STR_IDEA_PEUPLE_ASILE_BONUS, "Les réfugiés se fixent plus tôt et repartent moins") \
    X(STR_IDEA_PEUPLE_AFFRANCHISSEMENT_NAME,  "Affranchissement") \
    X(STR_IDEA_PEUPLE_AFFRANCHISSEMENT_BONUS, "Sous pacte, les déportés deviennent migrants") \
    X(STR_IDEA_PEUPLE_TOLERANCE_NAME,  "Tolérance") \
    X(STR_IDEA_PEUPLE_TOLERANCE_BONUS, "−25 % de friction des cultures étrangères") \
    X(STR_IDEA_PEUPLE_METISSAGE_NAME,  "Métissage") \
    X(STR_IDEA_PEUPLE_METISSAGE_BONUS, "Héritages étrangers accessibles plus tôt, +20 % de recherche du creuset") \
    X(STR_DOCT_COLONISATION_NAME,  "Colonisation") \
    X(STR_DOCT_COLONISATION_HOVER, "On part de plus petit, on tient sur la terre rude. Colons, vivres, climats.") \
    X(STR_IDEA_COLONISATION_COLONS_NAME,  "Colons") \
    X(STR_IDEA_COLONISATION_COLONS_BONUS, "−15 % de population requise par colonie") \
    X(STR_IDEA_COLONISATION_RAVITAILLEMENT_NAME,  "Ravitaillement") \
    X(STR_IDEA_COLONISATION_RAVITAILLEMENT_BONUS, "−25 % de réserve vivrière exigée") \
    X(STR_IDEA_COLONISATION_ACCLIMATATION_NAME,  "Acclimatation") \
    X(STR_IDEA_COLONISATION_ACCLIMATATION_BONUS, "−20 % de malus de terre rude") \
    X(STR_IDEA_COLONISATION_DOUBLE_CHANTIER_NAME,  "Double chantier") \
    X(STR_IDEA_COLONISATION_DOUBLE_CHANTIER_BONUS, "2 chantiers coloniaux simultanés") \
    X(STR_IDEA_COLONISATION_CLIMATS_NAME,  "Climats") \
    X(STR_IDEA_COLONISATION_CLIMATS_BONUS, "Apprentissage des climats 10 % plus tôt") \
    X(STR_IDEA_COLONISATION_GRAND_LARGE_NAME,  "Grand large") \
    X(STR_IDEA_COLONISATION_GRAND_LARGE_BONUS, "+50 % de rendement des colonies lointaines") \
    X(STR_DOCT_DIPLOMATIE_NAME,  "Diplomatie") \
    X(STR_DOCT_DIPLOMATIE_HOVER, "Parler plus souvent, plus vite, à deux voix. Opinion, émissaires, congrès.") \
    X(STR_IDEA_DIPLOMATIE_PRESTIGE_NAME,  "Prestige") \
    X(STR_IDEA_DIPLOMATIE_PRESTIGE_BONUS, "+25 % d'opinion des alliés et partenaires") \
    X(STR_IDEA_DIPLOMATIE_CHANCELLERIE_NAME,  "Chancellerie") \
    X(STR_IDEA_DIPLOMATIE_CHANCELLERIE_BONUS, "−20 % de coût d'influence des émissaires") \
    X(STR_IDEA_DIPLOMATIE_OUBLI_NAME,  "Oubli") \
    X(STR_IDEA_DIPLOMATIE_OUBLI_BONUS, "Vos griefs s'effacent 30 % plus vite") \
    X(STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_NAME,  "Second émissaire") \
    X(STR_IDEA_DIPLOMATIE_SECOND_EMISSAIRE_BONUS, "2 actions diplomatiques simultanées") \
    X(STR_IDEA_DIPLOMATIE_PERSUASION_NAME,  "Persuasion") \
    X(STR_IDEA_DIPLOMATIE_PERSUASION_BONUS, "Vos offres acceptées plus facilement") \
    X(STR_IDEA_DIPLOMATIE_CONGRES_NAME,  "Congrès") \
    X(STR_IDEA_DIPLOMATIE_CONGRES_BONUS, "Les guerres se concluent plus tôt, les vôtres aussi") \
    X(STR_DOCT_VASSAUX_NAME,  "Vassaux") \
    X(STR_DOCT_VASSAUX_HOVER, "Faire jurer, faire payer, faire mûrir. Serments, tribut, annexion.") \
    X(STR_IDEA_VASSAUX_SERMENTS_NAME,  "Serments") \
    X(STR_IDEA_VASSAUX_SERMENTS_BONUS, "Vassaux intégrés 15 % plus vite") \
    X(STR_IDEA_VASSAUX_TRIBUT_VASSAL_NAME,  "Tribut vassal") \
    X(STR_IDEA_VASSAUX_TRIBUT_VASSAL_BONUS, "Contribution plus tôt et +20 %") \
    X(STR_IDEA_VASSAUX_CONTRATS_NAME,  "Contrats") \
    X(STR_IDEA_VASSAUX_CONTRATS_BONUS, "Choisir le contrat de vassalité à la paix") \
    X(STR_IDEA_VASSAUX_LEVIERS_NAME,  "Leviers") \
    X(STR_IDEA_VASSAUX_LEVIERS_BONUS, "Don, allègement, division, intimidation des vassaux") \
    X(STR_IDEA_VASSAUX_ANNEXION_NAME,  "Annexion") \
    X(STR_IDEA_VASSAUX_ANNEXION_BONUS, "Peut annexer ses vassaux, durée −25 %") \
    X(STR_IDEA_VASSAUX_SUZERAINETE_NAME,  "Suzeraineté") \
    X(STR_IDEA_VASSAUX_SUZERAINETE_BONUS, "Proposer la vassalité en pleine paix") \
    X(STR_DOCT_PRODUCTION_NAME,  "Production") \
    X(STR_DOCT_PRODUCTION_HOVER, "Creuser plus profond, équiper plus de bras. Outillage, manufactures, paliers.") \
    X(STR_IDEA_PRODUCTION_EXTRACTION_NAME,  "Extraction") \
    X(STR_IDEA_PRODUCTION_EXTRACTION_BONUS, "+12 % de bras à l'extraction") \
    X(STR_IDEA_PRODUCTION_OUTILLAGE_NAME,  "Outillage") \
    X(STR_IDEA_PRODUCTION_OUTILLAGE_BONUS, "+30 % d'outils par ouvrier") \
    X(STR_IDEA_PRODUCTION_EXPLOITATION_NAME,  "Exploitation profonde") \
    X(STR_IDEA_PRODUCTION_EXPLOITATION_BONUS, "Paliers d'exploitation jusqu'à 12 au lieu de 8") \
    X(STR_IDEA_PRODUCTION_MANUFACTURES_NAME,  "Manufactures") \
    X(STR_IDEA_PRODUCTION_MANUFACTURES_BONUS, "+15 % de capacité des manufactures") \
    X(STR_IDEA_PRODUCTION_GAGES_NAME,  "Gages") \
    X(STR_IDEA_PRODUCTION_GAGES_BONUS, "−15 % de coût des manufactures, −20 % de gages d'État") \
    X(STR_IDEA_PRODUCTION_RENDEMENT_NAME,  "Rendement") \
    X(STR_IDEA_PRODUCTION_RENDEMENT_BONUS, "+6 % d'extraction par palier, paliers −25 %") \
    X(STR_DOCT_INFRASTRUCTURE_NAME,  "Infrastructure") \
    X(STR_DOCT_INFRASTRUCTURE_HOVER, "La pierre posée ne redevient jamais une ruine. Chantiers moins chers, bâti qui dure.") \
    X(STR_IDEA_INFRASTRUCTURE_MACONS_NAME,  "Maçons") \
    X(STR_IDEA_INFRASTRUCTURE_MACONS_BONUS, "−10 % de matière par chantier") \
    X(STR_IDEA_INFRASTRUCTURE_CARRIERES_NAME,  "Carrières") \
    X(STR_IDEA_INFRASTRUCTURE_CARRIERES_BONUS, "+30 % de réserves de construction") \
    X(STR_IDEA_INFRASTRUCTURE_ENTRETIEN_NAME,  "Entretien") \
    X(STR_IDEA_INFRASTRUCTURE_ENTRETIEN_BONUS, "Usure du bâti −25 %") \
    X(STR_IDEA_INFRASTRUCTURE_RENOVATION_NAME,  "Rénovation de masse") \
    X(STR_IDEA_INFRASTRUCTURE_RENOVATION_BONUS, "File nationale de rénovation, coût −20 %") \
    X(STR_IDEA_INFRASTRUCTURE_LOGEMENTS_NAME,  "Logements") \
    X(STR_IDEA_INFRASTRUCTURE_LOGEMENTS_BONUS, "+25 % de logements par manufacture") \
    X(STR_IDEA_INFRASTRUCTURE_INTENDANCE_NAME,  "Intendance") \
    X(STR_IDEA_INFRASTRUCTURE_INTENDANCE_BONUS, "−30 % de surcoût d'étendue") \
    X(STR_DOCT_TECHNOLOGIE_NAME,  "Technologie") \
    X(STR_DOCT_TECHNOLOGIE_HOVER, "La recherche devient une politique. Bibliothèques, écoles, copistes.") \
    X(STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_NAME,  "Bibliothèques") \
    X(STR_IDEA_TECHNOLOGIE_BIBLIOTHEQUES_BONUS, "+25 % de bonus de la chaîne Bibliothèque") \
    X(STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_NAME,  "Écoles de ville") \
    X(STR_IDEA_TECHNOLOGIE_ECOLES_VILLE_BONUS, "+30 % de recherche bourgeoise, +25 % ouvrière") \
    X(STR_IDEA_TECHNOLOGIE_PROGRAMME_NAME,  "Programme") \
    X(STR_IDEA_TECHNOLOGIE_PROGRAMME_BONUS, "Orienter la recherche : −20 % sur un quartier choisi") \
    X(STR_IDEA_TECHNOLOGIE_COPISTES_NAME,  "Copistes") \
    X(STR_IDEA_TECHNOLOGIE_COPISTES_BONUS, "Techs répandues jusqu'à −52 %") \
    X(STR_IDEA_TECHNOLOGIE_DISPENSE_NAME,  "Dispense") \
    X(STR_IDEA_TECHNOLOGIE_DISPENSE_BONUS, "2 paliers d'âge en avance, hors nœuds faustiens") \
    X(STR_IDEA_TECHNOLOGIE_SOBRIETE_NAME,  "Sobriété") \
    X(STR_IDEA_TECHNOLOGIE_SOBRIETE_BONUS, "−10 % techs propres, +50 % techs faustiennes") \
    X(STR_DOCT_CONNAISSANCES_NAME,  "Connaissances du monde") \
    X(STR_DOCT_CONNAISSANCES_HOVER, "Connaître le monde avant de le prendre. Côtes révélées, contacts, expéditions.") \
    X(STR_IDEA_CONNAISSANCES_PORTULANS_NAME,  "Portulans") \
    X(STR_IDEA_CONNAISSANCES_PORTULANS_BONUS, "×2 de côtes révélées autour du connu") \
    X(STR_IDEA_CONNAISSANCES_TRUCHEMENTS_NAME,  "Truchements") \
    X(STR_IDEA_CONNAISSANCES_TRUCHEMENTS_BONUS, "Contacts culturels 20 % plus vite") \
    X(STR_IDEA_CONNAISSANCES_EXPEDITION_NAME,  "Expédition") \
    X(STR_IDEA_CONNAISSANCES_EXPEDITION_BONUS, "Révéler une contrée lointaine et ouvrir un contact") \
    X(STR_IDEA_CONNAISSANCES_DICTIONNAIRES_NAME,  "Dictionnaires") \
    X(STR_IDEA_CONNAISSANCES_DICTIONNAIRES_BONUS, "Héritages étrangers accessibles plus tôt") \
    X(STR_IDEA_CONNAISSANCES_COLLEGES_NAME,  "Collèges des langues") \
    X(STR_IDEA_CONNAISSANCES_COLLEGES_BONUS, "+40 % de recherche des peuples digérés") \
    X(STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_NAME,  "Langue franque") \
    X(STR_IDEA_CONNAISSANCES_LANGUE_FRANQUE_BONUS, "Vos alliés et routes partagent leurs cartes") \
    X(STR_DOCT_FAUSTIEN_NAME,  "Faustien") \
    X(STR_DOCT_FAUSTIEN_HOVER, "La puissance immédiate contre la damnation lente. Transmuteurs, mutations, charge.") \
    X(STR_IDEA_FAUSTIEN_PAGES_INTERDITES_NAME,  "Pages interdites") \
    X(STR_IDEA_FAUSTIEN_PAGES_INTERDITES_BONUS, "−15 % de coût des techs faustiennes") \
    X(STR_IDEA_FAUSTIEN_CREUSETS_NAME,  "Creusets") \
    X(STR_IDEA_FAUSTIEN_CREUSETS_BONUS, "Alambic et Atelier de mage débloqués") \
    X(STR_IDEA_FAUSTIEN_PACTE_NAME,  "Le Pacte") \
    X(STR_IDEA_FAUSTIEN_PACTE_BONUS, "Les trois transmuteurs débloqués, plus aucun refus") \
    X(STR_IDEA_FAUSTIEN_OR_DU_PUITS_NAME,  "Or du puits") \
    X(STR_IDEA_FAUSTIEN_OR_DU_PUITS_BONUS, "+30 % d'or de la Foreuse — la monnaie se débase") \
    X(STR_IDEA_FAUSTIEN_TERRE_CHANGEE_NAME,  "Terre changée") \
    X(STR_IDEA_FAUSTIEN_TERRE_CHANGEE_BONUS, "+25 % de mutations, la charge se lave −35 %") \
    X(STR_IDEA_FAUSTIEN_PRIX_CONSENTI_NAME,  "Prix consenti") \
    X(STR_IDEA_FAUSTIEN_PRIX_CONSENTI_BONUS, "+25 % de sortie des machines, +50 % de charge") \
    X(STR_DOCT_ARISTOCRATIE_NAME,  "Aristocratie") \
    X(STR_DOCT_ARISTOCRATIE_HOVER, "L'influence naît des élites. Fiefs, offices, adoubement. Un seul courant à la fois.") \
    X(STR_IDEA_ARISTOCRATIE_BANNERETS_NAME,  "Bannerets") \
    X(STR_IDEA_ARISTOCRATIE_BANNERETS_BONUS, "+25 % de contribution vassale") \
    X(STR_IDEA_ARISTOCRATIE_OFFICES_NAME,  "Offices") \
    X(STR_IDEA_ARISTOCRATIE_OFFICES_BONUS, "+30 % de loyauté achetée, renvoi +50 % de grief") \
    X(STR_IDEA_ARISTOCRATIE_ADOUBEMENT_NAME,  "Adoubement") \
    X(STR_IDEA_ARISTOCRATIE_ADOUBEMENT_BONUS, "Promouvoir des bourgeois en élites") \
    X(STR_IDEA_ARISTOCRATIE_FIEFS_NAME,  "Fiefs") \
    X(STR_IDEA_ARISTOCRATIE_FIEFS_BONUS, "+35 % de charges d'élite par édifice") \
    X(STR_IDEA_ARISTOCRATIE_BAN_FEODAL_NAME,  "Ban féodal") \
    X(STR_IDEA_ARISTOCRATIE_BAN_FEODAL_BONUS, "+15 % de moral, −25 % d'impôt des élites") \
    X(STR_IDEA_ARISTOCRATIE_CLOTURE_NAME,  "Clôture") \
    X(STR_IDEA_ARISTOCRATIE_CLOTURE_BONUS, "Noblesse plus accessible, bourgeoisie plus fermée") \
    X(STR_DOCT_BOURGEOISIE_NAME,  "Bourgeoisie") \
    X(STR_DOCT_BOURGEOISIE_HOVER, "L'influence naît des bourgeois. Chartes, crédit, jurandes. Un seul courant à la fois.") \
    X(STR_IDEA_BOURGEOISIE_CHARTES_NAME,  "Chartes") \
    X(STR_IDEA_BOURGEOISIE_CHARTES_BONUS, "−15 % de coût administratif") \
    X(STR_IDEA_BOURGEOISIE_JURANDES_NAME,  "Jurandes") \
    X(STR_IDEA_BOURGEOISIE_JURANDES_BONUS, "+20 % de volume marchand, accession +10 %") \
    X(STR_IDEA_BOURGEOISIE_EMPRUNT_NAME,  "Emprunt intérieur") \
    X(STR_IDEA_BOURGEOISIE_EMPRUNT_BONUS, "La bourgeoisie prête à l'État") \
    X(STR_IDEA_BOURGEOISIE_CREDIT_NAME,  "Crédit") \
    X(STR_IDEA_BOURGEOISIE_CREDIT_BONUS, "−20 % de taux d'intérêt") \
    X(STR_IDEA_BOURGEOISIE_ROBE_NAME,  "Robe") \
    X(STR_IDEA_BOURGEOISIE_ROBE_BONUS, "Un siège de Conseil supplémentaire") \
    X(STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_NAME,  "Clés de la ville") \
    X(STR_IDEA_BOURGEOISIE_CLES_DE_LA_VILLE_BONUS, "Accession bourgeoise −25 %") \
    X(STR_DOCT_POPULAIRE_NAME,  "Populaire") \
    X(STR_DOCT_POPULAIRE_HOVER, "L'influence naît des journaliers. Pain, doléances, levée en masse. Un seul courant à la fois.") \
    X(STR_IDEA_POPULAIRE_DOLEANCES_NAME,  "Doléances") \
    X(STR_IDEA_POPULAIRE_DOLEANCES_BONUS, "Politique +20 % ressentie, −15 % d'agitation") \
    X(STR_IDEA_POPULAIRE_PAIN_NAME,  "Pain") \
    X(STR_IDEA_POPULAIRE_PAIN_BONUS, "Panier vital exonéré d'impôt, provinces contentes plus fécondes") \
    X(STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_NAME,  "Levée en masse") \
    X(STR_IDEA_POPULAIRE_LEVEE_EN_MASSE_BONUS, "Conscription au-delà de la limite pendant 5 ans") \
    X(STR_IDEA_POPULAIRE_CONCESSION_NAME,  "Concession") \
    X(STR_IDEA_POPULAIRE_CONCESSION_BONUS, "Apaiser une province avant la révolte, coût −30 %") \
    X(STR_IDEA_POPULAIRE_IMPOT_DU_RANG_NAME,  "Impôt du rang") \
    X(STR_IDEA_POPULAIRE_IMPOT_DU_RANG_BONUS, "+20 % d'impôt des élites, rangs fermés") \
    X(STR_IDEA_POPULAIRE_SOUVERAINETE_NAME,  "Souveraineté") \
    X(STR_IDEA_POPULAIRE_SOUVERAINETE_BONUS, "Céder ne coûte plus ni légitimité ni capacité") \
    X(STR_DOCT_DIVIN_NAME,  "Divin") \
    X(STR_DOCT_DIVIN_HOVER, "L'influence naît de la foi bâtie. Onction, ferveur, sacerdoce. Un seul courant à la fois.") \
    X(STR_IDEA_DIVIN_ONCTION_NAME,  "Onction") \
    X(STR_IDEA_DIVIN_ONCTION_BONUS, "+25 % de légitimité par la foi") \
    X(STR_IDEA_DIVIN_FERVEUR_NAME,  "Ferveur") \
    X(STR_IDEA_DIVIN_FERVEUR_BONUS, "+20 % de ferveur, dure 5 ans de plus") \
    X(STR_IDEA_DIVIN_SACERDOCE_NAME,  "Sacerdoce") \
    X(STR_IDEA_DIVIN_SACERDOCE_BONUS, "Missionnaire pour tous les crédos, fondation hors plafond") \
    X(STR_IDEA_DIVIN_APPEL_NAME,  "Appel à la foi") \
    X(STR_IDEA_DIVIN_APPEL_BONUS, "Mobiliser la ferveur : concorde ou zélotes") \
    X(STR_IDEA_DIVIN_CLERGE_NAME,  "Clergé") \
    X(STR_IDEA_DIVIN_CLERGE_BONUS, "2 lettrés, missions +40 % plus longues") \
    X(STR_IDEA_DIVIN_ORTHODOXIE_NAME,  "Orthodoxie") \
    X(STR_IDEA_DIVIN_ORTHODOXIE_BONUS, "Schismes ÷2, minorités +60 % de grogne") \
    /* LES DOCTRINES — les MOTS de la membrane (refus d'adoption, ligne de dépenses). */ \
    X(STR_DOCT_REASON_SLOT,      "Aucun emplacement libre") \
    X(STR_DOCT_REASON_ALREADY,   "Déjà adoptée") \
    X(STR_DOCT_REASON_PAIR,      "Commerce ou Mercantilisme, jamais les deux") \
    X(STR_DOCT_REASON_CURRENT,   "Un seul courant politique à la fois") \
    X(STR_DOCT_REASON_INFLUENCE, "Influence insuffisante")     X(STR_DOCT_REASON_FAITH,     "Aucune religion fondée") \
    X(STR_INFLUENCE_DEPENSES,    "Entretien de %d doctrine(s) : %d par mois") \
    X(STR_INFLUENCE_DEPENSES_0,  "Aucune doctrine à entretenir") \
    /* GRAND LIVRE (W2-7) — la ligne de RECOUPEMENT : ce que le trésor a bougé sans
     * passer par un poste nommé (achats d'État, marché, saisies). Sans elle le
     * panneau Trésor affichait « 0/mois » partout pendant que l'or fondait. */ \
    X(STR_FLUX_AUTRES,           "Autres mouvements") \
    /* MARCHÉ — les MOTS des noms de repli (jamais un identifiant moteur face joueur). */ \
    X(STR_PROV_SANS_NOM,         "Province sans nom") \
    X(STR_MANUF_SANS_NOM,        "Manufacture sans nom") \
    /* LES MOTS DU MENU CONSTRUCTION (UI-1, retour joueur 2026-09-04 : « on voit
     * +1 prospérité sur le port sans savoir ce que prospérité veut dire »). Même
     * convention que le glossaire §hover_* existant (STR_GLOSS_* le TITRE, STR_HOVER_*
     * LA PHRASE) : ces paires REJOIGNENT la table `G_GLOSSARY` (scps_lang.c) plutôt que
     * d'ouvrir un second registre — Prospérité et Savoir y étaient DÉJÀ, seuls les neuf
     * termes ci-dessous manquaient. `api_edifice_effet` compose sa ligne d'effet avec
     * CES MÊMES STR_GLOSS_*, donc un terme affiché ne peut pas manquer sa définition. */ \
    X(STR_GLOSS_CAPADM,      "Capacité administrative") \
    X(STR_HOVER_CAPADM,      "Ce que l'État tient vraiment dans la province : institutions et garnisons bâties. Elle tient l'ordre, porte les services rendus et l'impôt réellement levé — et s'use avec le temps.") \
    X(STR_GLOSS_LOGEMENT,    "Logement") \
    X(STR_HOVER_LOGEMENT,    "Ce que le bâti peut nourrir et abriter : la population croît vers ce plafond et s'y arrête.") \
    X(STR_GLOSS_SERVICE,     "Service") \
    X(STR_HOVER_SERVICE,     "Ce qui apaise la province (culte, aménités) : l'agitation retombe et la loyauté tient.") \
    X(STR_GLOSS_PORT,        "Port") \
    X(STR_HOVER_PORT,        "La rade de la province : sans port, ni route de mer, ni flotte, ni commerce maritime.") \
    X(STR_GLOSS_STRUCTUREL,  "Structurel") \
    X(STR_HOVER_STRUCTUREL,  "Aucun effet chiffré propre : cet édifice ouvre un palier ou une famille, pas une métrique.") \
    X(STR_GLOSS_ENTRETIEN,   "Entretien") \
    X(STR_HOVER_ENTRETIEN,   "Le coût récurrent du bâti, en couronnes par mois. Impayé, la province tombe en friche et sa production chute.") \
    X(STR_GLOSS_RECETTE,     "Recette") \
    X(STR_HOVER_RECETTE,     "Les matières que la manufacture consomme chaque mois, et le bien qu'elle rend en échange.") \
    X(STR_GLOSS_PALIER,      "Palier") \
    X(STR_HOVER_PALIER,      "Le rang suivant d'un édifice : chaque palier suit le tier du bourg et ouvre de nouveaux effets.") \
    X(STR_GLOSS_OR,          "Or") \
    X(STR_HOVER_OR,          "Le prix du chantier, payé une seule fois au trésor national — la recette de matières, elle, se prend sur les stocks.") \
    X(STR_EFFET_STRUCTUREL,  "Structurel (voir sa famille)")

#endif /* SCPS_STRINGS_IDS_H */
