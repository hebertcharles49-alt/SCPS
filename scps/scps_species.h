#ifndef SCPS_SPECIES_H
#define SCPS_SPECIES_H
/*
 * scps_species.h — HÉRITAGES & TRADITIONS (le créateur de culture, façon Stellaris/Civ)
 *
 * « Plus de races, tout le monde est humain. » Deux couches INDÉPENDANTES :
 *   · HÉRITAGE (SpeciesArchetype) — une LIGNÉE CULTURELLE qui ne pilote QUE les NOMS
 *     (banque de syllabes : ésotérique « elfique », clanique « orque »…). PAS de traits.
 *   · TRADITIONS (le build) — 3 traditions, une par AXE (Physique/Social/Intellectuel),
 *     FORCÉ à 1 atout MAJEUR (+2) + 1 atout MINEUR (+1) + 1 défaut (−1, plus de défaut
 *     majeur). Plus de budget/score : la composition EST la règle. Le joueur les compose ;
 *     l'IA les tire au hasard (culture_random_build) — indépendamment de l'héritage.
 *
 * Les traditions se SUPERPOSENT à la fiche culturelle SCPS (PopCulture) ; ensemble
 * (héritage + axe/éthos + traditions) elles définissent un peuple.
 * Membrane : le joueur lit des MOTS (« Robuste », « Belliqueux ») + leur définition
 * au survol — jamais les leviers chiffrés derrière.
 */
#include <stdbool.h>
#include <stdint.h>

/* ===================================================================== */
/* SPHÈRES — tiers de magnitude sur un seul continuum (gouffre corrigé)   */
/* ===================================================================== */
typedef enum { SPHERE_ANCIENS = 0, SPHERE_HOMMES, SPHERE_ETRANGERS, SPHERE_COUNT } Sphere;
/* Distance de sphère : un CONTINUUM (5 = demi-elfes courants, 7 = demi-orques
 * rares et tendus), jamais un mur — tout s'assimile à forte P+K et avec du
 * temps (cf. culture_can_syncretize). */
float       sphere_distance(Sphere a, Sphere b);
const char *sphere_name(Sphere s);

/* ===================================================================== */
/* TRAITS                                                                 */
/* ===================================================================== */
typedef enum { CAT_PHYSIQUE = 0, CAT_SOCIAL, CAT_INTELLECTUEL, CAT_COUNT } TraitCategory;

/* 36 traits = 3 catégories × (6 atouts puis 6 défauts appariés à l'antonyme). */
typedef enum {
    /* Physique — atouts */
    T_ROBUSTE = 0, T_REGENERANT, T_PROLIFIQUE, T_LONGEVIF, T_ENDURANT, T_SOBRE,
    /* Physique — défauts (antonymes appariés) */
    T_FRELE, T_CONVALESCENT, T_LENT_CROITRE, T_EPHEMERE, T_FRAGILE_CLIMAT, T_VORACE,
    /* Social — atouts */
    T_BELLIQUEUX, T_SOUDE, T_CHARISMATIQUE, T_OUVERT, T_DISCIPLINE, T_PROSELYTE,
    /* Social — défauts */
    T_DEBONNAIRE, T_FACTIEUX, T_REBUTANT, T_INSULAIRE, T_FRONDEUR, T_RESERVE,
    /* Intellectuel — atouts */
    T_INVENTIF, T_ARCANIQUE, T_STUDIEUX, T_BATISSEUR, T_ADAPTABLE, T_INDUSTRIEUX,
    /* Intellectuel — défauts */
    T_BORNE, T_SOURD_ARCANE, T_INCULTE, T_BROUILLON, T_TRADITIONALISTE, T_INDOLENT,
    TRAIT_COUNT
} TraitId;

/* Leviers (ancrés au moteur). Deltas : 0 = neutre. demographie/productivite/
 * derive sont RELATIFS (s'appliquent comme 1+x) ; les autres sont ABSOLUS
 * (additifs sur l'échelle 0..10 du moteur, ou portée diplo). */
typedef struct {
    float demographie;   /* croissance de pop */
    float productivite;  /* rendement éco / contribution PE */
    float influence;     /* portée diplo & culturelle */
    float coercition;    /* + H (militaire/contrôle) */
    float capacite;      /* + K (diversité métabolisée) */
    float permeabilite;  /* + P (porte d'assimilation) */
    float arcane;        /* affinité magie (+ = pente faustienne) */
    float derive;        /* drift d'horloge & timer d'assimilation */
    float fracture;      /* + = fracture aggravée ; − = amortie */
} SpeciesLeviers;

typedef struct {
    const char    *name;
    TraitCategory  cat;
    int            pts;       /* signé : +1/+2 atout, −1/−2 défaut */
    TraitId        antonym;
    SpeciesLeviers lev;       /* deltas */
    const char    *hover;     /* définition (membrane) */
} TraitDef;

const TraitDef *trait_def(TraitId t);
const char     *trait_name(TraitId t);
const char     *trait_hover(TraitId t);
const char     *category_name(TraitCategory c);

/* ===================================================================== */
/* ESPÈCES & BUILDS                                                       */
/* ===================================================================== */
/* GR1 — L'HÉRITAGE (ex-« race ») : une FONCTION, plus une espèce. L'ORDRE et les VALEURS
 * sont PRÉSERVÉS (SAVE-safe ; aucune formule touchée) — seuls les noms changent. */
typedef enum {
    HERITAGE_ESOTERIQUE = 0, HERITAGE_METALLURGISTE, HERITAGE_MECANISTE,
    HERITAGE_ADAPTATIF, HERITAGE_AGRAIRE, HERITAGE_CLANIQUE,
    HERITAGE_COUNT
} SpeciesArchetype;
/* GR-final — « plus de races, tout le monde est humain » : les HÉRITAGES sont des LIGNÉES
 * CULTURELLES (pas des espèces). L'ancien lexique RACE_* est entièrement migré vers HERITAGE_*. */

/* Un build = un trait par catégorie (indexé par TraitCategory). */
typedef struct { TraitId trait[CAT_COUNT]; } SpeciesBuild;

Sphere          species_sphere(SpeciesArchetype r);
const char     *species_name(SpeciesArchetype r);
SpeciesBuild    species_default_build(SpeciesArchetype r);

/* Valide : 3 TRADITIONS (une par catégorie), FORCÉ à 2 atouts (pts>0) + 1 défaut (pts<0),
 * aucun antonyme en conflit. Plus de budget/score (supprimé) — la composition 2+/1− EST la règle. */
bool            build_is_valid(const SpeciesBuild *b);
/* Compose les leviers des 3 traditions (somme des deltas). */
SpeciesLeviers  build_leviers(const SpeciesBuild *b);
/* TRADITIONS ALÉATOIRES déterministes — un build VALIDE tiré d'un seed (= identité d'empire),
 * INDÉPENDANT de l'héritage. L'IA en reçoit ; défaut du joueur avant qu'il compose la sienne. */
SpeciesBuild    culture_random_build(uint32_t seed);

#endif /* SCPS_SPECIES_H */
