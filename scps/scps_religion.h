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
#include <stdio.h>          /* FILE — sérialisation (section RELG, P3) */
#include "scps_culture.h"   /* enum Credo existant (CREDO_*) */
#include "scps_econ.h"      /* World/WorldEconomy/PopCulture (intégration P4/P8) */
#include "scps_legitimacy.h"/* WorldLegitimacy (L par région — fracture P8) */

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
/* P4 — cache d'accumulateur PAR PAYS + éligibilité au schisme             */
/* ===================================================================== */
/* acc CACHÉ du pays = somme pôles+crédo de SA religion (ZÉRO si sans religion).
 * Recalculé à religion_set_country / religion_load. Les consommateurs moteur LISENT
 * ceci (jamais de recompute par site). World visible via scps_culture.h→scps_types.h. */
const ReligAccum* religion_country_acc(int cid);

typedef enum { RSE_NONE = 0, RSE_RUPTURE, RSE_DERIVE } ReligSchismMode;
/* éligibilité au schisme (LECTURE pure, aucun effet de bord). RUPTURE : le pays NE
 * contrôle PAS la cellule-centre de sa religion (centre conquis/étranger). DERIVE
 * (dérive province distance-centre) : phase ultérieure — renvoie RSE_NONE pour l'instant. */
ReligSchismMode religion_schism_eligible(const World *w, int cid);

/* ===================================================================== */
/* P8 — religion par RÉGION (granularité du moteur : culture/L/agitation/  */
/* sécession sont RÉGIONALES) + héritage + fracture. État GLOBAL (RELG).   */
/* ===================================================================== */
#define RELIG_MAX_REGION 1024   /* ≥ SCPS_MAX_REG */
int  religion_of_region(int rg);              /* -1 = aucune */
void religion_set_region(int rg, int rid);
/* les régions du pays HÉRITENT de la religion du pays (à la fondation). */
void religion_inherit_regions(const World *w, int cid);
/* FRACTURE : au schisme INTERNE, les régions du pays CULTURELLEMENT distantes du centre
 * ET peu légitimes (L bas) basculent vers la religion ENFANT ; le noyau garde le parent.
 * Renvoie le nombre de régions basculées. Pure mutation de g_region_religion. */
int  religion_fracture(const World *w, const WorldEconomy *econ,
                       const WorldLegitimacy *wl, int cid, int child_rid);

/* ===================================================================== */
/* P6 — LETTRÉ (scholar) : agent religieux par pays (1 actif/pays).        */
/* Face dérivée du crédo : Pluraliste→Gourou(RESIST) · Évangéliste→        */
/* Missionnaire(CONVERT) · Purificateur→Moine(STABILIZE). État sérialisé.  */
/* ===================================================================== */
int  scholar_role_from_credo(int credo);              /* ScholarRole (-1 si crédo hors-borne) */
int  religion_scholar_recruit(int cid, int region);   /* role>=0 si le pays a une foi ; -1 sinon */
int  religion_scholar_active(int cid);                /* 1 si un lettré est déployé */
int  religion_scholar_role(int cid);                  /* ScholarRole courant ; -1 si aucun */
int  religion_scholar_region(int cid);                /* région d'action ; -1 */
void religion_scholar_tick(const World *w, WorldEconomy *econ);  /* CONVERT agit ; RESIST/STABILIZE = requêtes */
int  religion_region_stabilized(int rg);              /* un Moine y calme l'agitation ? (1/0) */
int  religion_region_resisted(int rg);                /* un Gourou y bloque la conversion ? (1/0) */

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

/* ===================================================================== */
/* LIEN pays→religion + SÉRIALISATION (P3) — état en GLOBAL du module       */
/* (comme g_creditor), PAS dans la struct pays partagée : la genèse/le save */
/* du viewer et le déterminisme (chronicle) restent INTACTS ; seule la save */
/* FAÇADE (scps_save) ajoute une section RELG.                              */
/* ===================================================================== */
#define RELIG_MAX_COUNTRY 512   /* ≥ SCPS_MAX_COUNTRY ; découplé de scps_types.h */
int  religion_of_country(int cid);            /* -1 = sans religion */
void religion_set_country(int cid, int rid);  /* rid=-1 efface */
void religion_reset(void);                    /* RAZ registre + liens (nouvelle partie) */
void religion_save(FILE *f);                  /* section RELG (registre + liens pays) */
int  religion_load(FILE *f);                  /* 0 ok · !=0 corrompu */

#endif /* SCPS_RELIGION_H */
