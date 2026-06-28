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
static const float BASE_COST[6] = { 0.f, 40.f, 90.f, 160.f, 260.f, 400.f }; /* par tier (rayon) */

/* ====================================================================== */
/* TABLE DES NŒUDS — 9 quartiers (angle), tier (rayon)                     */
/* Champs : name, unlocks, theme, func, tier, prereq, faustian, needs_ruins,
 *          native, dK,dL,dF, dEco,dMil, dH, dFracture, dPuissance, flux,
 *          charge, triggers_crisis                                        */
/* ====================================================================== */
static const TechNode NODES[TECH_COUNT] = {
/* ---- SAVOIR · PRODUCTION (spine sûre : vitesse de recherche, +K) ------ */
[TECH_BIBLIOTHEQUE] = { "Bibliothèque","Bibliothèque", THM_SAVOIR,FN_PRODUCTION,0, NONE, false,false,UNIV,
    1,0,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_SCRIPTORIUM] = { "Scriptorium","Scriptorium", THM_SAVOIR,FN_PRODUCTION,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    1,0,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_ACADEMIE] = { "Académie","Académie", THM_SAVOIR,FN_PRODUCTION,2, TECH_SCRIPTORIUM, false,false,UNIV,
    2,0,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_UNIVERSITE] = { "Université","Université", THM_SAVOIR,FN_PRODUCTION,3, TECH_ACADEMIE, false,false,UNIV,
    3,0,0, 0,0, 0, 0, 0, 0, 0, false },
/* ---- SAVOIR · ARMÉE (arcane offensif — faustien) --------------------- */
[TECH_SAVOIR_GUERRE] = { "Savoir de guerre","Collège de guerre", THM_SAVOIR,FN_ARMEE,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0,0, 0,1.0f, 0, 0, 0, 0.05f, 0.3f, false },
[TECH_MAGIE_BATAILLE] = { "Magie de bataille","Tour de mages", THM_SAVOIR,FN_ARMEE,2, TECH_SAVOIR_GUERRE, false,false,UNIV,
    0,0,0, 0,2.0f, 0, 0, 1.0f, 0.50f, 1.5f, false },
[TECH_INVOCATION] = { "Invocation","Cercle d'invocation", THM_SAVOIR,FN_ARMEE,3, TECH_MAGIE_BATAILLE, true,true,HERITAGE_ESOTERIQUE,
    0,0,0, 0,2.0f, 0, 0, 3.0f, 1.50f, 3.0f, false },
[TECH_EVEIL] = { "L'Éveil","Le Réveil (armée sans pop)", THM_SAVOIR,FN_ARMEE,4, TECH_MAGIE_BATAILLE, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 6.0f, 3.00f, 6.0f, true },
/* ---- SAVOIR · RENFORCEMENT (arcane durable — faustien) --------------- */
[TECH_WARDS] = { "Gardes runiques","Gardes runiques (Wards)", THM_SAVOIR,FN_RENFORCEMENT,1, TECH_BIBLIOTHEQUE, false,false,UNIV,
    0,0.5f,1.0f, 0,0, 0, 0, 0, 0, 0.3f, false },
[TECH_SCRYING] = { "Scrying","Bassin de scrying", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_WARDS, false,false,UNIV,
    0,0,0.5f, 0,0, 0, 0, 0.5f, 0.30f, 1.0f, false },
[TECH_COMMUNION] = { "Communion","Bosquet de communion", THM_SAVOIR,FN_RENFORCEMENT,3, TECH_SCRYING, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,2.0f, 0,0, 0, -1.0f, 0.5f, 0.10f, 0.5f, false },
[TECH_SAVOIR_INTERDIT] = { "Savoir interdit","Crypte interdite", THM_SAVOIR,FN_RENFORCEMENT,4, TECH_SCRYING, true,true,UNIV,
    0,0,0, 0,0, 0, 0, 4.0f, 2.00f, 4.0f, false },

/* ---- FORGE · PRODUCTION (sortie — le multiplicateur de rendement) ----- */
[TECH_COLLECTE_BOIS] = { "Collecte de bois","Camp de bûcherons", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false },
[TECH_COLLECTE_ARGILE] = { "Collecte d'argile","Carrière d'argile", THM_FORGE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false },
[TECH_FONDERIE] = { "Fonderie","Fonderie", THM_FORGE,FN_PRODUCTION,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 1.5f,0, 0, 0, 0, 0.05f, 0.3f, false },
[TECH_OUTILLAGE] = { "Outillage","Atelier d'outillage", THM_FORGE,FN_PRODUCTION,2, TECH_FONDERIE, false,false,UNIV,
    0,0,0, 2.5f,0, 0, 0, 0, 0.05f, 0.3f, false },
[TECH_MANUFACTURE] = { "Manufacture","Manufacture", THM_FORGE,FN_PRODUCTION,3, TECH_OUTILLAGE, false,false,UNIV,
    0,0,0, 3.0f,0, 0, 1.0f, 0, 0.30f, 1.0f, false },
[TECH_INDUSTRIE] = { "Industrie de masse","Complexe industriel", THM_FORGE,FN_PRODUCTION,4, TECH_MANUFACTURE, false,false,UNIV,
    0,0,0, 4.0f,2.0f, 0, 1.5f, 0, 1.00f, 3.0f, false },
/* §B2 — FOREUSE ARCANIQUE (faustien) : transmute l'essence en FER en masse → l'issue à la
 * famine de matière pour l'empire enclavé/affamé — mais forte CHARGE + flux vers la Brèche. */
[TECH_FOREUSE] = { "Foreuse arcanique","Foreuse arcanique", THM_FORGE,FN_PRODUCTION,4, TECH_INDUSTRIE, true,false,UNIV,
    0,0,0, 3.0f,0, 0, 0, 1.0f, 1.50f, 4.0f, false },
/* ---- FORGE · ARMÉE (armes — faustien) -------------------------------- */
[TECH_ARMURERIE] = { "Armurerie","Armurerie", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_BOIS, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0.3f, false },
[TECH_POUDRIERE] = { "Poudrière","Poudrière", THM_FORGE,FN_ARMEE,2, TECH_ARMURERIE, false,false,UNIV,
    0,0,0, 0,2.5f, 0, 0, 0, 0.20f, 1.0f, false },
[TECH_FORGE_RUNES] = { "Forge à runes","Forge céleste", THM_FORGE,FN_ARMEE,3, TECH_POUDRIERE, true,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,3.0f, 0, 0, 3.0f, 1.00f, 2.0f, false },
[TECH_OEUVRE_NOIRE] = { "L'Œuvre noire","L'Œuvre noire", THM_FORGE,FN_ARMEE,4, TECH_POUDRIERE, true,false,UNIV,
    0,0,0, 2.0f,5.0f, 3.0f, 2.0f, 2.0f, 1.50f, 5.0f, false },
/* ---- FORGE · RENFORCEMENT (durabilité / fortification) ---------------- */
[TECH_ATELIER] = { "Atelier de construction","Atelier de construction", THM_FORGE,FN_RENFORCEMENT,0, NONE, false,false,UNIV,
    0,0,0.5f, 0,0, 0, 0, 0, 0, 0, false },
[TECH_QUALITE_MATERIAUX] = { "Qualité des matériaux","Chantier (béton→marbre)", THM_FORGE,FN_RENFORCEMENT,1, TECH_ATELIER, false,false,UNIV,
    0,0,1.0f, 0.5f,0, 0, 0, 0, 0, 0, false },
[TECH_FORTIFICATIONS] = { "Fortifications","Forteresse → Citadelle", THM_FORGE,FN_RENFORCEMENT,2, TECH_QUALITE_MATERIAUX, false,false,UNIV,
    0,0,1.5f, 0,1.0f, 0, 0, 0, 0, 0.2f, false },
[TECH_AUTOMATES] = { "Automates","Grand Engrenage (Golems)", THM_FORGE,FN_RENFORCEMENT,3, TECH_FORTIFICATIONS, true,false,HERITAGE_MECANISTE,
    0,0,0, 3.0f,3.0f, 0, 0, 1.0f, 1.00f, 2.0f, false },

/* ---- SOCIÉTÉ · PRODUCTION (croissance / commerce / impôt — sûre) ------ */
[TECH_COLLECTE_NOURRITURE] = { "Collecte de nourriture","Collecte (liée au biome)", THM_SOCIETE,FN_PRODUCTION,0, NONE, false,false,UNIV,
    0,0,0, 0.5f,0, 0, 0, 0, 0, 0, false },
[TECH_IRRIGATION] = { "Irrigation & greniers","Greniers communs", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0.5f, 1.0f,0, 0, -0.5f, 0, 0, 0, false },
[TECH_COMMERCE] = { "Commerce","Marché → Banque", THM_SOCIETE,FN_PRODUCTION,2, TECH_IRRIGATION, false,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 0, 0, 0, false },
[TECH_CADASTRE] = { "Cadastre","Cadastre (impôt)", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,UNIV,
    0,0.5f,0, 1.5f,0, 0, 0, 0, 0, 0, false },
[TECH_ABONDANCE] = { "Abondance","Grenier d'abondance", THM_SOCIETE,FN_PRODUCTION,3, TECH_COMMERCE, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 3.0f,0, 0, -0.5f, 0, 0, 0, false },
/* E2 §13 — la branche MARCHANDE : le Comptoir branche la province au Centre
 * commercial (marge de transport réduite) ; les Halles ouvrent l'Entrepôt
 * (+500 de cap de stock chacun) — le jeu de marché (stocker bas, vendre haut). */
[TECH_COMPTOIRS] = { "Comptoirs marchands","Comptoir", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    0,0,0, 0.8f,0, 0, 0, 0, 0, 0, false },
[TECH_HALLES] = { "Halles & entrepôts","Entrepôt (cap de stock)", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMPTOIRS, false,false,UNIV,
    0,0,0, 1.2f,0, 0, 0, 0, 0, 0, false },
/* ---- SOCIÉTÉ · ARMÉE (levée — faustien : l'esclavage) ---------------- */
[TECH_CASERNE] = { "Caserne","Caserne", THM_SOCIETE,FN_ARMEE,0, NONE, false,false,UNIV,
    0,0,0, 0,0.5f, 0, 0, 0, 0, 0, false },
[TECH_CONSCRIPTION] = { "Conscription","Levée / Conscription", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,UNIV,
    0,0,0, 0,1.5f, 0, 0, 0, 0, 0, false },
[TECH_ORGANISATION] = { "Organisation militaire","État-major", THM_SOCIETE,FN_ARMEE,2, TECH_CONSCRIPTION, false,false,UNIV,
    0,0,0.5f, 0,2.0f, 0, 0, 0, 0, 0, false },
[TECH_ESCLAVAGE] = { "Économie servile","Marché aux esclaves", THM_SOCIETE,FN_ARMEE,3, TECH_ORGANISATION, true,false,HERITAGE_CLANIQUE,
    0,0,0, 3.0f,2.0f, 1.0f, 3.0f, 0, 0, 2.0f, false },
[TECH_CASTE_MARTIALE] = { "Caste martiale","Caste martiale", THM_SOCIETE,FN_ARMEE,4, TECH_ORGANISATION, true,false,UNIV,
    0,0,0, 0,4.0f, 2.0f, 2.0f, 0, 0, 2.5f, false },
/* ---- SOCIÉTÉ · RENFORCEMENT (K / L / intégration — la spine résiliente) */
[TECH_CHANCELLERIE] = { "Chancellerie","Tribunal / Chancellerie", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_COLLECTE_NOURRITURE, false,false,UNIV,
    3.0f,0,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_FOI] = { "Foi","Temple → Cathédrale", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_CHANCELLERIE, false,false,UNIV,
    0,3.0f,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_INTEGRATION] = { "Droit d'intégration","Creuset (assimilation)", THM_SOCIETE,FN_RENFORCEMENT,3, TECH_FOI, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, -3.0f, 0, 0, 0, false },
[TECH_CULTE_IMPERIAL] = { "Culte impérial","Mythe homogénéisant", THM_SOCIETE,FN_RENFORCEMENT,4, TECH_FOI, true,false,UNIV,
    1.0f,2.0f,0, 0,0, 0, -2.0f, 0, 0.50f, 3.0f, false },
/* F3 — ALCHIMIE (gate de l'Alambic) : la distillation du salpêtre en FLUX + nécessaire
 * d'alchimiste. NON-faustienne — c'est la SUPPLY bénigne ; la charge faustienne vivra sur
 * les TRANSMUTEURS (FAU2) et leur gate dédié (FAU4). Tier 2, peu profonde (atteignable). */
[TECH_ALCHIMIE] = { "Alchimie","Alambic", THM_SOCIETE,FN_PRODUCTION,2, TECH_COMMERCE, false,false,UNIV,
    0,0,0, 1.0f,0, 0, 0, 0, 0.10f, 0, false },
/* FAU4 — TRANSMUTATION (FAUSTIENNE, gate du Réplicateur ligneux : flux → bois). Profonde
 * (tier 3, derrière l'Alchimie) → charge de base élevée (paroxysme = pression de Brèche). */
[TECH_TRANSMUTATION] = { "Transmutation","Réplicateur ligneux", THM_SOCIETE,FN_PRODUCTION,3, TECH_ALCHIMIE, true,false,UNIV,
    0,0,0, 2.0f,0, 0, 0, 1.0f, 0.30f, 1.2f, false },

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
    0,0.5f,0, 0,0, 0, 0, 0.3f, 0, 0.10f, false },
[TECH_COMMUNION_ETHEREE] = { "Communion éthérée","Bastion éthéré", THM_SAVOIR,FN_RENFORCEMENT,2, TECH_GLYPHES_ETHERES, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0.6f, 0.15f, 0.40f, false },
/* ---- Métallurgiste (Forge·Armée → FORGE_RUNES) : +production ----------- */
[TECH_ALLIAGES_NAINS] = { "Alliages des profondeurs","Fonderie de bronze", THM_FORGE,FN_ARMEE,1, TECH_COLLECTE_ARGILE, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_GRAVURE_RUNES] = { "Gravure runique","Rune-forge", THM_FORGE,FN_ARMEE,2, TECH_ALLIAGES_NAINS, false,false,HERITAGE_METALLURGISTE,
    0,0.3f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.50f, false },
/* ---- Mécaniste (Forge·Prod→Renf → AUTOMATES) : +production/+efficacité - */
[TECH_MECANISTE_ROUAGES] = { "Rouages de précision","Engrenagerie", THM_FORGE,FN_PRODUCTION,1, TECH_FONDERIE, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0, 0.05f, 0.20f, false },
[TECH_MECANISTE_HORLOGERIE] = { "Mécanisme d'horlogerie","Horloge mécanique", THM_FORGE,FN_RENFORCEMENT,2, TECH_MECANISTE_ROUAGES, false,false,HERITAGE_MECANISTE,
    0.3f,0.5f,0, 0,0, 0, 0, 0.5f, 0.10f, 0.30f, false },
/* ---- Adaptatif (Société·Renforcement → INTEGRATION) : +cohésion -------- */
[TECH_DROIT_COUTUMIER] = { "Droit coutumier","Code coutumier", THM_SOCIETE,FN_RENFORCEMENT,1, TECH_CHANCELLERIE, false,false,HERITAGE_ADAPTATIF,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_LANGUE_FRANQUE] = { "Langue franque","Lingua franca", THM_SOCIETE,FN_RENFORCEMENT,2, TECH_DROIT_COUTUMIER, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, -1.0f, 0, 0, 0, false },
/* ---- Agraire (Société·Production → ABONDANCE) : +production agricole --- */
[TECH_VERGERS_ETAGES] = { "Vergers étagés","Vergers en terrasses", THM_SOCIETE,FN_PRODUCTION,1, TECH_COLLECTE_NOURRITURE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false },
[TECH_PATURAGES_INTEGRES] = { "Pâturages intégrés","Prairies-vergers", THM_SOCIETE,FN_PRODUCTION,2, TECH_VERGERS_ETAGES, false,false,HERITAGE_AGRAIRE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false },
/* ---- Clanique (Société·Armée → ESCLAVAGE) : +stabilité / +production --- */
[TECH_RITES_GUERRIERS] = { "Rites guerriers","Sanctuaire des ancêtres", THM_SOCIETE,FN_ARMEE,1, TECH_CASERNE, false,false,HERITAGE_CLANIQUE,
    0,0.3f,0, 0,0, 0.2f, 0, 0, 0, 0, false },
[TECH_HORDES_CONQUERANTES] = { "Hordes conquérantes","Camps de rapine", THM_SOCIETE,FN_ARMEE,2, TECH_RITES_GUERRIERS, false,false,HERITAGE_CLANIQUE,
    0,0.5f,0, 0,0, 0.3f, 0.5f, 0, 0, 0, false },

/* ====================================================================== */
/* COMBOS (2026-06-28) — TIER-4 COMBINATOIRE EXCLUSIF (une paire d'héritages)*/
/* prereq=NONE : la PORTE est la double-métabolisation (accès tier 3 aux deux*/
/* héritages, via tech_combo_native + le plafond de tier dans can_research) +*/
/* le coût tier-4. native=héritage A ; tech_combo_native renvoie l'héritage B.*/
/* Effets sur leviers VIVANTS : FN_ARMEE → army_doctrine (auto, tier4=+20/28%);*/
/* FN_PRODUCTION/RENFORCEMENT → NODE_PROD_PCT/EFF_PCT (plus bas) + prospérité. */
/* ====================================================================== */
[TECH_COMBO_POUDRE] = { "Arquebuserie de précision","Manufacture d'armes à feu", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Méca×Métal : +dégâts (feu) via doctrine + poudre (prod%) */
[TECH_COMBO_AUTOMATES_ARC] = { "Automates arcanes","Golems d'essence", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 1.0f, 0, 0, false },         /* Éso×Méca : +production (prod%) + puissance */
[TECH_COMBO_ACADEMIE] = { "Académie cosmopolite","Grande académie", THM_SAVOIR,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    1.0f,0,0, 0,0, 0, 0, 0, 0, 0, false },         /* Éso×Adaptatif : +recherche (Savoir·Prod) + efficacité */
[TECH_COMBO_DRUIDE] = { "Abondance druidique","Bosquet nourricier", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,1.0f,0, 0,0, 0, 0, 0, 0, 0, false },         /* Éso×Agraire : +production agricole (prod%) + stabilité */
[TECH_COMBO_CHAMAN] = { "Chamanisme de guerre","Cercle des chamans", THM_SAVOIR,FN_ARMEE,4, NONE, false,false,HERITAGE_ESOTERIQUE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Éso×Clanique : +magie de guerre via doctrine (arcane_power) */
[TECH_COMBO_GUILDES] = { "Guildes maîtresses","Hôtel des guildes", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false },         /* Métal×Adaptatif : +production (prod%) + or/stabilité */
[TECH_COMBO_CHARRUES] = { "Charrues lourdes","Atelier de charronnage", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Métal×Agraire : +production agricole (prod%) */
[TECH_COMBO_POLIORCETIQUE] = { "Poliorcétique","Forge de guerre", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_METALLURGISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Métal×Clanique : +dégâts via doctrine */
[TECH_COMBO_HORLOGE_MARCH] = { "Horlogerie marchande","Comptoir mécanique", THM_FORGE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false },         /* Méca×Adaptatif : +efficacité (eff%) + or/stabilité */
[TECH_COMBO_MACHINES_AGRI] = { "Machines agricoles","Moulins & semoirs", THM_SOCIETE,FN_PRODUCTION,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Méca×Agraire : +production agricole (prod%, fort) */
[TECH_COMBO_SIEGE] = { "Engins de siège","Arsenal de siège", THM_FORGE,FN_ARMEE,4, NONE, false,false,HERITAGE_MECANISTE,
    0,0,0, 0,0, 0, 0, 0, 0, 0, false },           /* Méca×Clanique : +dégâts via doctrine */
[TECH_COMBO_GRENIER_COLON] = { "Grenier colonial","Comptoir-grenier", THM_SOCIETE,FN_RENFORCEMENT,4, NONE, false,false,HERITAGE_ADAPTATIF,
    1.0f,1.5f,0, 0,0, 0, 0, 0, 0, 0, false },      /* Adaptatif×Agraire : +stabilité & +croissance (K,L) */
[TECH_COMBO_FOEDERATI] = { "Foederati","Camp fédéré", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_ADAPTATIF,
    0,0.5f,0, 0,0, 0, -0.5f, 0, 0, 0, false },     /* Adaptatif×Clanique : +moral via doctrine + cohésion */
[TECH_COMBO_HORDE_ECO] = { "Économie de horde","Halle du butin", THM_SOCIETE,FN_ARMEE,4, NONE, false,false,HERITAGE_AGRAIRE,
    0,0.5f,0, 0,0, 0, 0, 0, 0, 0, false },         /* Agraire×Clanique : +moral via doctrine + production (prod%) */
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
static const float NODE_PROD_PCT[TECH_COUNT] = {
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
static const float NODE_EFF_PCT[TECH_COUNT] = {
    /* Savoir·Production — le savoir-faire rend chaque bras meilleur (efficacité d'emploi). */
    [TECH_SCRIPTORIUM]=0.05f, [TECH_ACADEMIE]=0.07f, [TECH_UNIVERSITE]=0.10f,
    /* ÉTOFFE — l'horlogerie gnome SYNCHRONISE la production (efficacité d'emploi). */
    [TECH_MECANISTE_HORLOGERIE]=0.08f,
    /* COMBOS tier-4 à vocation efficacité/savoir. */
    [TECH_COMBO_HORLOGE_MARCH]=0.10f, [TECH_COMBO_ACADEMIE]=0.08f,
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
    /* COMBINAISON : certains nœuds exigent un SECOND archétype (ET), au même tier requis. */
    { Heritage combo=tech_combo_native(id);
      if (combo!=UNIV && tech_heritage_access_tier(heritage_access, combo) < need) return false; }
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
