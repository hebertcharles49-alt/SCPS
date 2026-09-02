#ifndef SCPS_DOCTRINES_H
#define SCPS_DOCTRINES_H
/*
 * scps_doctrines.h — LES DOCTRINES (docs/DESIGN_MISSIONS_DOCTRINES.md §4,
 * catalogue complet : docs/DESIGN_DOCTRINES_ANNEXE.md)
 *
 * L'ARBRE DE SPÉCIALISATION PARALLÈLE : 17 doctrines, 6 idées chacune, achetées
 * EN SÉQUENCE, payées en INFLUENCE POLITIQUE (scps_influence.h). Le joueur n'en
 * tiendra jamais plus de SIX — il renonce à la moitié du catalogue.
 *
 * ── LES RÉVISIONS FERMES (décisions joueur 2026-09-01) ────────────────────
 *   « Pas de gate, pas de prérequis : tu cliques, t'as le bonus. »
 *   AUCUN prérequis d'éthos, d'héritage, de religion, d'édifice ou de tech.
 *   Le frein est le COÛT, qui monte avec ce qu'on possède déjà :
 *     adopter  = DOCT_COST_BASE + DOCT_COST_STEP × doctrines actives   (50 → 175)
 *     acheter  = IDEA_COST_BASE + IDEA_COST_STEP × idées possédées     (30 → 135)
 *   ENTRETIEN EN INFLUENCE, pas en couronnes : DOCT_UPKEEP /mois par doctrine
 *   active, débité au tick mensuel APRÈS la génération. Insolvable ce mois ⇒ les
 *   doctrines les plus RÉCEMMENT adoptées se SUSPENDENT ce mois (mults à 1.0).
 *   Les SEULES règles d'exclusivité : Commerce ⊥ Mercantilisme, et un seul
 *   COURANT parmi Aristocratie/Bourgeoisie/Populaire/Divin.
 *   ABANDON LIBRE : le slot se libère, les idées sont perdues, aucun remboursement.
 *
 * ── LES SLOTS : SIX, LIBRES DÈS LA GENÈSE ────────────────────────────────
 * Décision joueur 2026-09-02 : AUCUNE ouverture progressive (l'ancienne idée
 * « un slot par âge engagé » est ABANDONNÉE). Les six slots sont ouverts
 * d'office ; le frein est le COÛT, rien d'autre. `doctrines_slots_open` rend
 * donc toujours DOCT_SLOTS_MAX — l'état « verrouillé » du contrat de readers
 * existe encore mais n'est JAMAIS émis.
 *
 * ── LA LINÉARISATION SUR L'ASSIETTE (décision joueur 2026-09-02) ──────────
 * Tous les prix (adoption, idée, entretien) sont multipliés par l'ÉCHELLE
 * D'ASSIETTE `é` (influence_scale, scps_influence.h) : un empire deux fois
 * plus noble gagne deux fois plus ET paie deux fois plus — « in fine ça revient
 * au même », mais le joueur est LIBRE de faire grandir sa noblesse. L'échelle
 * est calculée par l'APPELANT (qui a l'économie et le courant) et passée en
 * paramètre `ech` : le module reste léger (scps_econ.h l'inclut).
 *
 * ── P1 : LE JOUEUR SEUL ──────────────────────────────────────────────────
 * L'état est PAR PAYS (la symétrie IA « choix par score » est une vague à part),
 * mais génération de slots, adoption, achat et entretien sont gatés
 * `human_player` côté scps_sim.c ⇒ la chronique (human=-1) ne touche RIEN et le
 * golden est intact PAR CONSTRUCTION (motif décrets/influence/Desseins).
 *
 * ── LE SITE DE LECTURE ───────────────────────────────────────────────────
 * Motif decree_mult : `tune_f("CLÉ", défaut) × doctrine_key_mult(cid, "CLÉ")`
 * au site de lecture MOTEUR — JAMAIS tune_set (global, IA comprise). Le
 * multiplicateur est le PRODUIT des idées possédées et NON SUSPENDUES qui
 * portent la clé, CLAMPÉ [0.60, 1.60] (H2bis, l'anti-« modifier soup »).
 * Le miroir lu par doctrine_key_mult est un cache de PROCESS (jamais sérialisé),
 * rafraîchi par doctrines_tick ET juste après un chargement (doctrines_sync).
 */
#include "scps_types.h"      /* SCPS_MAX_COUNTRY — le SOCLE seul : scps_doctrines.h est inclus TRÈS TÔT (scps_econ.h), il ne doit rien tirer de plus */
#include <stdint.h>
#include <stdbool.h>

/* ── LE CATALOGUE ───────────────────────────────────────────────────────── */
typedef enum {
    DOCT_OFFENSE = 0,
    DOCT_DEFENSE = 1,
    DOCT_COMMERCE = 2,
    DOCT_MERCANTILISME = 3,
    DOCT_PEUPLE = 4,
    DOCT_COLONISATION = 5,
    DOCT_DIPLOMATIE = 6,
    DOCT_VASSAUX = 7,
    DOCT_PRODUCTION = 8,
    DOCT_INFRASTRUCTURE = 9,
    DOCT_TECHNOLOGIE = 10,
    DOCT_CONNAISSANCES = 11,
    DOCT_FAUSTIEN = 12,
    /* les QUATRE COURANTS POLITIQUES — un seul à la fois ; le courant actif
     * RE-SIED l'assiette de génération de l'influence sur SA classe (§4.3bis). */
    DOCT_ARISTOCRATIE = 13,
    DOCT_BOURGEOISIE = 14,
    DOCT_POPULAIRE = 15,
    DOCT_DIVIN = 16,
    DOCT_COUNT
} DoctrineId;

#define DOCT_IDEAS      6    /* idées par doctrine, achetées EN SÉQUENCE */
#define DOCT_SLOTS_MAX  6    /* slots sur TOUTE la partie (on renonce à 11 doctrines) */
#define DOCT_CURRENT_FIRST DOCT_ARISTOCRATIE   /* le premier des quatre COURANTS */

/* ── LA TABLE CENTRALE ──────────────────────────────────────────────────── */
typedef struct {
    uint16_t    name;    /* StrId — le nom de l'idée */
    uint16_t    bonus;   /* StrId — LA ligne de bonus (une seule, lisible) */
    const char *icon;    /* "idea_<doctrine>_<idée>" — le fichier lot15_idees installé */
    const char *k1; float m1;   /* clé de registre + multiplicateur (NULL = aucune) */
    const char *k2; float m2;
    uint8_t     verbe;   /* 1 = l'idée débloque une ACTION (vague suivante) */
    uint8_t     cable;   /* MASQUE : bit0 = k1 portée au site de lecture · bit1 = k2.
                          * 0 = aucun effet moteur cette vague (clé fantôme du registre,
                          * site hoisté hors de portée d'un cid, pure RÈGLE, ou VERBE) —
                          * la clé reste écrite pour mémoire, la façade dit « à venir ». */
} DoctIdeaDef;

typedef struct {
    uint16_t    name;    /* StrId */
    uint16_t    hover;   /* StrId — l'explication RAPIDE (1-2 phrases) */
    const char *bg;      /* "doct_<x>_bg" — le fichier lot14_doctrines installé */
    DoctIdeaDef idea[DOCT_IDEAS];
} DoctDef;

extern const DoctDef DOCT_DEF[DOCT_COUNT];

/* ── L'ÉTAT (sérialisé — section DOCT, save v106) ───────────────────────── *
 * Tout est BORNÉ et revalidé au chargement (scps_save_sane) : chaque champ
 * indexe un tableau ou borne une boucle. -1 = « vide » partout. */
typedef struct {
    int8_t  doct [SCPS_MAX_COUNTRY][DOCT_SLOTS_MAX];  /* doctrine adoptée par slot (-1 = vide) */
    int16_t seq  [SCPS_MAX_COUNTRY][DOCT_SLOTS_MAX];  /* RANG d'adoption, croissant (-1 = vide) */
    int8_t  susp [SCPS_MAX_COUNTRY][DOCT_SLOTS_MAX];  /* 1 = suspendue CE mois (entretien impayé) */
    int8_t  ideas[SCPS_MAX_COUNTRY][DOCT_COUNT];      /* idées possédées, 0..DOCT_IDEAS (séquentielles) */
    int16_t seq_next  [SCPS_MAX_COUNTRY];             /* le prochain rang d'adoption à distribuer */
} DoctrineState;

void doctrines_init(DoctrineState *ds);

/* CLÔTURE MENSUELLE (appelée pour le JOUEUR SEUL — l'appelant gate human_player,
 * motif décrets/influence) :
 *   1. PRÉLÈVE l'entretien EN INFLUENCE (DOCT_UPKEEP × doctrines actives × `ech`)
 *      — APRÈS la génération du mois (influence_tick) ; insolvable ⇒ suspend les
 *      doctrines les plus RÉCEMMENT adoptées, une à une, jusqu'à ce que le reste
 *      soit payable (ordre déterministe : rang d'adoption décroissant) ;
 *   2. RAFRAÎCHIT le miroir de process lu par doctrine_key_mult. */
struct InfluenceState;
void doctrines_tick(DoctrineState *ds, struct InfluenceState *is, int cid, float ech);

/* RAFRAÎCHIT le miroir de process depuis l'état sérialisé (cache jamais
 * sérialisé — motif missions_boons_sync). À rappeler juste après un chargement :
 * sans lui, une partie rechargée verrait ses doctrines MUETTES jusqu'à la
 * première clôture, et le --savetest (A==B) le prendrait. */
void doctrines_sync(const DoctrineState *ds);

/* LE SITE DE LECTURE MOTEUR. Produit des multiplicateurs des idées POSSÉDÉES et
 * NON SUSPENDUES qui portent `key`, clampé [0.60, 1.60]. 1.0 si rien (et
 * O(1) — un pays sans idée sort au premier test). */
float doctrine_key_mult(int cid, const char *key);

/* ── LES VERBES DE GESTION (drainés, revalidés — motif CMD_SEAL_DESSEIN) ──
 * Chacun revalide TOUT contre l'état courant et paie en influence ; renvoie
 * 1 si l'acte a pris, 0 sinon (silencieux, rien n'est modifié). `is` peut être
 * NULL (bancs) : tout est alors gratuit. */
int doctrines_adopt   (DoctrineState *ds, struct InfluenceState *is, int cid, int slot, int doctrine, float ech);
int doctrines_buy_idea(DoctrineState *ds, struct InfluenceState *is, int cid, int doctrine, float ech);
int doctrines_abandon (DoctrineState *ds, struct InfluenceState *is, int cid, int slot);

/* ── LECTEURS PURS (façade + bancs) ─────────────────────────────────────── */
int   doctrines_slots_open(const DoctrineState *ds, int cid);               /* toujours DOCT_SLOTS_MAX */
int   doctrines_at        (const DoctrineState *ds, int cid, int slot);      /* la doctrine du slot (-1) */
int   doctrines_slot_of   (const DoctrineState *ds, int cid, int doctrine);  /* le slot d'une doctrine (-1) */
bool  doctrines_suspended (const DoctrineState *ds, int cid, int slot);
int   doctrines_ideas_of  (const DoctrineState *ds, int cid, int doctrine);  /* 0..DOCT_IDEAS */
int   doctrines_n_active  (const DoctrineState *ds, int cid);                /* slots OCCUPÉS */
int   doctrines_n_ideas   (const DoctrineState *ds, int cid);                /* Σ toutes doctrines */
/* Les PRIX sont rendus DÉJÀ ÉCHELONNÉS (× `ech`) : la façade n'a rien à savoir
 * de la linéarisation, elle affiche l'entier tel quel. */
int   doctrines_adopt_cost(const DoctrineState *ds, int cid, float ech);     /* prix COURANT d'une adoption */
int   doctrines_idea_cost (const DoctrineState *ds, int cid, float ech);     /* prix COURANT d'une idée */
int   doctrines_upkeep    (const DoctrineState *ds, int cid, float ech);     /* Σ entretiens /mois (entier, membrane) */
/* Les MÊMES prix, non arrondis — ce que le moteur DÉBITE vraiment (l'entier
 * ci-dessus est la membrane d'affichage, jamais la vérité comptable). */
float doctrines_adopt_cost_f(const DoctrineState *ds, int cid, float ech);
float doctrines_idea_cost_f (const DoctrineState *ds, int cid, float ech);
float doctrines_upkeep_f    (const DoctrineState *ds, int cid, float ech);
/* Le COURANT politique actif (DOCT_ARISTOCRATIE..DOCT_DIVIN), -1 si aucun.
 * NON suspendu : un courant suspendu ne re-sied plus l'assiette ce mois-ci. */
int   doctrines_current   (const DoctrineState *ds, int cid);
/* Pourquoi `doctrine` n'est-elle PAS adoptable ? 0 = elle l'est.
 * DOCT_NO_SLOT / DOCT_NO_INFLUENCE / DOCT_ALREADY / DOCT_EXCLUSIVE_PAIR /
 * DOCT_EXCLUSIVE_CURRENT — la façade en fait des MOTS. */
enum { DOCT_OK = 0, DOCT_NO_SLOT, DOCT_ALREADY, DOCT_EXCLUSIVE_PAIR,
       DOCT_EXCLUSIVE_CURRENT, DOCT_NO_INFLUENCE };
int   doctrines_why_not(const DoctrineState *ds, const struct InfluenceState *is,
                        int cid, int slot, int doctrine, float ech);

/* TÉLÉMÉTRIE (print-only, chronicle) — Σ entretien PAYÉ (en influence) et Σ
 * suspensions posées depuis la genèse de cette sim (statiques de module, RAZ à
 * doctrines_init, JAMAIS sérialisées — motif econ_colony_stats). N'entrent dans
 * AUCUN calcul moteur. */
void  doctrines_stats_get(double *upkeep_paid, long *suspensions);

#endif /* SCPS_DOCTRINES_H */
