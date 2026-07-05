/*
 * scps_tech.c — arbre concentrique & fractal (voir scps_tech.h)
 *
 * Table de nœuds data-driven : 9 quartiers (3 thèmes × 3 fonctions), rayon = tier.
 * Aucune dépendance au reste du moteur : ce module n'écrit QUE dans un TechState
 * et répond des LECTURES (coût, accès, coordonnées). Le branchement IA/sim/UI se
 * fait par l'appelant (qui paie le coût, fournit le masque de héritages, lit l'arbre).
 */
#include "scps_tech.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>     /* MODTOOLS : dump/load fichier */
#include <string.h>
#include <stdlib.h>

#define NONE TECH_COUNT     /* sentinelle « pas de prérequis » */
#define UNIV HERITAGE_COUNT     /* sentinelle « tech universelle (pas de heritage native) » */

/* ---- Constantes de calibrage (surface d'équilibrage) ------------------ */
#define CRISIS_SCALE    12.0f   /* échelle de la courbe proximité = f(charge) */
#define SHOCK_MAGIE      0.50f   /* l'arcane gonfle l'ampleur du choc */
#define DEREAL_P_COEF    0.10f   /* terme (P/10)·C */
/* COÛT ∝ N^exp — DÉCOUPLÉ DE LA POP (2026-06-28). Le revenu de recherche monte avec la POP
 * (∝ N, plus de monde = plus de chercheurs) ; en scalant le COÛT sur √N (SOUS-linéaire), le
 * coût MARGINAL d'une province reste INFÉRIEUR à son apport de recherche → l'EXPANSION (wide)
 * est RÉCOMPENSÉE, mais sous-linéairement (le coût croît quand même → frein au snowball ;
 * rythme/empire ∝ N/√N = √N). Calé près des GRANDS empires (où vit l'essentiel de la pop)
 * pour borner le re-baseline ; plancher = un empire mono-province ne paie jamais ~rien.
 * COST_SCALE relève l'ensemble : un empire ne s'offre que ~40-60 % de l'arbre → il SPÉCIALISE. */
#define COST_SCALE        14.4f  /* P5.29 : coût des techs ×3 — l'income suit (IA ×3, joueur par tier) */
#define TECH_COST_N_K     0.90f  /* coefficient du coût ∝ N^exp (calé sur les grands empires : N~20 ≈ ancien popf) */
#define TECH_COST_N_EXP   0.5f   /* exposant SOUS-linéaire (0.5 = √N) — le cœur du « wide récompensé » */
#define TECH_COST_N_FLOOR 0.5f   /* plancher : un empire mono-province paie au moins BASE×SCALE×0.5 */
static float BASE_COST[6] = { 0.f, 40.f, 90.f, 160.f, 260.f, 400.f }; /* par tier — NON-const (MODTOOLS) */

/* ====================================================================== */
/* TABLE DES NŒUDS — 9 quartiers (angle), tier (rayon)                     */
/* Champs : name, unlocks, theme, func, tier, prereq, faustian, needs_ruins,
 *          native, dK,dL,dF, dEco,dMil, dH, dFracture, dPuissance, flux,
 *          charge, triggers_crisis                                        */
/* ====================================================================== */
static const TechNode NODES[TECH_COUNT] = {
/* ---- SAVOIR · PRODUCTION (spine sûre : vitesse de recherche, +K) ------ */
[TECH_BIBLIOTHEQUE] = { "Bibliothèque","Bibliothèque", THM_SAVOIR,FN_PRODUCTION,0, NONE, false,false,UNIV,
    1,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+recherche (le socle du savoir) · +1 capacité narrative (K)",
    "Un empire commence toujours par une pièce pleine de livres que personne n'a le temps de lire." },
[TECH_SCRIPTORIUM] = { "Scriptorium","Scriptorium", THM_SAVOIR,FN_PRODUCTION,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    1,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+recherche (×0.5 le rendement) · +1 capacité narrative (K)",
    "On a embauché des scribes pour recopier les livres que personne ne lisait déjà." },
[TECH_ACADEMIE] = { "Académie","Académie", THM_SAVOIR,FN_PRODUCTION,2, TECH_SCRIPTORIUM, false,false,UNIV,
    2,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+recherche (vitesse accrue) · +2 capacité narrative (K)",
    "Désormais les érudits débattent entre eux plutôt qu'avec le peuple — un progrès net." },
[TECH_UNIVERSITE] = { "Université","Université", THM_SAVOIR,FN_PRODUCTION,3, TECH_ACADEMIE, false,false,UNIV,
    3,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+recherche (vitesse maximale) · +3 capacité narrative (K)",
    "On y décerne des diplômes ; personne ne sait encore à quoi ils serviront." },
/* ---- SAVOIR · ARMÉE (arcane offensif — faustien) --------------------- */
[TECH_SAVOIR_GUERRE] = { "Savoir de guerre","Collège de guerre", THM_SAVOIR,FN_ARMEE,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0,0, 0,1.0f, 0, 0, 0, 0.05f, 0.3f, false,
    "+puissance arcane des armées (léger flux vers la Brèche)",
    "Le collège enseigne comment tuer avec méthode — la méthode est le vrai progrès." },
[TECH_MAGIE_BATAILLE] = { "Magie de bataille","Tour de mages", THM_SAVOIR,FN_ARMEE,2, TECH_SAVOIR_GUERRE, false,false,UNIV,
    0,0,0, 0,2.0f, 0, 0, 1.0f, 0.50f, 1.5f, false,
    "+puissance arcane des armées · +puissance brute (charge faustienne modérée)",
    "Les mages de combat promettent la victoire ; ils omettent de préciser pour qui." },
[TECH_INVOCATION] = { "Invocation","Cercle d'invocation", THM_SAVOIR,FN_ARMEE,3, TECH_MAGIE_BATAILLE, true,true,HERITAGE_ESOTERIQUE,
    0,0,0, 0,2.0f, 0, 0, 3.0f, 1.50f, 3.0f, false,
    "⚠ armée invoquée, sans coût de population · forte charge faustienne — rapproche la Brèche",
    "On convoque des soldats depuis nulle part ; nulle part se souvient toujours du prix." },
[TECH_EVEIL] = { "L'Éveil","Le Réveil (armée sans pop)", THM_SAVOIR,FN_ARMEE,4, TECH_MAGIE_BATAILLE, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 6.0f, 3.00f, 6.0f, true,
    "⚠ armée invoquée massive · DÉCLENCHE la crise de fin (charge maximale)",
    "Réveiller ce qui dort est toujours une bonne idée, jusqu'au réveil." },
/* ---- SAVOIR · RENFORCEMENT (arcane durable — faustien) --------------- */
[TECH_WARDS] = { "Gardes runiques","Gardes runiques (Wards)", THM_SAVOIR,FN_RENFORCEMENT,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0.5f,1.0f, 0,0, 0, 0, 0, 0, 0.3f, false,
    "+légitimité & +fédéralisme (protections runiques)",
    "Des runes protègent la capitale ; les provinces se contentent de la promesse qu'elles existent." },
[TECH_SCRYING] = { "Scrying","Bassin de scrying", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_WARDS, false,false,UNIV,
    0,0,0.5f, 0,0, 0, 0, 0.5f, 0.30f, 1.0f, false,
    "+fédéralisme (vision lointaine) · léger flux vers la Brèche",
    "Voir loin ne rend pas plus sage — mais donne l'air d'être informé." },
[TECH_COMMUNION] = { "Communion","Bosquet de communion", THM_SAVOIR,FN_RENFORCEMENT,3, TECH_SCRYING, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,2.0f, 0,0, 0, -1.0f, 0.5f, 0.10f, 0.5f, false,
    "+légitimité & +fédéralisme · −fracture interne (l'harmonie des peuples)",
    "On appelle « communion » le moment où plus personne n'a la force de se disputer." },
[TECH_SAVOIR_INTERDIT] = { "Savoir interdit","Crypte interdite", THM_SAVOIR,FN_RENFORCEMENT,4, TECH_SCRYING, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 4.0f, 2.00f, 4.0f, false,
    "⚠ grand pouvoir arcane · charge faustienne élevée — rapproche la Brèche",
    "C'était scellé pour une raison ; la raison, comme d'habitude, a perdu contre la curiosité." },

/* ---- FORGE · PRODUCTION (sortie — le multiplicateur de rendement) ----- */
[TECH_COLLECTE_BOIS] = { "Collecte de bois","Camp de bûcherons", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production de bois (+puissance économique)",
    "La première industrie de toute civilisation consiste à abattre ce qui poussait déjà." },
[TECH_COLLECTE_ARGILE] = { "Collecte d'argile","Carrière d'argile", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production d'argile (+puissance économique)",
    "On creuse un trou pour en boucher un autre, plus loin, plus tard, plus cher." },
[TECH_FONDERIE] = { "Fonderie","Fonderie", THM_FORGE,FN_PRODUCTION,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 1.5f,0, 0, 0, 0, 0.05f, 0.3f, false,
    "permet la production de métal (fer, acier) · +production (léger flux)",
    "Le feu qui forge le fer forge aussi, accessoirement, la première arme du voisin." },
[TECH_OUTILLAGE] = { "Outillage","Atelier d'outillage", THM_FORGE,FN_PRODUCTION,2, TECH_FONDERIE, false,false,UNIV,
    0,0,0, 2.5f,0, 0, 0, 0, 0.05f, 0.3f, false,
    "+production (le multiplicateur de rendement)",
    "Un bon outil permet de faire deux fois plus de choses inutiles en deux fois moins de temps." },
[TECH_MANUFACTURE] = { "Manufacture","Manufacture", THM_FORGE,FN_PRODUCTION,3, TECH_OUTILLAGE, false,false,UNIV,
    0,0,0, 3.0f,0, 0, 1.0f, 0, 0.30f, 1.0f, false,
    "+production (+8 à 15 % selon la table) · +puissance brute · légère fracture",
    "La manufacture regroupe cent artisans sous un seul toit pour qu'aucun ne soit plus fier de son travail." },
[TECH_INDUSTRIE] = { "Industrie de masse","Complexe industriel", THM_FORGE,FN_PRODUCTION,4, TECH_MANUFACTURE, false,false,UNIV,
    0,0,0, 4.0f,2.0f, 0, 1.5f, 0, 1.00f, 3.0f, false,
    "+production de masse & +armée · fracture notable · charge faustienne significative",
    "On l'appelle complexe industriel ; le complexe, c'est surtout celui des voisins qui n'en ont pas." },
/* §B2 — FOREUSE ARCANIQUE (faustien) : transmute l'essence en FER en masse → l'issue à la
 * famine de matière pour l'empire enclavé/affamé — mais forte CHARGE + flux vers la Brèche. */
[TECH_FOREUSE] = { "Foreuse arcanique","Foreuse arcanique", THM_FORGE,FN_PRODUCTION,4, TECH_INDUSTRIE, true,false,UNIV,
    0,0,0, 3.0f,0, 0, 0, 1.0f, 1.50f, 4.0f, false,
    "⚠ transmute l'essence en fer en masse · forte charge — rapproche la Brèche",
    "Faute de mines, on fore la réalité elle-même — un raccourci qui se paie toujours en intérêts." },
/* ---- FORGE · ARMÉE (armes — faustien) -------------------------------- */
[TECH_ARMURERIE] = { "Armurerie","Armurerie", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0.3f, false,
    "permet la production d'armes · +dégâts (doctrine, léger — premier échelon de l'armement)",
    "Fabriquer des armes est un métier noble ; les vendre en est un autre, plus lucratif." },
[TECH_POUDRIERE] = { "Poudrière","Poudrière", THM_FORGE,FN_ARMEE,2, TECH_ARMURERIE, false,false,UNIV,
    0,0,0, 0,2.5f, 0, 0, 0, 0.20f, 1.0f, false,
    "permet la production de poudre à canon · +dégâts (doctrine)",
    "On a trouvé comment faire exploser les choses à distance — la diplomatie ne s'en remettra jamais." },
[TECH_FORGE_RUNES] = { "Forge à runes","Forge céleste", THM_FORGE,FN_ARMEE,3, TECH_POUDRIERE, true,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,3.0f, 0, 0, 3.0f, 1.00f, 2.0f, false,
    "⚠ permet les armes enchantées · +dégâts (doctrine) — rapproche la Brèche",
    "Graver des runes sur une épée ne la rend pas plus tranchante ; elle fait juste plus peur au client." },
[TECH_OEUVRE_NOIRE] = { "L'Œuvre noire","L'Œuvre noire", THM_FORGE,FN_ARMEE,4, TECH_POUDRIERE, true,false,UNIV,
    0,0,0, 2.0f,5.0f, 3.0f, 2.0f, 2.0f, 1.50f, 5.0f, false,
    "⚠ +armée & +coercition · fracture et charge lourdes — rapproche la Brèche",
    "On ne nomme jamais « l'Œuvre noire » un projet dont on est fier." },
/* ---- FORGE · RENFORCEMENT (durabilité / fortification) ---------------- */
[TECH_ATELIER] = { "Atelier de construction","Atelier de construction", THM_FORGE,FN_RENFORCEMENT,0, NONE, false,false,UNIV,
    0,0,0.5f, 0,0, 0, 0, 0, 0, 0, false,
    "permet de bâtir (chantiers de construction) · +fédéralisme",
    "Le premier bâtiment qu'un empire construit sert à en construire d'autres — la bureaucratie a déjà gagné." },
[TECH_QUALITE_MATERIAUX] = { "Qualité des matériaux","Chantier (béton→marbre)", THM_FORGE,FN_RENFORCEMENT,1, TECH_ATELIER, false,false,UNIV,
    0,0,1.0f, 0.5f,0, 0, 0, 0, 0, 0, false,
    "+durabilité des bâtiments (béton → marbre) · +fédéralisme & production",
    "Le marbre ne dure pas plus longtemps que le béton — il coûte juste plus cher d'avoir l'air éternel." },
[TECH_FORTIFICATIONS] = { "Fortifications","Forteresse → Citadelle", THM_FORGE,FN_RENFORCEMENT,2, TECH_QUALITE_MATERIAUX, false,false,UNIV,
    0,0,1.5f, 0,1.0f, 0, 0, 0, 0, 0.2f, false,
    "+défense (forteresse → citadelle) · +fédéralisme & armée",
    "Une citadelle imprenable finit toujours par tomber — mais elle aura d'abord ruiné trois trésoreries." },
[TECH_AUTOMATES] = { "Automates","Grand Engrenage (Golems)", THM_FORGE,FN_RENFORCEMENT,3, TECH_FORTIFICATIONS, true,false,HERITAGE_MECANISTE,
    0,0,0, 3.0f,3.0f, 0, 0, 1.0f, 1.00f, 2.0f, false,
    "⚠ +défense & production (golems) · charge faustienne — rapproche la Brèche",
    "Le golem ne se plaint jamais de ses conditions de travail — c'est bien pour cela qu'on l'a inventé." },

/* ---- SOCIÉTÉ · PRODUCTION (croissance / commerce / impôt — sûre) ------ */
[TECH_COLLECTE_NOURRITURE] = { "Collecte de nourriture","Collecte (liée au biome)", THM_SOCIETE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production de nourriture (liée au biome)",
    "Nourrir le peuple est la première tâche de tout gouvernement — et la première qu'on rogne en cas de crise." },
[TECH_IRRIGATION] = { "Irrigation & greniers","Greniers communs", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0.5f, 1.0f,0, 0, -0.5f, 0, 0, 0, false,
    "+nourriture & +logements (greniers) · +fédéralisme · −fracture",
    "Le grenier commun appartient à tout le monde, ce qui signifie qu'il appartient surtout à qui le garde." },
[TECH_COMMERCE] = { "Commerce","Marché → Banque", THM_SOCIETE,FN_PRODUCTION,2, TECH_IRRIGATION, false,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 0, 0, 0, false,
    "+or (commerce, marché, banque) · +production",
    "Le marché fixe les prix ; la banque fixe qui a le droit de s'en plaindre." },
[TECH_CADASTRE] = { "Cadastre","Cadastre (impôt)", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,UNIV,
    0,0.5f,0, 1.5f,0, 0, 0, 0, 0, 0, false,
    "+or (l'impôt) · +légitimité",
    "Le cadastre ne sert qu'à une chose : savoir exactement combien prendre à qui, et où le trouver." },
[TECH_ABONDANCE] = { "Abondance","Grenier d'abondance", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 3.0f,0, 0, -0.5f, 0, 0, 0, false,
    "+croissance & +or (l'abondance agraire) · +légitimité · −fracture",
    "On appelle « abondance » l'année où le grenier déborde juste assez pour ne pas se poser de questions sur l'année suivante." },
/* E2 §13 — la branche MARCHANDE : le Comptoir branche la province au Centre
 * commercial (marge de transport réduite) ; les Halles ouvrent l'Entrepôt
 * (+500 de cap de stock chacun) — le jeu de marché (stocker bas, vendre haut). */
[TECH_COMPTOIRS] = { "Comptoirs marchands","Comptoir", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0, 0.8f,0, 0, 0, 0, 0, 0, false,
    "branche la province au marché mondial (marge de transport réduite) · +production",
    "Le comptoir promet de rapprocher les peuples ; il rapproche surtout leurs pièces d'or des nôtres." },
[TECH_HALLES] = { "Halles & entrepôts","Entrepôt (cap de stock)", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMPTOIRS, false,false,UNIV,
    0,0,0, 1.2f,0, 0, 0, 0, 0, 0, false,
    "+500 de capacité de stock par entrepôt (stocker bas, vendre haut) · +production",
    "L'entrepôt permet d'attendre que le prix monte — la spéculation, elle, n'attend jamais qu'on l'invente." },
/* ---- SOCIÉTÉ · ARMÉE (levée — faustien : l'esclavage) ---------------- */
[TECH_CASERNE] = { "Caserne","Caserne", THM_SOCIETE,FN_ARMEE,0, NONE, false,false,UNIV,
    0,0,0, 0,0.5f, 0, 0, 0, 0, 0, false,
    "permet de recruter de l'infanterie",
    "La caserne enseigne l'ordre, la discipline et l'art de dormir à quarante dans une pièce prévue pour dix." },
[TECH_CONSCRIPTION] = { "Conscription","Levée / Conscription", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0, false,
    "+armée (la levée en masse)",
    "La conscription rend le service militaire obligatoire pour tous — sauf, curieusement, pour qui la décrète." },
[TECH_ORGANISATION] = { "Organisation militaire","État-major", THM_SOCIETE,FN_ARMEE,2, TECH_CONSCRIPTION, false,false,UNIV,
    0,0,0.5f, 0,2.0f, 0, 0, 0, 0, 0, false,
    "+armée (organisation militaire) · +fédéralisme",
    "L'état-major invente des cartes, des plans et des grades — la victoire, elle, reste à inventer sur le terrain." },
[TECH_ESCLAVAGE] = { "Économie servile","Marché aux esclaves", THM_SOCIETE,FN_ARMEE,3, TECH_ORGANISATION, true,false,HERITAGE_CLANIQUE,
    0,0,0, 3.0f,2.0f, 1.0f, 3.0f, 0, 0, 2.0f, false,
    "⚠ main-d'œuvre & armées serviles · +coercition · forte fracture interne",
    "On appelle « économie » ce qui, pour les intéressés, ressemble beaucoup plus à une prison à ciel ouvert." },
[TECH_CASTE_MARTIALE] = { "Caste martiale","Caste martiale", THM_SOCIETE,FN_ARMEE,4, TECH_ORGANISATION, true,false,UNIV,
    0,0,0, 0,4.0f, 2.0f, 2.0f, 0, 0, 2.5f, false,
    "⚠ +armée (caste guerrière) · +coercition & fracture — rapproche la Brèche",
    "Naître soldat plutôt que le devenir : la caste martiale règle d'avance la question du mérite." },
/* ---- SOCIÉTÉ · RENFORCEMENT (K / L / intégration — la spine résiliente) */
[TECH_CHANCELLERIE] = { "Chancellerie","Tribunal / Chancellerie", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    3.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+services (administration) · +3 capacité narrative (K)",
    "Le tribunal rend la justice ; la chancellerie rend surtout la paperasse, en trois exemplaires." },
[TECH_FOI] = { "Foi","Temple → Cathédrale", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_CHANCELLERIE, false,false,UNIV,
    0,3.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+légitimité & services idéologiques (temple → cathédrale)",
    "Le temple console les pauvres de leur pauvreté ; la cathédrale, elle, coûte cher aux riches pour qu'on les remarque." },
[TECH_INTEGRATION] = { "Droit d'intégration","Creuset (assimilation)", THM_SOCIETE,FN_RENFORCEMENT,3, TECH_FOI, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, -3.0f, 0, 0, 0, false,
    "+cohésion (l'assimilation des peuples) · forte baisse de fracture",
    "Le creuset fond toutes les cultures en une seule — la nôtre, comme par hasard." },
[TECH_CULTE_IMPERIAL] = { "Culte impérial","Mythe homogénéisant", THM_SOCIETE,FN_RENFORCEMENT,4, TECH_FOI, true,false,UNIV,
    1.0f,2.0f,0, 0,0, 0, -2.0f, 0, 0.50f, 3.0f, false,
    "⚠ +cohésion forcée (K & légitimité) · charge faustienne — rapproche la Brèche",
    "Un bon mythe fondateur n'a pas besoin d'être vrai ; il a juste besoin d'être répété plus fort que les autres." },
/* F3 — ALCHIMIE (gate de l'Alambic) : la distillation du salpêtre en FLUX + nécessaire
 * d'alchimiste. NON-faustienne — c'est la SUPPLY bénigne ; la charge faustienne vivra sur
 * les TRANSMUTEURS (FAU2) et leur gate dédié (FAU4). Tier 2, peu profonde (atteignable). */
[TECH_ALCHIMIE] = { "Alchimie","Alambic", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMMERCE, false,false,UNIV,
    0,0,0, 1.0f,0, 0, 0, 0, 0.10f, 0, false,
    "permet l'Alambic (distillation du salpêtre) · +production, non faustienne",
    "L'alchimiste jure qu'il ne cherche que le salpêtre — l'or viendra bien assez tôt, dit-il, à chaque siècle." },
/* FAU4 — TRANSMUTATION (FAUSTIENNE, gate du Réplicateur ligneux : flux → bois). Profonde
 * (tier 3, derrière l'Alchimie) → charge de base élevée (paroxysme = pression de Brèche). */
[TECH_TRANSMUTATION] = { "Transmutation","Réplicateur ligneux", THM_SOCIETE,FN_PRODUCTION,3, TECH_ALCHIMIE, true,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 1.0f, 0.30f, 1.2f, false,
    "⚠ permet le Réplicateur ligneux (flux → bois) · charge faustienne — rapproche la Brèche",
    "Transmuter du bois à partir de rien règle la pénurie de charpente et ouvre, discrètement, une autre pénurie." },

/* ====================================================================== */
/* ÉTOFFE (2026-06-28) — BRANCHES CULTURELLES D'HÉRITAGE (tier 1-2)        */
/* Chaque héritage gagne 2 spécialités PEU PROFONDES (native=héritage)      */
/* menant vers sa signature tier-3 — rungs que la barre de métabolisation   */
/* (Temps 2) ouvre par tier. EFFETS sur les LEVIERS VIVANTS seulement :     */
/* dK→prospérité · dL→stabilité & croissance · dPuissance→prospérité+Brèche ·*/
/* dH→coercition · dFracture→fragilité · flux/charge→Brèche · et NODE_PROD_  */
/* PCT/EFF_PCT (plus bas) pour le +production/+efficacité concret. (dEco/dMil/*/
/* dF sont des champs MORTS du TechState — jamais lus — donc laissés à 0.)   */
/* ====================================================================== */
/* ---- Ésotérique (Savoir·Renforcement → COMMUNION) : +stabilité --------- */
[TECH_GLYPHES_ETHERES] = { "Glyphes éthérés","Cercle de glyphes", THM_SAVOIR,FN_RENFORCEMENT,1, TECH_BIBLIOTHEQUE, false,false,HERITAGE_ESOTERIQUE,
    0,0.5f,0, 0,0, 0, 0, 0.3f, 0, 0.10f, false,
    "+stabilité (glyphes protecteurs) · légère puissance brute",
    "Un glyphe bien tracé protège des mauvais esprits ; contre les mauvais impôts, on n'a rien trouvé." },
[TECH_COMMUNION_ETHEREE] = { "Communion éthérée","Bastion éthéré", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_GLYPHES_ETHERES, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0.6f, 0.15f, 0.40f, false,
    "+stabilité & +cohésion (l'harmonie éthérée) · puissance brute accrue",
    "Le bastion éthéré relie toutes les âmes du royaume — pratique, pour savoir qui pense encore de travers." },
/* ---- Métallurgiste (Forge·Armée → FORGE_RUNES) : +production ----------- */
[TECH_ALLIAGES_NAINS] = { "Alliages des profondeurs","Fonderie de bronze", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_ARGILE, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+dégâts d'armes (doctrine) · légère stabilité — métallurgie de fond",
    "Les alliages des profondeurs sortent de mines qu'on n'a jamais fini de creuser, ni de vider." },
[TECH_GRAVURE_RUNES] = { "Gravure runique","Rune-forge", THM_FORGE,FN_ARMEE,2, TECH_ALLIAGES_NAINS, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.50f, false,
    "+dégâts d'armes & +puissance brute (runes) · flux modéré vers la Brèche",
    "Graver une rune sur un marteau ne le rend pas magique — mais le forgeron facture comme si c'était le cas." },
/* ---- Mécaniste (Forge·Prod→Renf → AUTOMATES) : +production/+efficacité - */
[TECH_MECANISTE_ROUAGES] = { "Rouages de précision","Engrenagerie", THM_FORGE,FN_PRODUCTION,1, TECH_FONDERIE, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0, 0.05f, 0.20f, false,
    "+production (mécanique de précision) · +capacité narrative & stabilité",
    "Un rouage de précision tourne exactement comme prévu — le seul rouage du royaume dans ce cas." },
[TECH_MECANISTE_HORLOGERIE] = { "Mécanisme d'horlogerie","Horloge mécanique", THM_FORGE,FN_RENFORCEMENT,2, TECH_MECANISTE_ROUAGES, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.30f, false,
    "+efficacité d'emploi (synchronisation) · +capacité narrative & stabilité",
    "L'horloge mécanique donne l'heure exacte à tout le royaume — l'heure du prochain impôt, notamment." },
/* ---- Adaptatif (Société·Renforcement → INTEGRATION) : +cohésion -------- */
[TECH_DROIT_COUTUMIER] = { "Droit coutumier","Code coutumier", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_CHANCELLERIE, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+stabilité (droit coutumier, le compromis codifié)",
    "Le droit coutumier consiste à écrire noir sur blanc ce que tout le monde faisait déjà de toute façon." },
[TECH_LANGUE_FRANQUE] = { "Langue franque","Lingua franca", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_DROIT_COUTUMIER, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, -1.0f, 0, 0, 0, false,
    "+cohésion & +stabilité (langue commune) · +capacité narrative · −fracture",
    "Une langue commune règle bien des différends — surtout ceux qui portaient uniquement sur des malentendus." },
/* ---- Agraire (Société·Production → ABONDANCE) : +production agricole --- */
[TECH_VERGERS_ETAGES] = { "Vergers étagés","Vergers en terrasses", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+production agricole (vergers en terrasses) · légère stabilité",
    "Étager les vergers sur la colline permet à chaque paysan de voir, d'en haut, celui qui a la meilleure vue." },
[TECH_PATURAGES_INTEGRES] = { "Pâturages intégrés","Prairies-vergers", THM_SOCIETE,FN_PRODUCTION,2, TECH_VERGERS_ETAGES, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+production & +croissance (polyculture) · stabilité accrue",
    "Mélanger bétail et vergers sur la même terre : la polyculture, ou l'art de ne mettre aucun œuf dans le même panier — sauf tous." },
/* ---- Clanique (Société·Armée → ESCLAVAGE) : +stabilité / +production --- */
[TECH_RITES_GUERRIERS] = { "Rites guerriers","Sanctuaire des ancêtres", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,HERITAGE_CLANIQUE,
    0,0.3f,0, 0,0, 0.2f, 0, 0, 0, 0, false,
    "+moral des troupes (discipline martiale) · légère coercition",
    "Honorer les ancêtres au champ de bataille coûte moins cher que de bons soldes, et fonctionne presque aussi bien." },
[TECH_HORDES_CONQUERANTES] = { "Hordes conquérantes","Camps de rapine", THM_SOCIETE,FN_ARMEE,2, TECH_RITES_GUERRIERS, false,false,HERITAGE_CLANIQUE,
    0,0.5f,0, 0,0, 0.3f, 0.5f, 0, 0, 0, false,
    "+moral des troupes · +coercition · fracture interne en hausse",
    "Le camp de rapine finance la horde suivante avec le butin de la précédente — un modèle économique étonnamment stable." },

/* ====================================================================== */
/* COMBOS (2026-06-28) — TIER-4 COMBINATOIRE EXCLUSIF (une paire d'héritages)*/
/* prereq=NONE : la PORTE est la double-métabolisation (accès tier 3 aux deux*/
/* héritages, via tech_combo_native + le plafond de tier dans can_research) +*/
/* le coût tier-4. native=héritage A ; tech_combo_native renvoie l'héritage B.*/
/* Effets sur leviers VIVANTS : FN_ARMEE → army_doctrine (auto, tier4=+20/28%);*/
/* FN_PRODUCTION/RENFORCEMENT → NODE_PROD_PCT/EFF_PCT (plus bas) + prospérité. */
/* ====================================================================== */
[TECH_COMBO_POUDRE] = { "Arquebuserie de précision","Manufacture d'armes à feu", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Méca×Métal : +dégâts (feu) via doctrine + poudre (prod%) */
    "exige l'accès plein Mécaniste+Métallurgiste · +dégâts (doctrine, tier 4)",
    "L'ingénieur et le métallurgiste se sont enfin parlé — le résultat troue l'acier à cent pas." },
[TECH_COMBO_AUTOMATES_ARC] = { "Automates arcanes","Golems d'essence", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 1.0f, 0, 0, false,         /* Éso×Méca : +production (prod%) + puissance */
    "exige l'accès plein Ésotérique+Mécaniste · +10 % de production · +puissance brute",
    "Un golem animé par de l'essence pure ne demande jamais d'augmentation — c'est là tout son avantage sur le personnel." },
[TECH_COMBO_ACADEMIE] = { "Académie cosmopolite","Grande académie", THM_SAVOIR,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    1.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Adaptatif : +recherche (Savoir·Prod) + efficacité */
    "exige l'accès plein Ésotérique+Adaptatif · +1 capacité narrative & +efficacité",
    "L'académie cosmopolite accueille tous les savoirs du monde, à condition qu'ils citent nos érudits en premier." },
[TECH_COMBO_DRUIDE] = { "Abondance druidique","Bosquet nourricier", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Agraire : +production agricole (prod%) + stabilité */
    "exige l'accès plein Ésotérique+Agraire · +8 % de production agricole · +stabilité",
    "Le bosquet nourricier fait pousser le blé plus vite que la raison ne pousse chez les prêtres qui l'ont béni." },
[TECH_COMBO_CHAMAN] = { "Chamanisme de guerre","Cercle des chamans", THM_SAVOIR,FN_ARMEE,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Éso×Clanique : +magie de guerre via doctrine (arcane_power) */
    "exige l'accès plein Ésotérique+Clanique · +puissance arcane des armées (doctrine, tier 4)",
    "Le chaman de guerre invoque les esprits des ancêtres ; les ancêtres, eux, n'ont jamais rien demandé." },
[TECH_COMBO_GUILDES] = { "Guildes maîtresses","Hôtel des guildes", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Métal×Adaptatif : +production (prod%) + or/stabilité */
    "exige l'accès plein Métallurgiste+Adaptatif · +8 % de production · +stabilité",
    "L'hôtel des guildes régule les prix, les métiers et surtout qui a le droit d'en pratiquer un." },
[TECH_COMBO_CHARRUES] = { "Charrues lourdes","Atelier de charronnage", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Métal×Agraire : +production agricole (prod%) */
    "exige l'accès plein Métallurgiste+Agraire · +8 % de production agricole",
    "La charrue lourde retourne la terre plus profondément — et enterre un peu plus profondément aussi le paysan qui la tire." },
[TECH_COMBO_POLIORCETIQUE] = { "Poliorcétique","Forge de guerre", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Métal×Clanique : +dégâts via doctrine */
    "exige l'accès plein Métallurgiste+Clanique · +dégâts (doctrine, tier 4)",
    "La science du siège se résume à une idée simple : le mur le plus haut finit toujours par tomber, juste plus cher." },
[TECH_COMBO_HORLOGE_MARCH] = { "Horlogerie marchande","Comptoir mécanique", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Méca×Adaptatif : +efficacité (eff%) + or/stabilité */
    "exige l'accès plein Mécaniste+Adaptatif · +10 % d'efficacité · +stabilité",
    "Le comptoir mécanique calcule les taux de change plus vite qu'un marchand — et sans jamais se sentir coupable." },
[TECH_COMBO_MACHINES_AGRI] = { "Machines agricoles","Moulins & semoirs", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Méca×Agraire : +production agricole (prod%, fort) */
    "exige l'accès plein Mécaniste+Agraire · +12 % de production agricole",
    "Le moulin mécanique fait le travail de vingt paysans — les dix-neuf de trop iront grossir les villes." },
[TECH_COMBO_SIEGE] = { "Engins de siège","Arsenal de siège", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Méca×Clanique : +dégâts via doctrine */
    "exige l'accès plein Mécaniste+Clanique · +dégâts (doctrine, tier 4)",
    "L'engin de siège n'a qu'un argument, mais il est très convaincant contre n'importe quel mur." },
[TECH_COMBO_GRENIER_COLON] = { "Grenier colonial","Comptoir-grenier", THM_SOCIETE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, 0, 0, 0, 0, false,      /* Adaptatif×Agraire : +stabilité & +croissance (K,L) */
    "exige l'accès plein Adaptatif+Agraire · +capacité narrative & +légitimité",
    "Le grenier colonial nourrit la métropole d'abord, la colonie ensuite, et le colon jamais en premier." },
[TECH_COMBO_FOEDERATI] = { "Foederati","Camp fédéré", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_ADAPTATIF,
    0,0.5f,0, 0,0, 0, -0.5f, 0, 0, 0, false,     /* Adaptatif×Clanique : +moral via doctrine + cohésion */
    "exige l'accès plein Adaptatif+Clanique · +moral (doctrine, tier 4) · −fracture",
    "Les foederati combattent sous notre bannière pour un salaire — leur loyauté, elle, reste facturée à part." },
[TECH_COMBO_HORDE_ECO] = { "Économie de horde","Halle du butin", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Agraire×Clanique : +moral via doctrine + production (prod%) */
    "exige l'accès plein Agraire+Clanique · +moral (doctrine, tier 4) · +6 % de production",
    "L'économie de horde repose sur un principe agricole simple : on récolte ce que le voisin a semé." },

/* ====================================================================== */
/* APEX TRIPLES (2026-06-28) — TIER-5 : la fusion de TROIS héritages (accès PLEIN aux 3).   */
/* Le pinacle. N=3 via tech_combo_native (2e) + tech_combo_native2 (3e). prereq=NONE (la    */
/* porte = la triple-métabolisation + le coût tier-5). Effets sur leviers vivants.          */
/* ====================================================================== */
[TECH_APEX_ARQUEBUSE] = { "Arquebuse runique","Arsenal runique à feu", THM_FORGE,FN_ARMEE,5, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,            /* Méca×Métal×Éso : +dégâts (doctrine) + ARQUEBUSIERS ciblés (firearm_power) */
    "exige l'accès plein Mécaniste+Métallurgiste+Ésotérique · +dégâts (doctrine, tier 5) · +50 % de dégâts ARQUEBUSIERS (ciblé)",
    "Trois peuples ont fusionné leur génie pour ce pinacle : un fusil qui tire des runes. Personne n'a demandé si c'était sage." },
[TECH_APEX_CONCILE] = { "Concile des savants","Grand concile", THM_SAVOIR,FN_PRODUCTION,5, NONE, false,false,HERITAGE_ESOTERIQUE,
    1.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Adaptatif×Méca : +recherche (Savoir·Prod) + prod% */
    "exige l'accès plein Ésotérique+Adaptatif+Mécaniste · +1 capacité narrative · +12 % d'efficacité",
    "Le concile réunit les plus grands esprits de trois peuples — pour débattre, surtout, de qui présidera." },
[TECH_APEX_LEGION] = { "Légion universelle","Camp des nations", THM_SOCIETE,FN_ARMEE,5, NONE, false,false,HERITAGE_ADAPTATIF,
    0,0.5f,0, 0,0, 0, -0.5f, 0, 0, 0, false,     /* Adaptatif×Métal×Clanique : +moral (doctrine) + cohésion */
    "exige l'accès plein Adaptatif+Métallurgiste+Clanique · +moral (doctrine, tier 5) · +stabilité · −fracture",
    "La légion universelle accueille tous les peuples sous un même étendard — celui qui paie le mieux, en général." },
};

/* ====================================================================== */
/* RECETTES DE FUSION (intrants géologiques + enabler)                     */
/* ====================================================================== */
static const FusionRecipe FUSIONS[FUSION_COUNT] = {
    { "Poudre noire",     ING_COMBURANT,   ING_COMBUSTIBLE, TECH_FONDERIE,         2.0f,0.0f,0.0f,0.0f },
    { "Acier",            ING_COMBUSTIBLE, ING_MINERAI,     TECH_MANUFACTURE,      2.0f,0.0f,0.0f,0.2f },
    { "Béton/ciment",     ING_LIANT,       ING_MINERAI,     TECH_QUALITE_MATERIAUX,0.0f,1.5f,0.0f,0.0f },
    { "Armes enchantées", ING_MINERAI,     ING_CATALYSEUR,  TECH_FORGE_RUNES,      3.0f,0.0f,1.0f,1.0f },
    { "Cœur de pacte",    ING_CATALYSEUR,  ING_CATALYSEUR,  TECH_SAVOIR_INTERDIT,  0.0f,0.0f,2.0f,3.0f },
};
const FusionRecipe *tech_fusion_table(void) { return FUSIONS; }

/* ====================================================================== */
/* NŒUDS SYNCRÉTIQUES — diffusion d'une tradition par CONTACT (§4-8)        */
/* Chacun pend d'un nœud de base et se loquette quand l'archétype-source    */
/* est atteint à la profondeur requise. Diffusion BÉNÉFIQUE (jamais faust.).*/
/* La SURFACE (comptoir) passe les rudiments ; le MÉTIER (frontière/foi) le  */
/* savoir-faire ; le SECRET (gouvernance digérée) reste réservé aux         */
/* signatures profondes de l'arbre de base (gate native, déjà en place).    */
/* ====================================================================== */
/* Chaque nœud PEND d'un nœud de base POSITIONNÉ (tier ≥ 1) : il n'apparaît dans le cercle
 * qu'une fois ce parent acquis, et le clic sur le parent ouvre l'anneau de ses sous-techs. */
static const SyncNode SYNCS[SYNC_COUNT] = {
    { "Comptoir arcanique","Rudiments arcanes",    HERITAGE_ESOTERIQUE,    PROF_SURFACE, TECH_SAVOIR_GUERRE,    1.0f,0,0,   0,0 },
    { "Maçonnerie runique","Pierre des montagnes", HERITAGE_METALLURGISTE,    PROF_METIER,  TECH_QUALITE_MATERIAUX,0,0.5f,0,   0.5f,0 },
    { "École d'ingénierie","Mécanismes empruntés", HERITAGE_MECANISTE,   PROF_METIER,  TECH_FONDERIE,         0,0,0,      1.5f,0 },
    { "Doctrine d'accueil","Creuset emprunté",     HERITAGE_ADAPTATIF,  PROF_METIER,  TECH_CHANCELLERIE,     0,0.5f,1.0f,0,0 },
    { "Hospice pastoral","Abondance partagée",     HERITAGE_AGRAIRE,PROF_SURFACE, TECH_IRRIGATION,       0,1.0f,0,   1.0f,0 },
    { "Garde étrangère","Discipline d'emprunt",    HERITAGE_CLANIQUE,   PROF_METIER,  TECH_CONSCRIPTION,     0,0,0,      0,1.5f },
    /* profils d'ÉTHOS (briefs Savoir/Société §5) : la bureaucratie diffuse le scriptorium
     * au coude-à-coude (métier), le marchand répand le comptoir/cothon par le négoce (surface). */
    { "Scriptorium d'emprunt","Écriture administrative", ARCH_BUREAUCRATIQUE, PROF_METIER,  TECH_WARDS,    1.5f,0,0, 0,0 },
    { "Cothon","Bassin marchand",                        ARCH_MERCANTILE,     PROF_SURFACE, TECH_COMMERCE, 0,0,0,    2.0f,0 },
};
const SyncNode *tech_sync_node(int i){ return (i>=0&&i<SYNC_COUNT)?&SYNCS[i]:NULL; }

int tech_sync_tick(TechState *s, const unsigned char depth[ARCH_COUNT]){
    int newl=0;
    for (int i=0;i<SYNC_COUNT;i++){
        if (s->sync_unlocked[i]) continue;
        const SyncNode *sn=&SYNCS[i];
        if (sn->parent!=NONE && !s->unlocked[sn->parent]) continue;     /* cercle visible une fois le parent acquis */
        int a=sn->arch; if (a<0||a>=ARCH_COUNT) continue;
        if (depth[a] < (unsigned char)sn->prof_requise) continue;        /* archétype pas atteint à la profondeur requise */
        s->sync_unlocked[i]=true; s->n_sync++; newl++;
        s->K+=sn->dK; s->L+=sn->dL; s->F+=sn->dF; s->eco+=sn->dEco; s->mil+=sn->dMil;  /* diffusion : LOQUET permanent */
    }
    return newl;
}

/* ====================================================================== */
/* API                                                                    */
/* ====================================================================== */
void tech_state_init(TechState *s, bool has_ruins_access) {
    for (int i=0;i<TECH_COUNT;i++) s->unlocked[i]=false;
    s->n_unlocked=0;
    /* Socle de départ : un peu de K/L/F, et les 6 BÂTIMENTS DE BASE (centre)
     * déjà acquis — au début, rien d'autre. */
    s->K=3.0f; s->L=3.0f; s->F=2.0f;
    s->eco=0.f; s->mil=0.f; s->puissance=0.f;
    s->H=0.f; s->fracture=0.f; s->charge=0.f;
    s->has_ruins_access=has_ruins_access;
    s->crisis_triggered=false;
    s->research_points=0.f;
    for (int i=0;i<SYNC_COUNT;i++) s->sync_unlocked[i]=false;
    s->n_sync=0;
    for (int i=0;i<ARCH_COUNT;i++) s->arch_depth[i]=PROF_NONE;
    for (int i=0;i<TECH_COUNT;i++) if (NODES[i].tier==0){ s->unlocked[i]=true; s->n_unlocked++; }
}

/* Rendement de recherche : la spine SAVOIR·Production accélère la recherche. */
float tech_research_yield(const TechState *s){
    float y=1.0f;
    for (int i=0;i<TECH_COUNT;i++)
        if (s->unlocked[i] && NODES[i].theme==THM_SAVOIR && NODES[i].func==FN_PRODUCTION && NODES[i].tier>0)
            y += 0.5f;                 /* Scriptorium / Académie / Université */
    return y;
}

/* §B1 — techs de PRODUCTION : multiplicateurs MODESTES (non faustiens, charge 0) dispatchés
 * thématiquement. prod_pct abonde la production (prod_mult) ; eff_pct l'efficacité d'emploi.
 * Tables par nœud (0 par défaut) — le pain quotidien de l'arbre, le gain sain de spécialisation. */
static float NODE_PROD_PCT[TECH_COUNT] = {   /* NON-const (MODTOOLS) — surchargeable par SCPS_MODS */
    /* Forge·Production — « le multiplicateur de rendement » : extraction + manufacture. */
    [TECH_FONDERIE]=0.08f, [TECH_OUTILLAGE]=0.10f, [TECH_MANUFACTURE]=0.12f, [TECH_INDUSTRIE]=0.15f,
    /* Société·Production — rendement agricole / efficacité du commerce. */
    [TECH_IRRIGATION]=0.06f, [TECH_COMMERCE]=0.08f, [TECH_CADASTRE]=0.08f, [TECH_ABONDANCE]=0.10f,
    /* ÉTOFFE — rungs d'héritage à VOCATION PRODUCTION (les FN_ARMÉE vont à army_doctrine). */
    [TECH_MECANISTE_ROUAGES]=0.06f,
    [TECH_VERGERS_ETAGES]=0.05f, [TECH_PATURAGES_INTEGRES]=0.07f,
    /* COMBOS tier-4 à vocation production (le +production CONCRET de la fusion). */
    [TECH_COMBO_AUTOMATES_ARC]=0.10f, [TECH_COMBO_DRUIDE]=0.08f,
    [TECH_COMBO_GUILDES]=0.08f, [TECH_COMBO_CHARRUES]=0.08f,
    [TECH_COMBO_MACHINES_AGRI]=0.12f, [TECH_COMBO_HORDE_ECO]=0.06f,
};
static float NODE_EFF_PCT[TECH_COUNT] = {   /* NON-const (MODTOOLS) */
    /* Savoir·Production — le savoir-faire rend chaque bras meilleur (efficacité d'emploi). */
    [TECH_SCRIPTORIUM]=0.05f, [TECH_ACADEMIE]=0.07f, [TECH_UNIVERSITE]=0.10f,
    /* ÉTOFFE — l'horlogerie gnome SYNCHRONISE la production (efficacité d'emploi). */
    [TECH_MECANISTE_HORLOGERIE]=0.08f,
    /* COMBOS tier-4 à vocation efficacité/savoir. */
    [TECH_COMBO_HORLOGE_MARCH]=0.10f, [TECH_COMBO_ACADEMIE]=0.08f,
    /* APEX : le Concile des savants — l'efficacité du savoir des trois peuples. */
    [TECH_APEX_CONCILE]=0.12f,
};
float tech_prod_bonus(const TechState *s){
    float b=0.f; if(!s) return 0.f;
    for (int i=0;i<TECH_COUNT;i++) if (s->unlocked[i]) b+=NODE_PROD_PCT[i];
    return b;
}
float tech_eff_bonus(const TechState *s){
    float b=0.f; if(!s) return 0.f;
    for (int i=0;i<TECH_COUNT;i++) if (s->unlocked[i]) b+=NODE_EFF_PCT[i];
    return b;
}

/* Le penchant d'une heritage = le thème de sa signature (lecture, pas de « si heritage »). */
TechTheme tech_heritage_affinity(Heritage r){
    for (int i=0;i<TECH_COUNT;i++)
        if (NODES[i].native==r) return NODES[i].theme;   /* le thème où sa signature niche */
    return THM_SOCIETE;                                  /* défaut : le socle */
}

const TechNode *tech_node(TechId id){ return (id>=0&&id<TECH_COUNT)?&NODES[id]:NULL; }
const char *tech_name(TechId id){ return (id>=0&&id<TECH_COUNT)?NODES[id].name:"?"; }
const char *tech_unlocks(TechId id){ return (id>=0&&id<TECH_COUNT)?NODES[id].unlocks:"?"; }
/* PACK FLAVOR (display-only) — NULL-safe : hors-borne ou champ vide ⇒ "". */
const char *tech_hover(TechId id){
    if (id<0||id>=TECH_COUNT) return "";
    const char *h = NODES[id].hover;
    return h ? h : "";
}
const char *tech_flavor(TechId id){
    if (id<0||id>=TECH_COUNT) return "";
    const char *f = NODES[id].flavor;
    return f ? f : "";
}
const char *tech_theme_name(TechTheme t){
    static const char *N[THM_COUNT]={"Savoir","Forge","Société"};
    return (t>=0&&t<THM_COUNT)?N[t]:"?";
}
const char *tech_function_name(TechFunction f){
    static const char *N[FN_COUNT]={"Production","Armée","Renforcement"};
    return (f>=0&&f<FN_COUNT)?N[f]:"?";
}
int  tech_quarter(TechTheme t, TechFunction f){ return (int)t*FN_COUNT + (int)f; }
bool tech_is_base(TechId id){ return (id>=0&&id<TECH_COUNT)&&NODES[id].tier==0; }

/* ACCÈS D'HÉRITAGE GRADUÉ (Temps 2) — le masque `heritage_access` n'est plus binaire : il
 * encode, par héritage, le TIER d'accès atteint (0..3) sur 2 bits → tier·(1<<2r). Une tech
 * de signature au tier T n'est recherchable que si l'accès à son héritage atteint T (la
 * « barre » : commerce → tier 1-2, gouvernance/métabolisation → tier 3). tech_heritage_bit
 * garde sa SÉMANTIQUE D'OCTROI PLEIN (tier 3) pour les bancs/helpers qui « donnent l'accès ». */
unsigned tech_heritage_bit(Heritage r){ return (r>=0&&r<HERITAGE_COUNT)?(3u<<(2*r)):0u; }
int tech_heritage_access_tier(unsigned access, Heritage r){
    return (r>=0&&r<HERITAGE_COUNT) ? (int)((access>>(2*r))&3u) : 0;
}

/* §SYNCRÉTIQUE — COMBINAISON (brief Forge §5/§8) : un nœud-pointe peut exiger DEUX
 * archétypes culturels en contact, pas un seul. Emblème : les armes enchantées (Forge
 * céleste) = FORGE RUNIQUE (métallurgiste) × ARCANE (ésotérique) — il faut porter/gouverner les DEUX
 * cultures, le commerce seul ne suffit pas (la chaîne BLD_CELESTIAL_FORGE existe déjà :
 * on gate l'UNLOCK, pas la production). UNIV = aucun second requis. La porte PRIMAIRE
 * reste NODES[].native ; le masque `heritage_access` encode désormais l'ACCÈS D'ARCHÉTYPE. */
static Heritage tech_combo_native(TechId id){
    switch (id){
        case TECH_FORGE_RUNES: return HERITAGE_ESOTERIQUE;   /* runique (métallurgiste) ET arcane (ésotérique) */
        /* COMBOS tier-4 : le SECOND héritage requis (le premier = NODES[].native). */
        case TECH_COMBO_POUDRE:        return HERITAGE_METALLURGISTE; /* Méca × Métal */
        case TECH_COMBO_AUTOMATES_ARC: return HERITAGE_MECANISTE;     /* Éso × Méca */
        case TECH_COMBO_ACADEMIE:      return HERITAGE_ADAPTATIF;     /* Éso × Adaptatif */
        case TECH_COMBO_DRUIDE:        return HERITAGE_AGRAIRE;       /* Éso × Agraire */
        case TECH_COMBO_CHAMAN:        return HERITAGE_CLANIQUE;      /* Éso × Clanique */
        case TECH_COMBO_GUILDES:       return HERITAGE_ADAPTATIF;     /* Métal × Adaptatif */
        case TECH_COMBO_CHARRUES:      return HERITAGE_AGRAIRE;       /* Métal × Agraire */
        case TECH_COMBO_POLIORCETIQUE: return HERITAGE_CLANIQUE;      /* Métal × Clanique */
        case TECH_COMBO_HORLOGE_MARCH: return HERITAGE_ADAPTATIF;     /* Méca × Adaptatif */
        case TECH_COMBO_MACHINES_AGRI: return HERITAGE_AGRAIRE;       /* Méca × Agraire */
        case TECH_COMBO_SIEGE:         return HERITAGE_CLANIQUE;      /* Méca × Clanique */
        case TECH_COMBO_GRENIER_COLON: return HERITAGE_AGRAIRE;       /* Adaptatif × Agraire */
        case TECH_COMBO_FOEDERATI:     return HERITAGE_CLANIQUE;      /* Adaptatif × Clanique */
        case TECH_COMBO_HORDE_ECO:     return HERITAGE_CLANIQUE;      /* Agraire × Clanique */
        /* APEX TRIPLES : le 2e des trois héritages (le 3e = tech_combo_native2). */
        case TECH_APEX_ARQUEBUSE:      return HERITAGE_METALLURGISTE; /* Méca × MÉTAL × Éso */
        case TECH_APEX_CONCILE:        return HERITAGE_ADAPTATIF;     /* Éso × ADAPTATIF × Méca */
        case TECH_APEX_LEGION:         return HERITAGE_METALLURGISTE; /* Adaptatif × MÉTAL × Clanique */
        default:               return UNIV;
    }
}
/* APEX TRIPLES — le TROISIÈME héritage requis (UNIV pour tout le reste : pas de 3e porte). */
static Heritage tech_combo_native2(TechId id){
    switch (id){
        case TECH_APEX_ARQUEBUSE:      return HERITAGE_ESOTERIQUE;    /* Méca × Métal × ÉSO */
        case TECH_APEX_CONCILE:        return HERITAGE_MECANISTE;     /* Éso × Adaptatif × MÉCA */
        case TECH_APEX_LEGION:         return HERITAGE_CLANIQUE;      /* Adaptatif × Métal × CLANIQUE */
        default:               return UNIV;
    }
}

bool tech_can_research(const TechState *s, TechId id, unsigned heritage_access) {
    if (id<0||id>=TECH_COUNT) return false;
    if (s->unlocked[id]) return false;
    const TechNode *n=&NODES[id];
    /* PORTE D'ARCHÉTYPE : une tech-signature exige que l'empire ATTEIGNE l'archétype
     * (par sa culture ou un contact de gouvernance — le masque est calculé ainsi côté IA). */
    /* L'accès gradué plafonne à tier 3 (accès PLEIN) ; un nœud tier-4 (combo) exige donc
     * l'accès PLEIN (3) à son/ses héritage(s) — natif OU pleinement métabolisé. */
    int need = n->tier > 3 ? 3 : n->tier;
    if (n->native!=UNIV && tech_heritage_access_tier(heritage_access, n->native) < need) return false;
    /* COMBINAISON : certains nœuds exigent un SECOND (et, pour les APEX, un TROISIÈME)
     * archétype (ET), au même tier requis (accès PLEIN aux 2-3 héritages). */
    { Heritage combo=tech_combo_native(id);
      if (combo!=UNIV && tech_heritage_access_tier(heritage_access, combo) < need) return false;
      Heritage combo2=tech_combo_native2(id);
      if (combo2!=UNIV && tech_heritage_access_tier(heritage_access, combo2) < need) return false; }
    /* Porte arcane : les bouts faustiens du Savoir profond exigent une ruine. */
    if (n->needs_ruins && !s->has_ruins_access) return false;
    /* Prérequis : le nœud précédent du quartier doit être acquis. */
    if (n->prereq!=NONE && !s->unlocked[n->prereq]) return false;
    return true;
}

bool tech_research(TechState *s, TechId id, unsigned heritage_access) {
    if (!tech_can_research(s,id,heritage_access)) return false;
    const TechNode *n=&NODES[id];
    s->K        += n->dK;
    s->L        += n->dL;
    s->F        += n->dF;
    s->eco      += n->dEco;
    s->mil      += n->dMil;
    s->H        += n->dH;
    s->fracture += n->dFracture; if (s->fracture<0.f) s->fracture=0.f;
    s->puissance+= n->dPuissance;
    s->charge   += n->charge;          /* sens unique : jamais remboursé */
    if (n->triggers_crisis) s->crisis_triggered=true;
    s->unlocked[id]=true;
    s->n_unlocked++;
    return true;
}

float tech_cost(TechId id, float n_provinces){
    const TechNode *n=tech_node(id);
    if (!n) return 0.f;
    int t=n->tier; if (t<0) t=0; if (t>5) t=5;
    float N = (n_provinces>1.f ? n_provinces : 1.f);
    float f = TECH_COST_N_K * powf(N, TECH_COST_N_EXP);    /* coût ∝ √N : wide récompensé sous-linéairement */
    if (f<TECH_COST_N_FLOOR) f=TECH_COST_N_FLOOR;
    if (!(f<1e6f)) f=1e6f;   /* un N inf/NaN ne doit pas geler la recherche (coût inf) */
    return BASE_COST[t] * COST_SCALE * f;
}

/* ---- La Brèche (verrou SCPS, inchangé) -------------------------------- */
float tech_flux(const TechState *s) {
    float f=0.f;
    for (int i=0;i<TECH_COUNT;i++) if (s->unlocked[i]) f+=NODES[i].flux;
    return f;
}
float tech_dereal(const TechState *s) {
    float d = DEREAL_P_COEF*s->puissance*s->charge + tech_flux(s) - s->K;
    return (d>0.f)?d:0.f;
}
float tech_crisis_proximity(const TechState *s) {
    if (s->crisis_triggered) return 1.0f;
    return 1.0f - expf(-s->charge/CRISIS_SCALE);
}
/* FAU4 GAP — la charge-de-PROFONDEUR somme TOUS les nœuds faustiens pris, quelle que soit la
 * branche (Savoir, Forge OU Société). Le modèle 3-branches veut qu'un Martial/Forge profond
 * (forge runique) pèse sur l'entropie comme un Savoir profond — sinon la symétrie des trois
 * apocalypses ment. (Avant : THM_SAVOIR seul.) */
static float arcane_charge(const TechState *s) {
    float m=0.f;
    for (int i=0;i<TECH_COUNT;i++)
        if (s->unlocked[i] && NODES[i].faustian) m+=NODES[i].charge;
    return m;
}
float tech_shock_amplitude(const TechState *s) {
    return s->charge * (1.0f + SHOCK_MAGIE*arcane_charge(s));
}
float tech_fragility(const TechState *s) {
    float order = s->L - s->H;
    if (order < 0.5f) order = 0.5f;
    return s->fracture / order;
}

bool tech_fusion_available(const TechState *s, int recipe_idx,
                           const bool has_ingredient[ING_COUNT]) {
    if (recipe_idx<0||recipe_idx>=FUSION_COUNT) return false;
    const FusionRecipe *r=&FUSIONS[recipe_idx];
    if (!s->unlocked[r->enabler]) return false;
    if (!has_ingredient[r->in1]) return false;
    if (!has_ingredient[r->in2]) return false;
    return true;
}

/* ── MODTOOLS — surcharge des COÛTS/BONUS de tech par fichier (SCPS_MODS) ─────
 * basecost<TAB><tier 0-5><TAB><coût>  ·  techbonus<TAB><tech><TAB><prod_pct><TAB><eff_pct>
 * Sans fichier ⇒ valeurs compilées ⇒ golden/déterminisme INTACTS. */
static int tech_split(char *line, char *out[], int maxf){
    int n=0; char *p=line;
    while (n<maxf){ out[n++]=p; char *t=strchr(p,'\t'); if(!t) break; *t=0; p=t+1; }
    return n;
}
static int tech_by_name(const char *t){
    for (int i=0;i<TECH_COUNT;i++){ const char *n=tech_name((TechId)i); if(n&&strcmp(n,t)==0) return i; }
    return -1;
}
void tech_moddata_dump(FILE *f){
    if(!f) return;
    fprintf(f,"# basecost\t<tier 0-5>\t<coût de base>\n");
    for(int t=0;t<6;t++) fprintf(f,"basecost\t%d\t%.4g\n",t,BASE_COST[t]);
    fprintf(f,"# techbonus\t<tech>\t<prod_pct>\t<eff_pct>  (techs à bonus non nul)\n");
    for(int i=0;i<TECH_COUNT;i++)
        if(NODE_PROD_PCT[i]!=0.f||NODE_EFF_PCT[i]!=0.f)
            fprintf(f,"techbonus\t%s\t%.4g\t%.4g\n",tech_name((TechId)i),NODE_PROD_PCT[i],NODE_EFF_PCT[i]);
}
int tech_moddata_load(const char *path){
    if(!path||!*path) return -1;
    FILE *f=fopen(path,"r"); if(!f) return -1;
    char line[256]; int applied=0; char *fld[5];
    while(fgets(line,sizeof line,f)){
        if(line[0]=='#') continue;
        char *nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
        int nf=tech_split(line,fld,5); if(nf<3) continue;
        if(strcmp(fld[0],"basecost")==0){
            int t=atoi(fld[1]); if(t<0||t>5) continue;
            BASE_COST[t]=(float)atof(fld[2]); applied++;
        } else if(strcmp(fld[0],"techbonus")==0 && nf>=4){
            int i=tech_by_name(fld[1]); if(i<0) continue;
            NODE_PROD_PCT[i]=(float)atof(fld[2]); NODE_EFF_PCT[i]=(float)atof(fld[3]); applied++;
        }
    }
    fclose(f);
    if(applied>0) fprintf(stderr,"[mods] tech : %d valeur(s) surchargée(s) depuis %s.\n",applied,path);
    return applied;
}
