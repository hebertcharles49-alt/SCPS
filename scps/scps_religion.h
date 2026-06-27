#ifndef SCPS_RELIGION_H
#define SCPS_RELIGION_H
/*
 * scps_religion.h — MODULE FONDATION religion (PUR : data model + primitives).
 *
 * Aucun câblage moteur, aucune sérialisation, aucun bump SAVE (phases dédiées).
 * Le CRÉDO réutilise l'enum EXISTANT de scps_culture.h (Credo : CREDO_PLURALISTE /
 * CREDO_EVANGELISTE / CREDO_PURIFICATEUR) — jamais redéfini. RELIG_CREDO[] est aligné
 * sur l'ordre de cet enum.
 */
#include <stdint.h>
#include "scps_culture.h"   /* enum Credo existant (CREDO_*) */

#define RELIG_MAX 64

/* 8 axes ; pole>>1 == axe */
typedef enum { RA_SANG, RA_FEU, RA_SEUIL, RA_SERMENT,
               RA_VEILLE, RA_CANON, RA_DON, RA_GLAIVE, RA_AXIS_COUNT } ReligAxis;

/* 16 pôles, paires adjacentes par axe */
typedef enum {
  RP_FECONDITE, RP_OFFRANDE,   /* SANG    */
  RP_TRANSE,    RP_SILENCE,    /* FEU     */
  RP_ACCUEIL,   RP_MUR,        /* SEUIL   */
  RP_COURONNE,  RP_ASSEMBLEE,  /* SERMENT */
  RP_ANCETRES,  RP_CENDRE,     /* VEILLE  */
  RP_GNOSE,     RP_ORTHODOXIE, /* CANON   */
  RP_FRUGALITE, RP_FASTE,      /* DON     */
  RP_COURAGE,   RP_TREVE,      /* GLAIVE  */
  RP_COUNT
} ReligPole;

/* canaux abstraits ; phase 4 les mappe aux vraies coordonnées */
typedef enum {
  RC_POPGROWTH, RC_STAB, RC_P, RC_H, RC_L, RC_K, RC_F, RC_PE, RC_I,
  RC_COHESION, RC_MORALE, RC_CONSCRIPT, RC_RESEARCH, RC_ENTROPY,
  RC_INFLUENCE, RC_REVENUE, RC_ASSIM, RC_COERCION, RC_AGITATION,
  RC_COUNT
} ReligChannel;

typedef struct { ReligChannel ch; float mag; } ReligDelta;
typedef struct { ReligAxis axis; const char* key; ReligDelta d[2]; } ReligPoleDef;

extern const ReligPoleDef RELIG_POLES[RP_COUNT];

typedef struct {
  int      id;
  int      parent;        /* -1 = racine */
  int      centre_cell;   /* 1er sanctuaire ; pivot du schisme */
  int      credo;         /* enum Credo de scps_culture.h */
  int      traditions[3]; /* ReligPole, 1 max par axe */
  uint8_t  color[3];      /* RGB ; schisme => variante proche */
  int      founder_cid;
} Religion;

typedef struct { float ch[RC_COUNT]; } ReligAccum;

extern Religion g_religions[RELIG_MAX];
extern int      g_religion_count;

int  religion_picks_valid(int p0, int p1, int p2);          /* axes distincts + bornes */
int  religion_spawn(int credo, const int trad[3],
                    int centre_cell, int founder_cid,
                    const uint8_t color[3]);                /* -> id racine, -1 si invalide */
int  religion_schism(int parent_id, int slot_a, int pole_a,
                     int slot_b, int pole_b, int new_credo,
                     int declare_cell, int founder_cid,
                     int randomize_color, uint32_t seed);   /* -> id enfant, -1 si invalide */
void religion_apply(const Religion* r, ReligAccum* out);    /* somme pôles + crédo */
void religion_color_variant(const uint8_t parent[3], uint8_t out[3],
                            int randomize, uint32_t seed);  /* IA: jitter proche */
int  religion_color_near(const uint8_t parent[3], const uint8_t chosen[3]); /* validation choix joueur */
void religion_selftest(void);

/* ===================================================================== */
/* i18n — mots RÉSOLUS (membrane ; même mécanisme que credo_name/species_name) */
/* ===================================================================== */
const char *relig_axis_name(ReligAxis a);   /* Sang/Feu/Seuil/… */
const char *relig_pole_name(ReligPole p);   /* Fécondité/Offrande/… */
const char *relig_pole_tip(ReligPole p);    /* descripteur diégétique court (sans chiffre) */

/* Faces du LETTRÉ (scholar) — la face dérive du crédo (P6 emploie le roster). */
typedef enum { SCHOLAR_CONVERT, SCHOLAR_RESIST, SCHOLAR_STABILIZE, SCHOLAR_ROLE_COUNT } ScholarRole;
const char *scholar_role_name(int role);     /* Missionnaire/Gourou/Moine */
const char *scholar_role_ability(int role);  /* Conversion/Résistance/Stabilisation */

#endif /* SCPS_RELIGION_H */
