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
/* ⚠ tier-0 : les deltas des bâtiments de BASE ne sont JAMAIS appliqués (tech_state_init les
 * marque acquis sans passer par tech_research) — leurs champs restent à 0 pour que le hover
 * chiffré (scps_api) n'affiche pas un bonus fantôme. */
[TECH_BIBLIOTHEQUE] = { "Bibliothèque","Bibliothèque", THM_SAVOIR,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "bâtiment de base — ouvre la branche du savoir (Scriptorium, Académie, Université)",
    "Quand le premier registre fut relié de cuir, le royaume découvrit qu'une parole écrite survivait même à celui qui l'avait jurée." },
[TECH_SCRIPTORIUM] = { "Scriptorium","Scriptorium", THM_SAVOIR,FN_PRODUCTION,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    1,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+50 % au rendement de recherche",
    "À la lueur des chandelles, cent mains recopièrent la même vérité, et chacune y laissa une faute différente." },
[TECH_ACADEMIE] = { "Académie","Académie", THM_SAVOIR,FN_PRODUCTION,2, TECH_SCRIPTORIUM, false,false,UNIV,
    2,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+50 % au rendement de recherche (cumulatif)",
    "On y entra pour apprendre à répondre. On en sortit surtout avec de meilleures questions et de plus puissants protecteurs." },
[TECH_UNIVERSITE] = { "Université","Université", THM_SAVOIR,FN_PRODUCTION,3, TECH_ACADEMIE, false,false,UNIV,
    3,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+50 % au rendement de recherche (cumulatif)",
    "Les maîtres jurèrent de ne servir que le savoir. Le chancelier leur rappela qui payait le parchemin." },
/* ---- SAVOIR · ARMÉE (arcane offensif — faustien) --------------------- */
[TECH_SAVOIR_GUERRE] = { "Savoir de guerre","Collège de guerre", THM_SAVOIR,FN_ARMEE,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0,0, 0,1.0f, 0, 0, 0, 0.05f, 0.3f, false,
    "+7 % aux dégâts des mages (doctrine) · léger flux vers la Brèche",
    "Le mage de cour traça un cercle dans la boue. Les vétérans cessèrent de rire lorsque l'ennemi y entra." },
[TECH_MAGIE_BATAILLE] = { "Magie de bataille","Tour de mages", THM_SAVOIR,FN_ARMEE,2, TECH_SAVOIR_GUERRE, false,false,UNIV,
    0,0,0, 0,2.0f, 0, 0, 1.0f, 0.50f, 1.5f, false,
    "permet de recruter le Sorcier · +14 % aux dégâts des mages (doctrine)",
    "Désormais, chaque cohorte marche avec son sorcier, et chaque sorcier avec deux gardes chargés de surveiller ses mains." },
[TECH_INVOCATION] = { "Invocation","Cercle d'invocation", THM_SAVOIR,FN_ARMEE,3, TECH_MAGIE_BATAILLE, true,true,HERITAGE_ESOTERIQUE,
    0,0,0, 0,2.0f, 0, 0, 3.0f, 1.50f, 3.0f, false,
    "⚠ armée invoquée, sans coût de population · +21 % aux dégâts des mages — rapproche la Brèche",
    "Les soldats apparurent sans mères, sans solde et sans sépulture. Cette nuit-là, même les vainqueurs verrouillèrent leurs portes." },
[TECH_EVEIL] = { "L'Éveil","Le Réveil (armée sans pop)", THM_SAVOIR,FN_ARMEE,4, TECH_MAGIE_BATAILLE, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 6.0f, 3.00f, 6.0f, true,
    "⚠ armée invoquée massive · +28 % aux dégâts des mages · DÉCLENCHE la crise de fin",
    "Nous pensions appeler une armée. Quelque chose, de l'autre côté, entendit plutôt prononcer son nom." },
/* ---- SAVOIR · RENFORCEMENT (arcane durable — faustien) --------------- */
[TECH_WARDS] = { "Gardes runiques","Gardes runiques (Wards)", THM_SAVOIR,FN_RENFORCEMENT,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0.5f,1.0f, 0,0, 0, 0, 0, 0, 0, false,
    "+stabilité (les runes tiennent l'ordre de la couronne) — aucune charge",
    "Les runes au-dessus des portes ne promettent pas la paix. Elles rappellent seulement à chacun le prix du désordre." },
[TECH_SCRYING] = { "Clairvoyance","Bassin de scrying", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_WARDS, false,false,UNIV,
    0.5f,0,0.5f, 0,0, 0, 0, 0.5f, 0.30f, 1.0f, false,
    "+0,5 prospérité (K) · +puissance brute (l'œil qui voit loin) · flux 0,3 · charge 1 — attise légèrement la Brèche",
    "Le miroir montra les frontières lointaines, puis la chambre derrière l'observateur. On le recouvrit trop tard." },
[TECH_COMMUNION] = { "Communion","Bosquet de communion", THM_SAVOIR,FN_RENFORCEMENT,3, TECH_SCRYING, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,2.0f, 0,0, 0, -1.0f, 0.5f, 0.10f, 0.5f, false,
    "+stabilité · −fracture interne (l'harmonie des peuples)",
    "Durant une heure, le prince entendit son peuple comme une seule voix. Il pleura avant d'ordonner que le rite recommence." },
[TECH_SAVOIR_INTERDIT] = { "Savoir interdit","Crypte interdite", THM_SAVOIR,FN_RENFORCEMENT,4, TECH_SCRYING, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 4.0f, 2.00f, 4.0f, false,
    "⚠ +puissance brute (le scellé rompu) · lourde charge faustienne — rapproche la Brèche",
    "Le sceau portait neuf avertissements et trois malédictions. Le Conseil prit cela pour une table des matières." },

/* ---- FORGE · PRODUCTION (sortie — le multiplicateur de rendement) ----- */
[TECH_COLLECTE_BOIS] = { "Sylviculture","Camp de bûcherons", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production de bois",
    "La forêt devint charpentes, palissades et foyers. Les anciens furent les seuls à demander ce qu'elle était auparavant." },
[TECH_COLLECTE_ARGILE] = { "Extraction d'argile","Carrière d'argile", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production d'argile",
    "Là où les paysans voyaient de la boue, le maître d'œuvre vit des murs, des fours et une ville entière." },
[TECH_FONDERIE] = { "Fonderie","Fonderie", THM_FORGE,FN_PRODUCTION,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 1.5f,0, 0, 0, 0, 0.05f, 0.3f, false,
    "+8 % de production globale (le travail du fer s'organise) · flux 0,05 · charge 0,3",
    "Le fer entra rouge dans la halle et en ressortit utile. Les hommes qui l'y poussèrent gardèrent la même couleur aux poumons." },
[TECH_OUTILLAGE] = { "Outillage","Atelier d'outillage", THM_FORGE,FN_PRODUCTION,2, TECH_FONDERIE, false,false,UNIV,
    0,0,0, 2.5f,0, 0, 0, 0, 0, 0, false,
    "+10 % de production globale (l'outil démultiplie le bras) · aucune charge — étape industrielle sûre",
    "Un bon outil épargne dix bras, affirma l'intendant. Il ne précisa jamais ce qu'il adviendrait de ces bras." },
[TECH_MANUFACTURE] = { "Manufacture","Manufacture", THM_FORGE,FN_PRODUCTION,3, TECH_OUTILLAGE, false,false,UNIV,
    0,0,0, 3.0f,0, 0, 1.0f, 0, 0.30f, 1.0f, false,
    "+12 % de production globale · légère fracture · flux 0,3 · charge 1 (cent artisans sous un toit)",
    "Cent artisans sous un même toit produisirent comme mille. Ils apprirent aussi à se plaindre d'une seule voix." },
[TECH_INDUSTRIE] = { "Industrie de masse","Complexe industriel", THM_FORGE,FN_PRODUCTION,4, TECH_MANUFACTURE, false,false,UNIV,
    0,0,0, 4.0f,2.0f, 0, 1.5f, 0, 1.00f, 3.0f, false,
    "+15 % de production globale · fracture notable · flux 1 · charge 3 — faustienne, rapproche la Brèche",
    "Les cheminées ne dorment plus. Dans leur ombre, le royaume prospère assez vite pour oublier ce qu'il consume." },
/* §B2 — FOREUSE ARCANIQUE (faustien) : transmute l'essence en FER en masse → l'issue à la
 * famine de matière pour l'empire enclavé/affamé — mais forte CHARGE + flux vers la Brèche. */
[TECH_FOREUSE] = { "Foreuse arcanique","Foreuse arcanique", THM_FORGE,FN_PRODUCTION,4, TECH_INDUSTRIE, true,false,UNIV,
    0,0,0, 3.0f,0, 0, 0, 1.0f, 1.50f, 4.0f, false,
    "⚠ transmute l'essence en fer en masse · +puissance brute · flux 1,5 · charge 4 — rapproche la Brèche",
    "La foreuse trouva du fer sous la pierre, puis une pulsation sous le fer. L'ingénieur ordonna de creuser plus droit." },
/* ---- FORGE · ARMÉE (armes — faustien) -------------------------------- */
[TECH_ARMURERIE] = { "Armurerie","Armurerie", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0, false,
    "permet la production d'armes · +5 % aux dégâts (doctrine) — aucune charge (arme civile)",
    "Le forgeron façonnait jadis des outils et parfois une épée. La couronne régla cette regrettable indécision." },
[TECH_POUDRIERE] = { "Poudrière","Poudrière", THM_FORGE,FN_ARMEE,2, TECH_ARMURERIE, false,false,UNIV,
    0,0,0, 0,2.5f, 0, 0, 0, 0.20f, 1.0f, false,
    "permet la poudre à canon & l'Arquebusier · +10 % aux dégâts (doctrine)",
    "Le premier tir dispersa les chevaux, les vitraux et les certitudes de trois générations de chevaliers." },
[TECH_FORGE_RUNES] = { "Forge à runes","Forge céleste", THM_FORGE,FN_ARMEE,3, TECH_POUDRIERE, true,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,3.0f, 0, 0, 3.0f, 1.00f, 2.0f, false,
    "⚠ permet les armes enchantées & la Garde runique · +15 % aux dégâts (doctrine) — rapproche la Brèche",
    "La lame chanta quand on grava le dernier signe. Aucun des présents ne reconnut la langue, mais tous comprirent la menace." },
[TECH_OEUVRE_NOIRE] = { "L'Œuvre noire","L'Œuvre noire", THM_FORGE,FN_ARMEE,4, TECH_POUDRIERE, true,false,UNIV,
    0,0,0, 2.0f,5.0f, 3.0f, 2.0f, 2.0f, 1.50f, 5.0f, false,
    "⚠ +20 % aux dégâts (doctrine) · +coercition · fracture et charge lourdes — rapproche la Brèche",
    "On ne forge pas cette arme : on la persuade. Et toute chose que l'on persuade finit par demander quelque chose en retour." },
/* ---- FORGE · RENFORCEMENT (durabilité / fortification) ---------------- */
[TECH_ATELIER] = { "Atelier de construction","Atelier de construction", THM_FORGE,FN_RENFORCEMENT,0, NONE, false,false,UNIV,
    0,0,0.5f, 0,0, 0, 0, 0, 0, 0, false,
    "permet de bâtir (chantiers de construction)",
    "Avant l'atelier, chacun bâtissait selon sa mémoire. Après lui, même les rêves reçurent des mesures et un contremaître." },
[TECH_QUALITE_MATERIAUX] = { "Taille de précision","Chantier (pierre de taille)", THM_FORGE,FN_RENFORCEMENT,1, TECH_ATELIER, false,false,UNIV,
    0,0,1.0f, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet l'arbalète à pavois (Arbalétrier lourd) · +5 % de production globale (la taille fine des matériaux)",
    "Le maître tailleur écarta neuf pierres avant d'en accepter une. Le trésorier, lui, compta surtout les neuf autres." },
/* RÈGLE 3 (2026-07-10) : Fortifications n'avait AUCUN levier vivant (dF/dMil morts, charge 0.2
 * = pur coût) — la promesse « +défense » mentait. dL +1.5 (stabilité : derrière les murs,
 * l'ordre tient), calibré entre FOI (t2, dL 3) et COMMUNION (t3, dL 1). Passe éditoriale
 * 2026-07-10 : la charge résiduelle 0.2 (non faustienne, sans levier) est retirée à son tour —
 * une tech civile sûre ne garde aucun coût caché. Re-baseline golden. */
[TECH_FORTIFICATIONS] = { "Fortifications","Forteresse → Citadelle", THM_FORGE,FN_RENFORCEMENT,2, TECH_QUALITE_MATERIAUX, false,false,UNIV,
    0,1.5f,1.5f, 0,1.0f, 0, 0, 0, 0, 0, false,
    "+stabilité (derrière les murs, l'ordre tient) — aucune charge",
    "Les murs rassurent ceux qui vivent derrière. Ils apprennent aussi aux gouvernants à ne plus regarder au-delà." },
[TECH_AUTOMATES] = { "Automates","Grand Engrenage (Golems)", THM_FORGE,FN_RENFORCEMENT,3, TECH_FORTIFICATIONS, true,false,HERITAGE_MECANISTE,
    0,0,0, 3.0f,3.0f, 0, 0, 1.0f, 1.00f, 2.0f, false,
    "⚠ +8 % de production globale · +puissance brute (golems au chantier et au rempart) · flux 1 · charge 2 — rapproche la Brèche",
    "Le golem porta seul la pierre que vingt hommes refusaient de soulever. Personne n'osa lui demander s'il voulait la poser." },

/* ---- SOCIÉTÉ · PRODUCTION (croissance / commerce / impôt — sûre) ------ */
[TECH_COLLECTE_NOURRITURE] = { "Subsistances","Collecte (liée au biome)", THM_SOCIETE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false,
    "permet la production de nourriture (liée au biome)",
    "Le royaume apprit enfin à nommer tout ce qui pouvait nourrir un homme, et à taxer ceux qui le ramassaient." },
[TECH_IRRIGATION] = { "Irrigation & greniers","Greniers communs", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0.5f, 1.0f,0, 0, -0.5f, 0, 0, 0, false,
    "+6 % de production globale (greniers & canaux) · −fracture",
    "Nous avons commandé à l'eau de suivre nos sillons. Elle obéit, mais conserva la mémoire de ses anciennes rives." },
[TECH_COMMERCE] = { "Commerce","Marché → Banque", THM_SOCIETE,FN_PRODUCTION,2, TECH_IRRIGATION, false,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 0, 0, 0, false,
    "+8 % de production globale (le négoce s'organise — marché, banque)",
    "Le marchand franchit la frontière avec du sel et revint avec de l'or, trois rumeurs et l'oreille d'un magistrat." },
[TECH_CADASTRE] = { "Cadastre","Cadastre (impôt)", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,UNIV,
    0,0.5f,0, 1.5f,0, 0, 0, 0, 0, 0, false,
    "+8 % de production globale · +stabilité (l'impôt sait où prendre)",
    "Le géomètre traça une ligne sur sa carte. Au matin, deux familles apprirent qu'elle passait au milieu de leur maison." },
[TECH_ABONDANCE] = { "Abondance","Grenier d'abondance", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 3.0f,0, 0, -0.5f, 0, 0, 0, false,
    "+10 % de production globale · +stabilité · −fracture (l'abondance agraire)",
    "Les greniers débordaient et les enfants ne comptaient plus les jours avant la moisson. Ce fut notre plus douce victoire." },
/* E2 §13 — la branche MARCHANDE : le Comptoir branche la province au Centre
 * commercial (marge de transport réduite) ; les Halles ouvrent l'Entrepôt
 * (+500 de cap de stock chacun) — le jeu de marché (stocker bas, vendre haut). */
[TECH_COMPTOIRS] = { "Comptoirs marchands","Comptoir", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0, 0.8f,0, 0, 0, 0, 0, 0, false,
    "permet le Comptoir (branche au marché mondial) · permet les mercenaires (Lame franche, Cav. de raid)",
    "Là où flotte notre enseigne, nos lois hésitent encore, mais nos poids, nos contrats et nos créanciers sont déjà souverains." },
[TECH_HALLES] = { "Halles & entrepôts","Entrepôt (cap de stock)", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMPTOIRS, false,false,UNIV,
    0,0,0, 1.2f,0, 0, 0, 0, 0, 0, false,
    "permet l'Entrepôt (+500 de capacité de stock chacun — stocker bas, vendre haut)",
    "Ce qui dort dans un entrepôt ne nourrit personne aujourd'hui. C'est précisément pourquoi il vaudra davantage demain." },
/* ---- SOCIÉTÉ · ARMÉE (levée — faustien : l'esclavage) ---------------- */
[TECH_CASERNE] = { "Caserne","Caserne", THM_SOCIETE,FN_ARMEE,0, NONE, false,false,UNIV,
    0,0,0, 0,0.5f, 0, 0, 0, 0, 0, false,
    "permet de recruter l'infanterie (dont le Hallebardier)",
    "On fit entrer des paysans par une porte et sortir des soldats par l'autre. Entre les deux, ils apprirent à marcher ensemble." },
[TECH_CONSCRIPTION] = { "Conscription","Levée / Conscription", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0, false,
    "+5 % au moral (doctrine) · permet le Berserker & le décret de levée permanente",
    "Le décret disait que chaque foyer devait un défenseur. Dans les villages, on comprit qu'il exigeait surtout un fils." },
[TECH_ORGANISATION] = { "Organisation militaire","État-major", THM_SOCIETE,FN_ARMEE,2, TECH_CONSCRIPTION, false,false,UNIV,
    0,0,0.5f, 0,2.0f, 0, 0, 0, 0, 0, false,
    "+10 % au moral (doctrine) · permet le Lancier de choc & la Garde d'escorte",
    "Une armée courageuse gagne parfois. Une armée qui sait où dormir, manger et frapper gagne plus souvent." },
[TECH_ESCLAVAGE] = { "Économie servile","Marché aux esclaves", THM_SOCIETE,FN_ARMEE,3, TECH_ORGANISATION, true,false,HERITAGE_CLANIQUE,
    0,0,0, 3.0f,2.0f, 1.0f, 3.0f, 0, 0, 2.0f, false,
    "⚠ ouvre la traite servile · +15 % au moral (doctrine) · +coercition · forte fracture interne",
    "Les comptes étaient irréprochables : tant de chaînes, tant de bras, tant de profit. Seuls les noms avaient disparu des colonnes." },
[TECH_CASTE_MARTIALE] = { "Caste martiale","Caste martiale", THM_SOCIETE,FN_ARMEE,4, TECH_ORGANISATION, true,false,UNIV,
    0,0,0, 0,4.0f, 2.0f, 2.0f, 0, 0, 2.5f, false,
    "⚠ +20 % au moral (doctrine) · permet la Cav. cuirassée · +coercition — rapproche la Brèche",
    "Ils naissent pour commander, proclama le héraut. Ceux qui naissaient pour obéir n'avaient pas de héraut." },
/* ---- SOCIÉTÉ · RENFORCEMENT (K / L / intégration — la spine résiliente) */
[TECH_CHANCELLERIE] = { "Chancellerie","Tribunal / Chancellerie", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    3.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,
    "+prospérité (l'administration porte les services)",
    "Chaque requête trouva désormais un bureau, un sceau et un délai. Le peuple appela cela l'ordre, faute d'un mot plus patient." },
[TECH_FOI] = { "Foi","Temple → Cathédrale", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_CHANCELLERIE, false,false,UNIV,
    0,3.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+stabilité (la foi tient l'ordre — temple, cathédrale)",
    "Quand les récoltes manquèrent, le temple ne fit pas pleuvoir. Il empêcha toutefois les voisins de s'entre-tuer avant l'hiver." },
[TECH_INTEGRATION] = { "Droit d'intégration","Creuset (assimilation)", THM_SOCIETE,FN_RENFORCEMENT,3, TECH_FOI, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, -3.0f, 0, 0, 0, false,
    "+stabilité · forte baisse de fracture (le creuset assimile)",
    "La loi déclara tous les peuples égaux devant la couronne. Il fallut ensuite des générations pour apprendre à les regarder ainsi." },
[TECH_CULTE_IMPERIAL] = { "Culte impérial","Mythe homogénéisant", THM_SOCIETE,FN_RENFORCEMENT,4, TECH_FOI, true,false,UNIV,
    1.0f,2.0f,0, 0,0, 0, -2.0f, 0, 0.50f, 3.0f, false,
    "⚠ cohésion forcée : +prospérité & +stabilité · −fracture — rapproche la Brèche",
    "On plaça le portrait du souverain sur chaque autel. Bientôt, nul ne sut plus si l'on priait pour lui ou devant lui." },
/* F3 — ALCHIMIE (gate de l'Alambic) : la distillation du salpêtre en FLUX + nécessaire
 * d'alchimiste. NON-faustienne — c'est la SUPPLY bénigne ; la charge faustienne vivra sur
 * les TRANSMUTEURS (FAU2) et leur gate dédié (FAU4). Tier 2, peu profonde (atteignable). */
[TECH_ALCHIMIE] = { "Alchimie","Alambic", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMMERCE, false,false,UNIV,
    0,0,0, 1.0f,0, 0, 0, 0, 0.10f, 0, false,
    "permet l'Alambic (salpêtre → flux) & l'Alchimiste · +5 % de production globale — non faustienne",
    "L'alchimiste ne promit pas l'or. Il promit seulement qu'un baril banal pouvait devenir le souffle d'une armée." },
/* FAU4 — TRANSMUTATION (FAUSTIENNE, gate du Réplicateur ligneux : flux → bois). Profonde
 * (tier 3, derrière l'Alchimie) → charge de base élevée (paroxysme = pression de Brèche). */
[TECH_TRANSMUTATION] = { "Transmutation","Réplicateur ligneux", THM_SOCIETE,FN_PRODUCTION,3, TECH_ALCHIMIE, true,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 1.0f, 0.30f, 1.2f, false,
    "⚠ permet le Réplicateur ligneux (flux → bois) · +puissance brute — rapproche la Brèche",
    "Le premier arbre sortit du réplicateur avec ses feuilles, ses anneaux et aucun souvenir du vent." },

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
    "+stabilité · +puissance brute (glyphes protecteurs) · charge 0,1",
    "Les signes luisaient faiblement sur les remparts. Les sentinelles dormaient mieux sans savoir ce qu'ils tenaient dehors." },
[TECH_COMMUNION_ETHEREE] = { "Communion éthérée","Bastion éthéré", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_GLYPHES_ETHERES, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0.6f, 0.15f, 0.40f, false,
    "+stabilité · +puissance brute (l'harmonie éthérée) · flux 0,15 · charge 0,4",
    "La cité partagea un même rêve. Au réveil, les querelles semblaient mesquines, sauf celle portant sur l'auteur du rêve." },
/* ---- Métallurgiste (Forge·Armée → FORGE_RUNES) : +production ----------- */
[TECH_ALLIAGES_NAINS] = { "Alliages des profondeurs","Fonderie de bronze", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_ARGILE, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+5 % aux dégâts (doctrine) · légère stabilité — métallurgie de fond",
    "Le métal des profondeurs plia sans rompre. Le maître nain, lui, ne plia pas même devant les remerciements du roi." },
[TECH_GRAVURE_RUNES] = { "Gravure runique","Rune-forge", THM_FORGE,FN_ARMEE,2, TECH_ALLIAGES_NAINS, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.50f, false,
    "+10 % aux dégâts (doctrine) · +puissance brute (runes) · flux 0,1 · charge 0,5",
    "Une rune bien gravée survit au bras qui la porte. C'est pourquoi les forgerons choisissent leurs mots avec tant de soin." },
/* ---- Mécaniste (Forge·Prod→Renf → AUTOMATES) : +production/+efficacité - */
[TECH_MECANISTE_ROUAGES] = { "Rouages de précision","Engrenagerie", THM_FORGE,FN_PRODUCTION,1, TECH_FONDERIE, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0, 0.05f, 0.20f, false,
    "+6 % de production globale (mécanique de précision) · +prospérité & stabilité · flux 0,05 · charge 0,2",
    "Le mécaniste posa six roues dentées sur la table. Au septième matin, la halle travaillait au rythme de son invention." },
[TECH_MECANISTE_HORLOGERIE] = { "Horlogerie civique","Horloge mécanique", THM_FORGE,FN_RENFORCEMENT,2, TECH_MECANISTE_ROUAGES, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.30f, false,
    "+8 % d'efficacité générale (la journée synchronisée) · +puissance brute · +prospérité & stabilité · flux 0,1 · charge 0,3",
    "Les cloches synchronisèrent ateliers, marchés et tribunaux. Le temps, autrefois commun à tous, appartint désormais à l'administration." },
/* ---- Adaptatif (Société·Renforcement → INTEGRATION) : +cohésion -------- */
[TECH_DROIT_COUTUMIER] = { "Droit coutumier","Code coutumier", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_CHANCELLERIE, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+stabilité (droit coutumier, le compromis codifié)",
    "Le juge demanda ce que faisaient les anciens. Puis il l'écrivit, afin que les anciens eux-mêmes ne puissent plus en changer." },
[TECH_LANGUE_FRANQUE] = { "Langue franque","Lingua franca", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_DROIT_COUTUMIER, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, -1.0f, 0, 0, 0, false,
    "+prospérité & +stabilité · −fracture (la langue commune)",
    "Le marchand, le soldat et le magistrat partagèrent enfin les mêmes mots. Ils purent donc se quereller avec une parfaite précision." },
/* ---- Agraire (Société·Production → ABONDANCE) : +production agricole --- */
[TECH_VERGERS_ETAGES] = { "Vergers étagés","Vergers en terrasses", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+5 % de production globale (vergers en terrasses) · légère stabilité",
    "La montagne refusait les champs. Les cultivateurs lui taillèrent des marches et plantèrent des printemps à chaque degré." },
[TECH_PATURAGES_INTEGRES] = { "Pâturages intégrés","Prairies-vergers", THM_SOCIETE,FN_PRODUCTION,2, TECH_VERGERS_ETAGES, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,
    "+7 % de production globale (polyculture) · +stabilité",
    "Bêtes, vergers et céréales cessèrent de se disputer la terre. Les propriétaires, eux, poursuivirent la querelle." },
/* ---- Clanique (Société·Armée → ESCLAVAGE) : +stabilité / +production --- */
[TECH_RITES_GUERRIERS] = { "Rites guerriers","Sanctuaire des ancêtres", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,HERITAGE_CLANIQUE,
    0,0.3f,0, 0,0, 0.2f, 0, 0, 0, 0, false,
    "+5 % au moral (doctrine) · légère coercition",
    "Avant la bataille, ils frappent leurs boucliers et récitent les noms des morts. Après, les survivants ajoutent les nouveaux." },
[TECH_HORDES_CONQUERANTES] = { "Hordes conquérantes","Camps de rapine", THM_SOCIETE,FN_ARMEE,2, TECH_RITES_GUERRIERS, false,false,HERITAGE_CLANIQUE,
    0,0.5f,0, 0,0, 0.3f, 0.5f, 0, 0, 0, false,
    "+10 % au moral (doctrine) · +coercition · fracture interne en hausse",
    "Ils ne demandent pas où mène la route, seulement qui prétend la fermer. Le reste se décide au galop." },

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
    "Le mécaniste régla la détente, le métallurgiste le canon. À cent pas, un héritage chevaleresque prit fin sans cérémonie." },
[TECH_COMBO_AUTOMATES_ARC] = { "Automates arcanes","Golems d'essence", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 1.0f, 0, 0, false,         /* Éso×Méca : +production globale (prod%) + puissance */
    "exige l'accès plein Ésotérique+Mécaniste · +10 % de production globale · +puissance brute",
    "Le mage donna l'ordre, le mécaniste lui donna des jambes. Depuis, la machine travaille même lorsque tous deux dorment." },
[TECH_COMBO_ACADEMIE] = { "Académie cosmopolite","Grande académie", THM_SAVOIR,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    1.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Adaptatif : +recherche (Savoir·Prod) + efficacité générale */
    "exige l'accès plein Ésotérique+Adaptatif · +recherche (spine) & +prospérité · +8 % d'efficacité générale (toute production)",
    "Sous ses voûtes, chaque peuple apporta une méthode et repartit avec les questions des autres." },
[TECH_COMBO_DRUIDE] = { "Abondance druidique","Bosquet nourricier", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Agraire : +production globale (prod%) + stabilité */
    "exige l'accès plein Ésotérique+Agraire · +8 % de production globale · +stabilité",
    "Les druides enseignèrent aux canaux à contourner les racines. La moisson fut si riche que même les sceptiques offrirent des graines." },
[TECH_COMBO_CHAMAN] = { "Chamanisme de guerre","Cercle des chamans", THM_SAVOIR,FN_ARMEE,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Éso×Clanique : +magie de guerre via doctrine (arcane_power) */
    "exige l'accès plein Ésotérique+Clanique · +puissance arcane des armées (doctrine, tier 4)",
    "Le chamane peignit les guerriers de signes mouvants. L'ennemi vit une horde ; la horde se sentit légion." },
[TECH_COMBO_GUILDES] = { "Guildes maîtresses","Hôtel des guildes", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Métal×Adaptatif : +production globale (prod%) + or/stabilité */
    "exige l'accès plein Métallurgiste+Adaptatif · +8 % de production globale · +stabilité",
    "Les maîtres jurèrent sur le métal et le contrat. Dès lors, un ouvrage médiocre devint une faute contre toute la guilde." },
[TECH_COMBO_CHARRUES] = { "Charrues lourdes","Atelier de charronnage", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Métal×Agraire : +production globale (prod%) */
    "exige l'accès plein Métallurgiste+Agraire · +8 % de production globale",
    "La nouvelle charrue ouvrit la terre comme une porte. Derrière elle vinrent les récoltes, les colons et les collecteurs." },
[TECH_COMBO_POLIORCETIQUE] = { "Poliorcétique","Forge de guerre", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Métal×Clanique : +dégâts via doctrine */
    "exige l'accès plein Métallurgiste+Clanique · +dégâts (doctrine, tier 4)",
    "Les murailles avaient été bâties pour arrêter des hommes. Nos ingénieurs arrivèrent avec des mathématiques." },
[TECH_COMBO_HORLOGE_MARCH] = { "Horlogerie marchande","Comptoir mécanique", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Méca×Adaptatif : +efficacité générale (eff%) + or/stabilité */
    "exige l'accès plein Mécaniste+Adaptatif · +10 % d'efficacité générale · +stabilité",
    "À midi précis, les portes s'ouvrent, les prix changent et les caravanes repartent. Même le hasard respecte désormais les horaires." },
[TECH_COMBO_MACHINES_AGRI] = { "Machines agricoles","Moulins & semoirs", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Méca×Agraire : +production globale (prod%, fort) */
    "exige l'accès plein Mécaniste+Agraire · +12 % de production globale",
    "La machine ne connaît ni fatigue ni saison. Le paysan apprit à la craindre un peu avant d'apprendre à l'aimer." },
[TECH_COMBO_SIEGE] = { "Engins de siège","Arsenal de siège", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,           /* Méca×Clanique : +dégâts via doctrine */
    "exige l'accès plein Mécaniste+Clanique · +dégâts (doctrine, tier 4)",
    "Le clan apporta la force, le mécaniste la portée. La première pierre franchit un rempart que nul guerrier n'avait dépassé." },
[TECH_COMBO_GRENIER_COLON] = { "Grenier colonial","Comptoir-grenier", THM_SOCIETE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, 0, 0, 0, 0, false,      /* Adaptatif×Agraire : +stabilité & +croissance (K,L) */
    "exige l'accès plein Adaptatif+Agraire · +prospérité & +stabilité (la croissance suit)",
    "Chaque nouvelle frontière reçut d'abord un grenier. Les maisons, les lois et les querelles suivirent naturellement." },
[TECH_COMBO_FOEDERATI] = { "Fédérés","Camp fédéré", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_ADAPTATIF,
    0,0.5f,0, 0,0, 0, -0.5f, 0, 0, 0, false,     /* Adaptatif×Clanique : +moral via doctrine + cohésion */
    "exige l'accès plein Adaptatif+Clanique · +moral (doctrine, tier 4) · −fracture",
    "Ils conservent leurs bannières, leurs chefs et leurs chants. En bataille, pourtant, ils tiennent désormais notre ligne." },
[TECH_COMBO_HORDE_ECO] = { "Économie de horde","Halle du butin", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false,         /* Agraire×Clanique : +moral via doctrine + production globale (prod%) */
    "exige l'accès plein Agraire+Clanique · +moral (doctrine, tier 4) · +6 % de production globale",
    "Le troupeau nourrit le camp, le camp protège le troupeau. Tout le reste doit suivre, ou être laissé derrière." },

/* ====================================================================== */
/* APEX TRIPLES (2026-06-28) — TIER-5 : la fusion de TROIS héritages (accès PLEIN aux 3).   */
/* Le pinacle. N=3 via tech_combo_native (2e) + tech_combo_native2 (3e). prereq=NONE (la    */
/* porte = la triple-métabolisation + le coût tier-5). Effets sur leviers vivants.          */
/* ====================================================================== */
[TECH_APEX_ARQUEBUSE] = { "Arquebuse runique","Arsenal runique à feu", THM_FORGE,FN_ARMEE,5, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false,            /* Méca×Métal×Éso : +dégâts (doctrine) + ARQUEBUSIERS ciblés (firearm_power) */
    "exige l'accès plein Mécaniste+Métallurgiste+Ésotérique · +dégâts (doctrine, tier 5) · +50 % de dégâts ARQUEBUSIERS (ciblé)",
    "La poudre projette le plomb ; la rune choisit sa destination. Le tireur n'a plus qu'à vivre avec le résultat." },
[TECH_APEX_CONCILE] = { "Concile des savants","Grand concile", THM_SAVOIR,FN_PRODUCTION,5, NONE, false,false,HERITAGE_ESOTERIQUE,
    1.0f,0,0, 0,0, 0, 0, 0, 0, 0, false,         /* Éso×Adaptatif×Méca : +recherche (Savoir·Prod) + efficacité générale */
    "exige l'accès plein Ésotérique+Adaptatif+Mécaniste · +recherche (spine) & +prospérité · +12 % d'efficacité générale (toute production)",
    "Aucun savant ne possède toute la réponse. Réunis, ils possèdent toutefois assez de désaccords pour la découvrir." },
[TECH_APEX_LEGION] = { "Légion universelle","Camp des nations", THM_SOCIETE,FN_ARMEE,5, NONE, false,false,HERITAGE_ADAPTATIF,
    0,0.5f,0, 0,0, 0, -0.5f, 0, 0, 0, false,     /* Adaptatif×Métal×Clanique : +moral (doctrine) + cohésion */
    "exige l'accès plein Adaptatif+Métallurgiste+Clanique · +moral (doctrine, tier 5) · +stabilité · −fracture",
    "Chaque peuple combat à sa manière, sous un même commandement. L'ennemi doit donc vaincre toutes nos histoires à la fois." },
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
    /* Forge·Renforcement — taille de précision + golems (passe éditoriale 2026-07-10, ex-muets). */
    [TECH_QUALITE_MATERIAUX]=0.05f, [TECH_AUTOMATES]=0.08f,
    /* Société·Production — rendement agricole / efficacité du commerce / alchimie (idem, 2026-07-10). */
    [TECH_IRRIGATION]=0.06f, [TECH_COMMERCE]=0.08f, [TECH_CADASTRE]=0.08f, [TECH_ABONDANCE]=0.10f,
    [TECH_ALCHIMIE]=0.05f,
    /* ÉTOFFE — rungs d'héritage à VOCATION PRODUCTION (les FN_ARMÉE vont à army_doctrine). */
    [TECH_MECANISTE_ROUAGES]=0.06f,
    [TECH_VERGERS_ETAGES]=0.05f, [TECH_PATURAGES_INTEGRES]=0.07f,
    /* COMBOS tier-4 à vocation production (le +production CONCRET de la fusion). */
    [TECH_COMBO_AUTOMATES_ARC]=0.10f, [TECH_COMBO_DRUIDE]=0.08f,
    [TECH_COMBO_GUILDES]=0.08f, [TECH_COMBO_CHARRUES]=0.08f,
    [TECH_COMBO_MACHINES_AGRI]=0.12f, [TECH_COMBO_HORDE_ECO]=0.06f,
};
static float NODE_EFF_PCT[TECH_COUNT] = {   /* NON-const (MODTOOLS) */
    /* Passe éditoriale 2026-07-10 : Scriptorium/Académie/Université PERDENT leur +eff% caché
     * (jamais annoncé au hover de toute façon) — Savoir = recherche+K, pas la meilleure branche
     * de production en plus (doc RETOURS_2026-07-10 §TECH). */
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
/* le bonus PAR NŒUD — pour le hover chiffré de la façade (« il faut spécifier combien »). */
float tech_node_prod_pct(TechId id){ return ((int)id>=0 && id<TECH_COUNT) ? NODE_PROD_PCT[id] : 0.f; }
float tech_node_eff_pct (TechId id){ return ((int)id>=0 && id<TECH_COUNT) ? NODE_EFF_PCT[id]  : 0.f; }

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
/* LOT T — voir scps_tech.h. Balaie les TECH_COUNT nœuds (petite table, appel peu
 * fréquent — au tick de cache d'econ_apply_country_tech, pas par-province). */
bool tech_has_tier(const TechState *s, int tier){
    if (tier<=0) return true;
    if (!s) return true;
    for (int i=0;i<TECH_COUNT;i++) if (s->unlocked[i] && NODES[i].tier==tier) return true;
    return false;
}

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
