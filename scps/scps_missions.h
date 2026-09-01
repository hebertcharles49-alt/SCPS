#ifndef SCPS_MISSIONS_H
#define SCPS_MISSIONS_H
/*
 * scps_missions.h — LES DESSEINS (docs/DESIGN_MISSIONS_DOCTRINES.md §2,
 * contenu des branches : docs/DESIGN_DESSEINS_ANNEXE.md)
 *
 * ── LA COMMISSION DÉCENNALE EST DÉPOSÉE (décision joueur 2026-09-01,
 *    « engagements décennaux chiants ») ────────────────────────────────────
 * Plus d'émission tous les dix ans, plus d'or de récompense (le canal violait
 * de toute façon la règle d'or §2.4 : JAMAIS d'or créé), plus de loyauté de
 * siège gagnée/perdue au calendrier, plus de HeroMissionBonus. Le module garde
 * son NOM et sa section de save (MISS) — il porte désormais les DESSEINS.
 *
 * ── CE QU'EST UN DESSEIN ─────────────────────────────────────────────────
 * Un arbre d'AMBITIONS instancié sur la géographie RÉELLE de la graine : une
 * BRANCHE = 8 ÉCHELONS en cascade, chacun visant un objet NOMMÉ du monde
 * (cette province-ci, ce rival-là). L'échelon N accompli ARME le N+1. Aucun
 * échec, aucune date limite : le Dessein ATTEND.
 *
 *   1. la CIBLE se résout déterministiquement sur le monde (géographie pure —
 *      la branche du Sol ne tire AUCUN xs32) ;
 *   2. la CONDITION est un prédicat d'état RÉEL (prov[pid].owner, jamais
 *      region[].owner qui est un agrégat dérivé) ;
 *   3. remplie, l'échelon devient PRÊT — le JOUEUR le SCELLE (CMD_SEAL_DESSEIN) ;
 *   4. le scellage verse la récompense (revendication nommée, coordonnée bâtie,
 *      remise DATÉE, personnage) — jamais un lingot d'or.
 *
 * ── P1 : LE JOUEUR SEUL ──────────────────────────────────────────────────
 * La génération, la résolution et le scellage sont gatés `human_player` : la
 * chronique (human=-1) ne génère RIEN et le golden reste intact PAR
 * CONSTRUCTION (motif décrets). La structure est par-pays quand même — la
 * symétrie IA est une vague à part (§2.7).
 *
 * ── LE CANAL DATÉ (annexe N2 / D1.1) ─────────────────────────────────────
 * Il n'existe pas dans le moteur ; on a pris le plus simple qui marche : un
 * LATCH D'ANNÉE de scellage (motif AgesState.year_eligible[]).
 *   dessein_mult = m  si (année − sealed_year) < DESSEIN_BOON_YEARS, sinon 1.0
 * Zéro accumulateur décrémenté, zéro tick, sérialisation triviale.
 * ⚠ `Modifier.expires_tick` est DÉCLARÉ mais NON APPLIQUÉ — interdit de s'y fier.
 */
#include "scps_world.h"      /* World */
#include "scps_econ.h"       /* WorldEconomy, ProvinceEconomy, ProvBuild */
#include "scps_diplo.h"      /* DiploState : rancune, vassalité, revendications */
#include "scps_statecraft.h" /* Statecraft : le Conseil (embauche gratuite) */

/* ── LES BRANCHES ────────────────────────────────────────────────────────
 * P1 ne livre que la branche du SOL (le pilote qui prouve le framework —
 * elle est TOUJOURS attribuée). Les six autres (Mer & Comptoirs, Routes &
 * Caravanes, Foi, Savoir, Creuset, Horde) sont des vagues suivantes : leur
 * ajout ici fera grandir MissionsState ⇒ bump SAVE_VERSION à ce moment-là. */
typedef enum { DESS_SOL = 0, DESS_BRANCH_COUNT } DesseinBranch;

/* ── LES ÉCHELONS DE LA BRANCHE DU SOL (noms courts, passe de style D6) ───
 * Tronc 0-2 · PIVOT 3 · quatre échelons par voie (4-7). Le PARACHÈVEMENT est
 * l'échelon 7, quelle que soit la voie.
 *
 *   voie CONQUÊTE (1)          voie VASSALISATION (2)
 *   4 Pacification             4 Premier vassal
 *   5 Les marches              5 Trois vassaux
 *   6 Capitale rivale          6 Intégration
 *   7 Hégémonie                7 Hégémonie                                  */
enum { DESS_RUNG_UNIFICATION = 0, DESS_RUNG_EXPANSION, DESS_RUNG_RIVAL,
       DESS_RUNG_PIVOT, DESS_RUNG_4, DESS_RUNG_5, DESS_RUNG_6, DESS_RUNG_7,
       DESSEIN_RUNGS };
enum { DESS_VOIE_AUCUNE = 0, DESS_VOIE_CONQUETE, DESS_VOIE_VASSALISATION };

/* Le SLOT D'AFFICHAGE d'un échelon (0..11) : le tronc et le pivot partagent
 * leurs quatre premiers slots ; les deux voies occupent 4-7 (conquête) et
 * 8-11 (vassalisation). C'est l'index des bandes STR_DESS_SOL_*. */
#define DESSEIN_DISPLAY_SLOTS 12

/* Prix UNIFORME d'un pivot (D6.4 : 20 d'influence, plat, aucune modulation
 * d'éthos, aucune réduction par doctrine sœur). */
#define DESSEIN_PIVOT_INFLUENCE 20

/* ── L'ÉTAT D'UNE BRANCHE ────────────────────────────────────────────────
 * Tout est BORNÉ et revalidé au chargement (scps_save_sane). `tpid`/`trio`
 * portent des index de PROVINCE (voie conquête) ou de PAYS (voie
 * vassalisation) selon l'échelon — la sémantique est fixée par l'échelon,
 * jamais devinée. -1 = « pas de cible » (l'échelon PATIENTE, il ne rate pas). */
typedef struct {
    int8_t  gen;        /* 1 = la branche est GÉNÉRÉE pour ce pays (joueur seul en P1) */
    int8_t  rung;       /* échelon COURANT (le prochain à sceller) ; == DESSEIN_RUNGS ⇒ branche ACHEVÉE */
    int8_t  voie;       /* DESS_VOIE_* — posée au scellage du pivot, IRRÉVERSIBLE */
    int8_t  ready;      /* 1 = la condition de l'échelon courant est REMPLIE (attend le sceau du joueur) */
    int8_t  proof_a;    /* LATCH « preuve d'usage » de la voie Conquête (une province arrachée par traité) */
    int8_t  proof_b;    /* LATCH « preuve d'usage » de la voie Vassalisation (au moins un vassal) */
    int8_t  claim_pend; /* 1 = une revendication de Dessein ATTEND que le slot fab_state se libère */
    int16_t claim_pid;  /* la province visée par cette revendication en attente (-1 = aucune) */
    int16_t rival;      /* le pays RIVAL, latché à l'échelon 2 (-1 = pas encore nommé) */
    int16_t tpid[DESSEIN_RUNGS];        /* cible PROVINCE de chaque échelon (-1 = non résolue) */
    int16_t trio[3];                    /* échelon 5 : 3 provinces (Conquête) ou 3 pays (Vassalisation) */
    int16_t sealed_year[DESSEIN_RUNGS]; /* l'AN du scellage — LE canal daté (-1 = non scellé) */
} Dessein;

#define SCPS_MISSIONS_MAX SCPS_MAX_COUNTRY
typedef struct {
    Dessein d[SCPS_MISSIONS_MAX][DESS_BRANCH_COUNT];
} MissionsState;

/* ── LES RÉCOMPENSES DATÉES (le canal N2) ────────────────────────────────
 * Une clé du registre J par échelon porteur. Le site de LECTURE moteur
 * applique `tune_f("CLÉ", défaut) × dessein_mult(cid, DBOON_…)` — jamais
 * tune_set (global, IA comprise). Le mult est CLAMPÉ [0.60, 1.60] (H2bis
 * étendu aux Desseins) au site de lecture, ici même. */
typedef enum {
    DBOON_FAB_VALID_DAYS = 0,   /* échelon 2 « Le rival »        : ×1.60 — les torts consignés */
    DBOON_ANNEX_SOFT_SCAR,      /* échelon 4A « Pacification »   : ×0.75 */
    DBOON_ANNEX_YEARS_PER_PRICE,/* échelon 5A « Les marches »    : ×0.80 */
    DBOON_OPINION_VASSAL,       /* échelon 4B « Premier vassal » : ×1.30 */
    DBOON_ANNEX_MIN_INTEGRATION,/* échelon 5B « Trois vassaux »  : ×0.85 */
    DBOON_COUNT
} DesseinBoon;

/* LE SITE DE LECTURE. Appelable depuis n'importe quel module du moteur (le
 * miroir statique interne est rafraîchi à chaque clôture par missions_tick et
 * au chargement par missions_boons_sync) — même contrat que decree_mult :
 * 1.0 si rien n'est scellé, si la fenêtre est passée, ou si
 * DESSEIN_BOON_YEARS vaut 0 (kill-switch). */
float dessein_mult(int cid, DesseinBoon k);

/* RAFRAÎCHIT le miroir statique lu par dessein_mult depuis l'état sérialisé.
 * Appelé en tête de missions_tick ET juste après un chargement (le miroir est
 * un cache de process, jamais sérialisé : sans ce rappel, une partie rechargée
 * verrait ses remises muettes jusqu'à la première clôture — divergence au
 * savetest). */
void missions_boons_sync(const MissionsState *ms, int year);

/* ── LE PRIX DU PIVOT ────────────────────────────────────────────────────
 * BRANCHÉ SUR L'INFLUENCE AU MERGE : le module d'INFLUENCE POLITIQUE (§3 du
 * design — 0.002/noble × Conseil) n'existe pas dans cet arbre. Le stub accepte
 * toujours ; l'orchestrateur y câble le débit de DESSEIN_PIVOT_INFLUENCE (20,
 * plat). C'est le SEUL point de contact à reprendre. */
bool dessein_pivot_pay(int cid);

void missions_init(MissionsState *ms);

/* CLÔTURE MENSUELLE (JOUEUR SEUL — `human_player` < 0 ⇒ no-op total) :
 *   1. génère la branche du Sol du joueur si elle ne l'est pas encore ;
 *   2. LATCHE les preuves d'usage du pivot ;
 *   3. RE-RÉSOUT la cible de l'échelon courant si elle a été DÉTRUITE
 *      (l'invalidation est la DESTRUCTION, jamais l'inconvénient : une cible
 *      passée à un allié RESTE la cible — il faudra la prendre) ;
 *   4. relance une revendication RETENUE dont le slot s'est libéré ;
 *   5. teste la CONDITION et pose `ready` (le sceau reste au joueur).
 * JAMAIS appelée pendant un chargement : prov_adj est un pointeur tas rebâti,
 * et la première clôture post-load fait le travail. */
void missions_tick(MissionsState *ms, World *w, WorldEconomy *econ,
                   DiploState *dp, int year, int human_player);

/* SCELLE l'échelon courant de la branche (le verbe CMD_SEAL_DESSEIN, drainé).
 * `voie` n'est lu QUE pour le pivot (DESS_VOIE_CONQUETE/VASSALISATION) et exige
 * la preuve d'usage correspondante + dessein_pivot_pay. Revalide TOUT contre
 * l'état courant (miroir save_sane) : branche générée, échelon bien le courant,
 * condition remplie. Verse la récompense, avance l'échelon et résout la cible
 * du suivant. Renvoie 1 si le sceau a pris, 0 sinon (silencieux). */
int  missions_seal(MissionsState *ms, World *w, WorldEconomy *econ,
                   DiploState *dp, Statecraft *sc, uint32_t seed, int year,
                   int cid, int branch, int rung, int voie);

/* ── LECTEURS PURS (façade + bancs) ──────────────────────────────────────── */
const Dessein *dessein_of(const MissionsState *ms, int cid, int branch);
/* Le SLOT D'AFFICHAGE (0..11) de l'échelon `rung` sous la voie `voie`. */
int  dessein_display_slot(int rung, int voie);
/* La cible PROVINCE de l'échelon courant (-1 si aucune / si l'échelon vise un pays). */
int  dessein_target_pid(const Dessein *d);
/* La cible PAYS de l'échelon courant (-1 si aucune / si l'échelon vise une province).
 * `dp`/`cid` servent au seul échelon 6B (« Intégration ») dont la cible est
 * RÉÉVALUÉE à chaque lecture : celui de mes trois vassaux dont v_integration est
 * la plus haute — la cible est CELUI QUI MÛRIT, jamais un index latché. */
int  dessein_target_cid(const Dessein *d, const DiploState *dp, int cid);
bool dessein_is_pivot (int rung);
bool dessein_is_finale(int rung);   /* le PARACHÈVEMENT (échelon 7) : Annale + épithète + Âge des Héros */
/* Le SIÈGE du Conseil qui porte la branche (la branche du Sol est territoriale
 * ⇒ Royaume/1). C'est lui que l'Âge des Héros interroge au parachèvement, et
 * lui que l'échelon 6 pourvoit gratuitement. -1 = branche inconnue. */
int  dessein_seat_of(int branch);

#endif /* SCPS_MISSIONS_H */
