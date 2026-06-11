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
    X(STR_HOVER_PROSP, "La richesse qui circule et qu'on parvient à lever ; un royaume opulent rayonne, une disette le vide.") \
    X(STR_HOVER_SAVOIR, "Le savoir né aux carrefours des cultures ; il nourrit les arts et les arcanes.") \
    X(STR_HOVER_PRESAGE, "Ce que la quête de puissance attire ; plus on force l'arcane, plus l'ombre s'épaissit.") \
    X(STR_HOVER_STATURE, "L'ampleur de l'établissement humain, du hameau perdu à la cité grouillante.") \
    X(STR_HOVER_FLUX, "Le mouvement des âmes : un afflux gonfle la province, un exode la vide.") \
    X(STR_HOVER_AISANCE, "La richesse qui circule ici ; les carrefours prospèrent, les culs-de-sac s'étiolent.") \
    X(STR_HOVER_CARREFOUR, "Quand des cultures se croisent ici, la richesse afflue — jusqu'à ce que le flux déborde et que la ville-monde se déchire.") \
    X(STR_HOVER_HUMEUR, "Le cœur de la province envers la couronne ; loyale, elle paie sans broncher — frondeuse, elle attend l'étincelle.") \
    X(STR_HOVER_LIGNEE, "Ce qui la lie à la culture du trône ; le même sang se gouverne aisément, l'inassimilable jamais sans la force.") \
    X(STR_HOVER_AGITATION, "La colère qui monte dans la province ; soutenue, elle vire à la révolte — qu'apaisent la stabilité du royaume, la garnison et la légitimité.") \
    X(STR_HOVER_FOI, "La ferveur de la province envers le culte du trône ; dévote, elle nourrit la légitimité sacrée — hérétique, elle couve le schisme.") \
    X(STR_HOVER_SEDITION, "La tension d'une faction forte dont les valeurs s'opposent à la direction du régime ; séditieuse, elle complote le coup d'État pour imposer son éthos.") \
    X(STR_TUTO_TITLE_0, "1 · Ce monde se lit.") \
    X(STR_TUTO_TITLE_1, "2 · Le temps coule en jours.") \
    X(STR_TUTO_TITLE_2, "3 · Ton empire.") \
    X(STR_TUTO_TITLE_3, "4 · Décider coûte.") \
    X(STR_TUTO_TITLE_4, "5 · Les autres.") \
    X(STR_TUTO_TITLE_5, "6 · Le savoir voyage.") \
    X(STR_TUTO_TITLE_6, "7 · La Brèche.") \
    X(STR_TUTO_PAGE_0, "Ici, pas de pourcentages cachés : l'état des choses se dit en MOTS.\nUne province est Unie ou Fracturée, un peuple Loyal ou Frondeur,\nun marché Sain ou En pénurie. Survole : tout se définit en bas d'écran.") \
    X(STR_TUTO_PAGE_1, "En haut à droite : la date, l'âge du monde, la vitesse.\nESPACE met en pause — et en pause, tu peux tout consulter,\ntout ordonner. Rien ne presse jamais que toi.") \
    X(STR_TUTO_PAGE_2, "En haut : ton or, tes vivres, tes matériaux, et la santé de ta couronne —\nStabilité, Légitimité, Cohésion, Prospérité. Clique une province pour la voir\nde près ; ouvre la BARRE DE GAUCHE pour l'empire entier :\nÉconomie, Démographie, Stocks, Armée, Filtres.") \
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
    X(STR_DIPLO_RANCUNE, "rancune vive") \
    X(STR_DIPLO_MOTIF_FMT, "motif : {0}") \
    X(STR_DIPLO_RACE_FMT, "Race : {0}") \
    X(STR_DIPLO_STATUT_FMT, "Statut : {0}") \
    X(STR_DIPLO_MENACE_FMT, "Menace : {0}%") \
    X(STR_DIPLO_ACTIONS, "Actions — onglet Diplomatie (G)") \
    X(STR_CB_TERRITORIAL, "frontière revendiquée") \
    X(STR_CB_RELIGIOUS, "schisme religieux") \
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
    X(STR_FACTION_ETHOS_3, "la voie de la tradition — terre, foi, continuité") \
    X(STR_FACTION_ETHOS_4, "la voie de l'interdit — arcane, risque, tabou franchi") \
    X(STR_FACTION_ETHOS_5, "le petit peuple — pain, paix, sécurité du quotidien") \
    X(STR_FACTION_HOV_FMT, "{0} · {1}. Satisfaction {2}% = leur adhésion au régime ; part {3}% = leur poids politique.") \
    X(STR_FACTION_HOV_COUP, " — ALIÉNÉE & PUISSANTE : le coup couve.") \
    X(STR_CENTRE_RESEAU_OUVERT, "Réseau inter-pays OUVERT (un Centre commercial tenu)") \
    X(STR_CENTRE_RESEAU_FERME, "Réseau inter-pays FERMÉ — aucun Centre commercial (en conquérir un)") \
    X(STR_CENTRE_COMMERCIAL, "Centre commercial — hub du réseau inter-régional (le tenir = commercer)") \
    X(STR_PAN_MARCHE, "MARCHÉ") \
    X(STR_MARCHE_PRIX_FMT, "prix courant {0} or l'unité") \
    X(STR_MARCHE_PRIX_HOV, "Le prix du marché intérieur : la demande le tire, l'offre et la vente le détendent. Acheter et vendre se font à CE prix.") \
    X(STR_MARCHE_ROW_HOV, "{0} — en réserve {1}. Acheter ou vendre par lots de 10 au prix courant (l'or sort et entre par le trésor).") \
    X(STR_SLOT_VERROU_FMT, "{0} — verrouillé ({1})") \
    X(STR_BTN_COMPTOIR_FMT, "Bâtir un Comptoir ici  ({0} or)") \
    X(STR_COMPTOIR_HOV, "Le Comptoir branche la province au Centre commercial le plus proche : la marge de transport tombe d'un tiers à ce bout des routes marchandes.") \
    X(STR_ENTREPOT_CAP_FMT, "stock {0}/{1} — Entrepôts ×{2}") \
    X(STR_ROW_ENTREPOTS, "Entrepôts") \
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

#endif /* SCPS_STRINGS_IDS_H */
