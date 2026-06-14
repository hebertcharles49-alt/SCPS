#ifndef SCPS_AGENCY_H
#define SCPS_AGENCY_H
/*
 * scps_agency.h — LA COUCHE D'AGENCY : actions, temps, bâtiments (§1-§2)
 *
 * tick = 1 JOUR ; une partie = 250 ans ≈ 91 250 jours. Le jour est l'atome ;
 * tout se joue sur des mois, des années, des décennies.
 *
 * Règle non négociable : une action est un LEVIER qui déplace une COORDONNÉE
 * (K, P, H…), jamais un bonus plat. Un bâtiment n'ajoute pas « +10% » : c'est
 * de la densité institutionnelle réalisée, accumulée dans RegionEconomy.build
 * (ProvBuild), que le moteur d'ordre (prosperity_tick) et la légitimité LISENT.
 *
 * Ce module est l'ÉCRIVAIN de ces accumulateurs ; prosperity/legitimacy les
 * relisent. Le joueur voit des bâtiments nommés (membrane), dessous K/H/P bouge.
 */
#include "scps_econ.h"        /* ProvBuild, WorldEconomy, Resource */
#include "scps_world.h"       /* World (biome) */
#include "scps_legitimacy.h"  /* WorldLegitimacy (défrichement ronge L) */
#include "scps_demography.h"  /* leviers intérieurs : coercition (Kuran), groupes, ModifierStack */
#include "scps_tech.h"        /* TechState : édifices gatés par l'arbre (E2 §13) */
#include <stdio.h>

#define SCPS_DAYS_PER_YEAR 365
#define SCPS_GAME_YEARS    250

/* Édifices — chacun déplace une coordonnée (cf. ProvBuild). */
typedef enum {
    EDI_TRIBUNAL = 0, EDI_CHANCELLERIE, EDI_ACADEMIE,  /* → K (Académie aussi P) */
    EDI_GARNISON, EDI_FORTERESSE, EDI_CITADELLE,        /* → H (ronge L) */
    EDI_PORT, EDI_CARAVANSERAIL,                        /* → P */
    EDI_MARCHE, EDI_ENTREPOT,                           /* → PE local */
    EDI_GRENIER, EDI_IRRIGATION, EDI_AQUEDUC,           /* → food (santé urbaine → croissance) */
    EDI_SANCTUAIRE, EDI_TEMPLE, EDI_CATHEDRALE,         /* → foi (SOUTIENT L) */
    EDI_BIBLIOTHEQUE, EDI_MONASTERE,                    /* → savoir (recherche ; le monastère aussi foi) */
    EDI_COMPTOIR, EDI_BANQUE,                           /* → PE (commerce) */
    /* M5 (forks, design §9) — les FOURCHES v1. Maritime : le successeur du PORT
     * fork sur le pôle (Arsenal/Amirauté/Port marchand — §24). Savoir : la BASE
     * fork (Bibliothèque militaire / Bibliothèque→Monastère / Observatoire). */
    EDI_ARSENAL, EDI_AMIRAUTE, EDI_PORT_MARCHAND,       /* ↑ Port, par pôle (M/O/F) */
    EDI_BIBLIO_MIL, EDI_OBSERVATOIRE,                   /* savoir martial / fluide (l'Ordre garde Bibliothèque→Monastère) */
    EDI_TRADE_CENTER,                                   /* M2 : le Centre commercial — hub du réseau GLOBAL (causal : g_centre DÉRIVE de ce bâti, plus un flag) */
    EDIFICE_COUNT
} Edifice;

/* Coût d'un bâtiment (§1) : une RECETTE de matériaux, achetée AU MARCHÉ. Le joueur
 * ne tient pas de stock de matériaux : les ressources vont dans le marché, et bâtir
 * les y ACHÈTE en OR (à leur prix courant) — le manque renchérit (rareté = prix). Le
 * coût monte avec le TIER (plus, et des matériaux plus avancés : bois → bois+métal →
 * métal+précieux), pas avec l'étendue (un grenier coûte pareil partout). */
#define BUILD_RES_MAX 4   /* E1 : le palier 960 j porte 4 composantes (pierre·métal·outils·précieux) */
typedef struct {
    Resource res[BUILD_RES_MAX];
    float    qty[BUILD_RES_MAX];
} BuildCost;

typedef struct {
    const char *name;
    int         days;     /* durée de construction (l'arc de 250 ans) */
    ProvBuild   delta;    /* ce qu'il ajoute à la province à l'achèvement */
    BuildCost   cost;     /* matériaux requis (achetés au marché en or) — §1 */
} EdificeDef;

const EdificeDef *edifice_def(Edifice e);
/* E1bis.11 — FAMILLES ↑ : palier précédent (EDIFICE_COUNT = base/singleton) ; et
 * la pose est-elle bloquée (membre déjà bâti, ou ↑ sans son palier) — pour l'UI. */
Edifice edifice_prev(Edifice e);
Edifice edifice_succ(Edifice e);   /* palier suivant (EDIFICE_COUNT = sommet/singleton) */
Edifice edifice_next_buildable(const WorldEconomy *econ, int region, Edifice base);  /* le ↑ à poser */
bool    edifice_build_blocked(const WorldEconomy *econ, int region, Edifice e);

/* Le PRIX en OR de la recette au marché de la région (Σ qty × prix courant) — un
 * nombre de jeu (affichable). Sert au garde de construction ET à l'UI. */
float agency_build_gold(const WorldEconomy *econ, int region, Edifice e);

/* Familles d'action de province (le motif s'étend). */
typedef enum { AGY_BUILD = 0, AGY_CLEAR, AGY_EXPLOIT, AGY_RELOCATE,
               AGY_REPRESS, AGY_ASSIMILATE, AGY_PURGE, AGY_COLONIZE } ActionKind;

/* Une action en cours (file par pays/province). */
typedef struct {
    ActionKind kind;
    int        region;
    int        param;     /* Edifice (BUILD) | Resource (EXPLOIT) | inutilisé (CLEAR) */
    int        days_total, days_done;
    bool       active;
} BuildOrder;

#define SCPS_MAX_BUILDS 512
typedef struct {
    BuildOrder order[SCPS_MAX_BUILDS];
    int        n;
    int        day;       /* compteur de partie (jours écoulés) */
} AgencyState;

void agency_init(AgencyState *a);
/* Met une action en file (false si pleine). NU — sans coût (bas niveau / bancs d'essai). */
bool agency_order_build  (AgencyState *a, int region, Edifice e);

/* Bâtir EN PAYANT (§1) : achète la recette AU MARCHÉ (or du trésor régional ; le
 * marché est consommé), PUIS enfile le chantier. Refuse — pas de chantier — si le
 * trésor ne couvre pas. C'est la voie de la sim vive (IA). */
bool agency_build(AgencyState *a, WorldEconomy *econ, int region, Edifice e);
/* E0.3 — LE TRÉSOR UNIQUE du joueur : même chantier, mais l'or sort du COMPTE
 * fourni (le trésor labor) au lieu du trésor régional — la topbar dit VRAI.
 * gold_acct NULL ≡ agency_build. Les matériaux sortent toujours du marché régional. */
bool agency_build_acct(AgencyState *a, WorldEconomy *econ, int region, Edifice e, long *gold_acct);
/* E2 §13 — édifices GATÉS par l'arbre : Comptoir ← « Comptoirs marchands »,
 * Entrepôt ← « Halles & entrepôts ». Tout le reste est libre. ts NULL = libre
 * (bancs d'essai, voies basses). */
bool   edifice_unlocked (const TechState *ts, Edifice e);
TechId edifice_gate_tech(Edifice e);   /* le nœud qui l'ouvre (TECH_COUNT = libre) — pour l'UI */
/* §4 Défrichement : convertit la terre → food, dérive la SUBSISTANCE locale vers
 * l'agriculture (impérialisme culturel sur la terre), et ronge L en niche
 * forestière (les peuples de la forêt voient leur monde rasé). */
bool agency_order_clear  (AgencyState *a, int region);
/* §3 Exploitation : un aménagement (mine/carrière…) monte l'extraction d'une
 * ressource (matériaux pour bâtir/armer, stratégiques pour la tech/valeur). */
bool agency_order_exploit(AgencyState *a, int region, Resource res);
/* §reloc (sidebar Démographie) : ORDONNE le déplacement d'un ensemencement de pop
 * (RELOC_POP familles) de `region` (source) vers `dst_region` — en JOURS, comme tout
 * ordre. À terme : econ_relocate_pop (la coercition monte à la source — le coût,
 * affiché AVANT). Joueur et IA passent par le MÊME actionneur, jamais l'appel direct. */
bool agency_order_relocate(AgencyState *a, int region, int dst_region);
/* E1 — COLONISER PREND 180 JOURS : le convoi part (payé à l'ordre), la région
 * n'est peuplée qu'à l'arrivée (econ_colonize_from au terme — 100 colons depuis
 * src). Si src a changé de couronne entre-temps, le convoi se perd (abandon). */
bool agency_order_colonize(AgencyState *a, int dst_region, int src_region);
#define AGY_RELOC_POP 300   /* familles déplacées par ordre (l'ensemencement mesuré) */
/* Annule un ordre encore en cours (index dans order[]) — la file est VISIBLE (struct
 * publique) et un chantier non fini se révoque. */
bool agency_cancel(AgencyState *a, int idx);

/* ── LES TROIS LEVIERS INTÉRIEURS (brief leviers §2) — des ordres en jours, des
 * coûts SCPS différés. Aucun n'est gratuit, aucun n'est instantané. ──────────── */
/* MATER (30 j) : la botte — province_apply_coercion (Kuran : l'agitation se TAIT,
 * le grief est MASQUÉ et ressortira amplifié à la levée). H s'écrit (différé). */
bool agency_order_repress(AgencyState *a, int region);
/* FORMER (1 an) : accélère l'assimilation du plus gros groupe minoritaire — écoles,
 * missions, magistrats. `creuset` (tech Droit d'intégration) double l'efficacité.
 * Coercition modérée ; le groupe en formation a l'humeur dégradée le temps de la
 * conversion. NB : former une source de savoir, c'est TARIR son canal syncrétique. */
bool agency_order_assimilate(AgencyState *a, int region, bool creuset);
/* PURGER (4 ans, par TRANCHES annuelles visibles — arrêtable en cours au prix du
 * gâchis) : les pops du plus gros groupe minoritaire MEURENT (fraction/an), la
 * stabilité plonge (cicatrice + L au plancher + coercition totale), la fracture,
 * H et la CHARGE faustienne s'écrivent (différé → la Brèche se rapproche). L'acte
 * le plus faustien du panneau — il se nomme, sans euphémisme. */
bool agency_order_purge(AgencyState *a, int region);
#define AGY_PURGE_YEARS 4
/* Coûts SCPS DIFFÉRÉS des leviers (charge/fracture/H par pays), à DRAINER par le
 * harnais chaque jour vers TechState (agency ne connaît pas ts — séparation). */
bool agency_drain_levier_costs(int cid, float *charge, float *fracture, float *H);
/* Chronique des leviers (cumul sim, RAZ par agency_init) : matages, formations,
 * purges achevées, morts de purge. */
void agency_levier_stats(int *repress, int *assim, int *purges, long *purge_dead);
/* sauvegarde : les statiques du module (coûts différés + chronique des leviers). */
void agency_save(FILE *f);
bool agency_load(FILE *f);
/* DIAGNOSTIC G0.3 — dump par édifice (bâtis / bloqués au palier / sans or) sur stderr. */
void agency_edi_dump(void);

/* ── M3 (forks §8) — LA SUCCESSION CONTEXTUELLE + L'HYSTÉRÉSIS ──────────────
 * edifice_fork_successor : la variante de `base` sous un pôle (EDIFICE_COUNT si
 * base non forkée). edifice_region_pole : le pôle VALIDÉ d'une région (lu des
 * factions de SA population ; un candidat divergent doit tenir ≥ 360 j — champs
 * RegionEconomy.last_pole/pole_since_day). edifice_succ_ctx : les deux composées.
 * Un fork bâti ne se reconvertit JAMAIS (les frères sont bloqués — voir
 * edifice_build_blocked). Entrepôt et Comptoir restent UNIVERSELS. */
#include "scps_factions.h"   /* TechPole (le pôle vient des factions) */
Edifice  edifice_fork_successor(Edifice base, TechPole pole);
TechPole edifice_region_pole(const World *w, WorldEconomy *econ, int region, int day);
Edifice  edifice_succ_ctx(const World *w, WorldEconomy *econ, int region, Edifice base, int day);

/* Avance de `days` jours : progresse les chantiers ; à l'achèvement, applique
 * l'effet (déplace une coordonnée que le moteur LIT). `drift` (pile de dérive
 * démographique) porte la falsification Kuran des leviers — nullable (démos). */
void agency_advance(AgencyState *a, World *w, WorldEconomy *econ,
                    WorldLegitimacy *wl, ModifierStack *drift, int days);
/* Nombre de chantiers actifs sur une région (pour l'UI). */
int  agency_active_in_region(const AgencyState *a, int region);
int  agency_year(const AgencyState *a);

#endif /* SCPS_AGENCY_H */
