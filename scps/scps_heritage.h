#ifndef SCPS_SPECIES_H
#define SCPS_SPECIES_H
/*
 * scps_heritage.h — HÉRITAGES & TRADITIONS (le créateur de culture, façon Stellaris/Civ)
 *
 * « Plus d'espèces, tout le monde est humain. » Deux couches INDÉPENDANTES :
 *   · HÉRITAGE (Heritage) — une LIGNÉE CULTURELLE qui ne pilote QUE les NOMS
 *     (banque de syllabes : ésotérique « ésotérique », clanique « clanique »…). PAS de traits.
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
#include <stdio.h>   /* FILE — sérialisation des slots de culture (sauvegarde) */

/* ===================================================================== */
/* SPHÈRES — tiers de magnitude sur un seul continuum (gouffre corrigé)   */
/* ===================================================================== */
typedef enum { SPHERE_ANCIENS = 0, SPHERE_HOMMES, SPHERE_ETRANGERS, SPHERE_COUNT } Sphere;
/* Distance de sphère : un CONTINUUM (5 = demi-ésotériques courants, 7 = demi-claniques
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
} HeritageLeviers;

typedef struct {
    const char    *name;
    TraitCategory  cat;
    int            pts;       /* signé : +1/+2 atout, −1/−2 défaut */
    TraitId        antonym;
    HeritageLeviers lev;       /* deltas */
    const char    *hover;     /* définition (membrane) */
} TraitDef;

const TraitDef *trait_def(TraitId t);
const char     *trait_name(TraitId t);
const char     *trait_hover(TraitId t);
const char     *category_name(TraitCategory c);

/* ===================================================================== */
/* ESPÈCES & BUILDS                                                       */
/* ===================================================================== */
/* GR1 — L'HÉRITAGE (ex-« heritage ») : une FONCTION, plus une espèce. L'ORDRE et les VALEURS
 * sont PRÉSERVÉS (SAVE-safe ; aucune formule touchée) — seuls les noms changent. */
typedef enum {
    HERITAGE_ESOTERIQUE = 0, HERITAGE_METALLURGISTE, HERITAGE_MECANISTE,
    HERITAGE_ADAPTATIF, HERITAGE_AGRAIRE, HERITAGE_CLANIQUE,
    HERITAGE_COUNT
} Heritage;
/* GR-final — « plus d'espèces, tout le monde est humain » : les HÉRITAGES sont des LIGNÉES
 * CULTURELLES (pas des espèces). L'ancien lexique RACE_* est entièrement migré vers HERITAGE_*. */

/* Un build = un trait par catégorie (indexé par TraitCategory). */
typedef struct { TraitId trait[CAT_COUNT]; } HeritageBuild;

Sphere          heritage_sphere(Heritage r);
const char     *heritage_name(Heritage r);
HeritageBuild    heritage_default_build(Heritage r);

/* Valide : 3 TRADITIONS (une par catégorie), FORCÉ à 2 atouts (pts>0) + 1 défaut (pts<0),
 * aucun antonyme en conflit. Plus de budget/score (supprimé) — la composition 2+/1− EST la règle. */
bool            build_is_valid(const HeritageBuild *b);
/* Compose les leviers des 3 traditions (somme des deltas). */
HeritageLeviers  build_leviers(const HeritageBuild *b);
/* TRADITIONS ALÉATOIRES déterministes — un build VALIDE tiré d'un seed (= identité d'empire),
 * INDÉPENDANT de l'héritage. L'IA en reçoit ; défaut du joueur avant qu'il compose la sienne. */
HeritageBuild    culture_random_build(uint32_t seed);

/* ===================================================================== */
/* CRÉATEUR DE CULTURE — la composition du JOUEUR (override du tirage IA)  */
/* ===================================================================== */
/* Le joueur (et, façon Stellaris, chaque empire) compose une culture (héritage + éthos
 * + 3 traditions) qui REMPLACE le tirage aléatoire pour SON empire. Modèle à SLOTS par
 * ORDINAL d'empire : slot 0 = JOUEUR, slots 1..N-1 = empires IA (POLITY_ANTAGONIST) dans
 * l'ordre des cid. À la genèse, on LIE chaque cid d'empire à son slot (culture_bind_cid).
 * TANT QU'AUCUN slot n'est posé (le défaut — chronique, bancs, déterminisme), TOUT est
 * exactement comme avant : héritage ADAPTATIF, traditions aléatoires, éthos émergent.
 * `ethos` voyage en int (l'enum Ethos vit dans scps_culture.h — évite un cycle d'include). */
#define CULTURE_SLOTS 64
void              culture_slot_set(int slot, Heritage heritage, int ethos, HeritageBuild build);
void              culture_slot_clear_all(void);         /* désactive tous les slots + RAZ la map cid */
bool              culture_slot_active(int slot);
Heritage  culture_slot_heritage(int slot);      /* HERITAGE_ADAPTATIF si inactif */
int               culture_slot_ethos(int slot);         /* 2 (ETHOS_ORDRE) si inactif */
void              culture_reset_cid_map(void);          /* map cid→slot toute à -1 (par genèse) */
void              culture_bind_cid(int cid, int slot);  /* établit cid→slot à la genèse */
int               culture_slot_of_cid(int cid);         /* slot d'un cid, -1 si aucun */
bool              culture_any_active(void);              /* un slot au moins est-il posé ? */
/* Build EFFECTIF pour un cid : l'override de son slot s'il est lié+actif, sinon le tirage. */
HeritageBuild      culture_build_for(uint32_t cid);

/* SAUVEGARDE — sérialise/restaure les slots + la map cid→slot (section CULT) : un monde
 * chargé garde les cultures composées (joueur ET IA). culture_slots_load rétablit aussi
 * la map cid→slot ⇒ culture_build_for rend les bons builds post-chargement. false si corrompu. */
void              culture_slots_save(FILE *f);
bool              culture_slots_load(FILE *f);

/* ---- Compat « joueur » (= slot 0) : la surface d'origine, conservée ---------- */
void              culture_player_compose(Heritage heritage, int ethos, HeritageBuild build); /* = slot 0 */
void              culture_player_bind(int cid);     /* = culture_bind_cid(cid, 0) */
void              culture_player_clear(void);        /* = culture_slot_clear_all */
bool              culture_player_active(void);        /* = culture_any_active */
int               culture_player_cid(void);          /* cid lié au slot 0, -1 sinon */
Heritage  culture_player_heritage(void);     /* slot 0 (HERITAGE_ADAPTATIF si inactif) */
int               culture_player_ethos(void);        /* slot 0 (2/ETHOS_ORDRE si inactif) */

#endif /* SCPS_SPECIES_H */
