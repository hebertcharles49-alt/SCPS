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
#define RELIG_SCHISM_MAX 5   /* schismes MAX par racine fondatrice — relâché 2→5 pour la DÉRIVE
                              * (la Réforme culturelle) : une foi peut se ramifier en plusieurs
                              * courants adaptés aux cultures de ses marches (Rome → catholique +
                              * protestant + orthodoxe…). La porte culture-mismatch (region_faith_drifts)
                              * BORNE naturellement le nombre réel (un empire homogène n'y touche pas). */

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
 * contrôle PAS la cellule-centre de sa religion (centre conquis/étranger) → il rompt
 * et s'unit sur une foi autonome. DÉRIVE (la Réforme) : le pays tient son centre mais
 * une marche SUR la foi d'État dérive culturellement (distance au centre > seuil, L
 * basse) → un schisme adapté à SA culture va y prendre, le centre gardant le parent
 * (minorité de même racine = hérésie). econ/wl requis pour la porte de DÉRIVE. */
ReligSchismMode religion_schism_eligible(const World *w, const WorldEconomy *econ,
                                        const WorldLegitimacy *wl, int cid);

/* ---- Lecteurs manquants (mission « lecteurs ») — PURS, dérivés, aucun état neuf ------- *
 * religion_fracture_level : part POP-PONDÉRÉE des régions du pays dont le culte DOMINANT
 *   (religion_of_region) ≠ la foi d'État (religion_of_country). [0..1], 0 = athée ou
 *   uniforme. Nécessaire à C2 (décret tolérance). Voir doc complète en tête d'impl. */
float religion_fracture_level(const World *w, const WorldEconomy *econ, int cid);
/* religion_credo_drift : ALIAS documenté de religion_fracture_level — le module religion
 *   ne fournit qu'UN signal dérivable honnêtement pour « dérive de crédo/pratique vs foi
 *   professée » (pas un canal distinct inventé). Nécessaire à C4. */
float religion_credo_drift(const World *w, const WorldEconomy *econ, int cid);
/* religion_scholar_drift : le lettré actif porte-t-il une FACE périmée (≠ celle qu'exige
 *   le crédo d'État COURANT, scholar_role_from_credo) ? {0,1} — PAS une durée d'inactivité
 *   (le module ne stocke aucune ancienneté de recrutement, seulement le décompte de la
 *   mission courante) ; 0 si aucun lettré actif ou pays sans religion. Nécessaire à C3. */
float religion_scholar_drift(int cid);

/* ===================================================================== */
/* P8 — religion par RÉGION (granularité du moteur : culture/L/agitation/  */
/* sécession sont RÉGIONALES) + héritage + fracture. État GLOBAL (RELG).   */
/* ===================================================================== */
#define RELIG_MAX_REGION 1024   /* ≥ SCPS_MAX_REG */
int  religion_of_region(int rg);              /* culte DOMINANT (cache dérivé des groupes) ; -1 = aucun */
/* CACHE : recalcule le culte dominant d'une région / de toutes depuis PopGroup.faith. À
 * appeler au tick (après la démographie) — la foi VIT sur le groupe, ceci n'est que le reflet. */
void religion_refresh_region(const WorldEconomy *econ, int r);
void religion_refresh_all   (const WorldEconomy *econ);
/* convertit les NATIFS de souche de la région (rep-province) à `rid` (missionnaire / Contre-
 * Réforme) + rafraîchit le cache ; les diasporas gardent leur foi portée. econ requis. */
void religion_set_region(WorldEconomy *econ, int rg, int rid);
/* les NATIFS des régions du pays HÉRITENT de la foi d'État (à la fondation) ; diasporas exemptées. */
void religion_inherit_regions(const World *w, WorldEconomy *econ, int cid);
/* FRACTURE : au schisme INTERNE, les régions du pays CULTURELLEMENT distantes du centre
 * ET peu légitimes (L bas) basculent vers la religion ENFANT ; le noyau garde le parent.
 * Renvoie le nombre de régions basculées. Pure mutation de g_region_religion. */
int  religion_fracture(const World *w, WorldEconomy *econ,
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

/* ── PLAFOND mondial de religions FONDATRICES = ⌈n_empires/2⌉ sur les RACINES (LOT T,
 * 2026-07-07 : relâché de ⌈N/3⌉) ; les schismes ont leur PROPRE plafond PAR RACINE
 * (religion_can_schism, RELIG_SCHISM_MAX) ────────────────────────────────────────── */
int  religion_root_count(void);                       /* nb de religions racines (parent==-1) */
int  religion_cap(int n_empires);                     /* ⌈n_empires/2⌉, ≥1 */
int  religion_can_found(int n_empires);               /* RACINES < ⌈N/2⌉ ? (gate FONDATION) */
int  religion_root_of(int rid);                       /* racine-ancêtre (remonte parent) */
int  religion_can_schism(int parent_rid);             /* < RELIG_SCHISM_MAX schismes sous la racine ? */
void religion_set_empire_ref(int n);                  /* ancre le plafond au compte d'empires de GENÈSE */
int  religion_empire_ref(void);                       /* ledit compte (0 si non semé ⇒ cap 1) */
int  religion_found_random(int cid, int centre_cell, uint32_t seed); /* fonde une racine ALÉATOIRE valide + set_country ; -1 */
int  religion_adopt_existing(int cid, uint32_t seed); /* RALLIE une racine existante + set_country ; -1 si aucune */

/* ===================================================================== */
/* i18n — mots RÉSOLUS (membrane ; même mécanisme que credo_name/heritage_name) */
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
