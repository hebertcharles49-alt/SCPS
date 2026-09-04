#ifndef SCPS_WARHOST_H
#define SCPS_WARHOST_H
/*
 * scps_warhost.h — LES ARMÉES VIVENT : la mobilisation par pays
 *
 * scps_army (recrutement, armes, pierre-feuille-ciseaux) était complet mais
 * n'était branché à AUCUN tick : les armées existaient en tant que données, sans
 * vivre. Ce module les fait vivre dans la boucle.
 *
 * Chaque pays porte une ArmyState. Une fois l'an, sur PIED DE GUERRE (en guerre ou
 * menacé) il MOBILISE : il fabrique des armes et lève des unités depuis sa pop &
 * ses matériaux (semés à bas coût depuis l'économie de région). La force levée se
 * DÉPOSE en armes (RES_ARMS) sur sa capitale → elle nourrit diplo_mil_power, SANS
 * réécrire la guerre. Boucle de guerre-économie : l'attrition (diplo_war_tick)
 * saigne les armes, la mobilisation les renouvelle pour qui est sur le pied de
 * guerre — la paix démobilise.
 *
 * Membrane : warhost_units renvoie un nombre tangible (paquets de 100).
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_army.h"
#include "scps_labor.h"
#include "scps_diplo.h"
#include "scps_campaign.h"   /* Campaign : le pool de levée compte les corps au front (§4.2) */

typedef struct {
    ArmyState  army[SCPS_MAX_COUNTRY];   /* l'armée levée de chaque pays (persiste) */
    int        levy[SCPS_MAX_COUNTRY];   /* jauge de LEVÉE (sidebar §5) : 0 basse · 1 garde · 2 guerre · 3 masse */
    /* (le `scratch` LaborEcon a disparu : la levée LIT désormais les strates econ du pays.) */
} WarHost;

/* Jauge de levée (décision joueur/IA) : module la cadence de mobilisation. La LEVÉE
 * EN MASSE (3) force la main des familles → coercition à la capitale (le coût,
 * affiché AVANT). Tout est en mots/paliers — aucune coordonnée ne sort. */
#define WH_LEVY_BASSE  0
#define WH_LEVY_GARDE  1
#define WH_LEVY_GUERRE 2
#define WH_LEVY_MASSE  3
void warhost_set_levy(WarHost *h, int cid, int levy);
int  warhost_levy    (const WarHost *h, int cid);
const char *warhost_levy_name(int levy);

void warhost_init(WarHost *h);
void warhost_free(WarHost *h);

/* Mobilisation (dt en ANNÉES) : chaque pays vivant lève des troupes ∝ son pied de
 * guerre ; la force se dépose en armes sur sa capitale (→ mil_power).
 * `cmp` (NULLABLE) : la CAMPAGNE, lue en SEULE LECTURE pour que le pool de recrutement
 * voie les corps partis au front — sans elle, un corps en campagne vide l'affectation
 * du host et le pays relève sa population une seconde fois (CALIB_ARMEE §4.2). NULL =
 * pays sans corps déployé (bancs) : comptabilité du host seul.
 * A4 (2026-09-04) — `cmp` est désormais MUTABLE : la solde facture AUSSI les corps au
 * front (WH_PAY_CORPS) et la désertion faute de solde y fond au prorata. Un régiment
 * parti en campagne n'est plus gratuit. */
void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, const TechState *ts, Campaign *cmp,
                  float dt_years);  /* ts[SCPS_MAX_COUNTRY] : F8 gate de variété */

long warhost_units (const WarHost *h, int cid);   /* paquets de 100 levés (UI/IA) */

/* L'ANCRE EU4 (mission solde 2026-07-06) : l'entretien mensuel d'UN régiment = son
 * prix de recrutement / 13 — or (REGIMENT_PRICE × unit_pay_mult × IPM) + armes
 * consommées à la levée (100 armes macro au prix de `price_region` — passer la
 * région-capitale du pays : prix NATIONAL P1). Lu par le moteur (warhost_tick),
 * la chronique (diags) et l'UI — un seul point de vérité du prix payé. */
float warhost_unit_pay_month(const WorldEconomy *econ, int price_region, UnitType t);
/* LA LIMITE DE FORCE (lecture EU4) : combien de régiments un pays de `n_regions`
 * entretient à prix plein — au-delà, l'intendance renchérit chaque régiment. */
float warhost_force_limit(int n_regions);
/* AUDIT DU GOULOT D'ARMES (SCPS_ARMSDIAG) : expose les compteurs de levée par
 * Resource (armes voulues / prises à l'arsenal / paquets ×100 levés après le gate
 * pop / rendues à la démob). Diagnostic pur — jamais lu par le moteur. */
void warhost_armsdiag(const long **want, const long **got, const long **levied, const long **returned);
/* LE FREIN ÉCONOMIQUE DE LA LEVÉE, COMPTÉ (2026-09-03) : paquets ×100 partis faute de solde
 * (WH_DESERT_RATE) · mois-pays au-dessus du plafond de solde (WH_PAY_REVENUE_FRAC) · mois-pays
 * observés (le dénominateur). Diagnostic pur — jamais lu par le moteur, RAZ par warhost_init. */
void warhost_braking_stats(long *deserted, long *overbudget_months, long *checked_months);
/* LA RAISON DU REFUS DE LEVÉE (2026-09-04, P3 du sweep W1/W2 — PRINT-ONLY) : sans elle,
 * « le 1er empire du monde à 0 régiment avec 184 577 or et 127 329 armes lourdes » reste
 * une devinette. Un code par pays et par an (le DERNIER passage de warhost_tick), plus les
 * cumuls pays-an du monde. Diagnostic pur — aucune décision moteur n'en dépend, RAZ par
 * warhost_init (par sim), exactement comme ARMSDIAG et le frein. */
enum { WHR_LEVE=0,          /* la levée a rendu des paquets */
       WHR_COMPLET,         /* rien à lever : la garnison de paix est atteinte (ou on dégraisse) */
       WHR_BUDGET,          /* la solde ne suit plus (trésor < 3 mois, ou au-dessus du revenu) */
       WHR_ARMES,           /* l'arsenal n'a pas donné un seul paquet d'armes DU TYPE voulu */
       WHR_POOL,            /* armes prises, mais la classe n'a plus d'hommes disponibles */
       WHR_SANS_CAPITALE,   /* capital_prov < 0 : le pays ne lève jamais rien, à vie */
       WHR_SANS_REGION,     /* aucune région : hors du tick */
       WHR_JOUEUR,          /* main humaine : c'est le joueur qui compose */
       WHR_COUNT };
int  warhost_levy_reason(int cid);                 /* dernier code vu par ce pays (-1 = jamais vu) */
const char *warhost_levy_reason_name(int code);    /* le MOT (outillage console, français) */
/* `par_code` : SCPS_MAX pays-an par code · `elite_gated` : pays-an où le gate d'élite (≤200
 * aristocrates) a rayé au moins une unité voulue · `sans_revenu` : pays-mois où le revenu
 * fiscal était nul, donc où le plafond WH_PAY_REVENUE_FRAC était DÉSARMÉ · `croissance_hors_limite` :
 * pays-an où la levée de guerre a grossi une armée DÉJÀ au-dessus de sa limite de force. */
void warhost_levy_reason_stats(const long **par_code, long *elite_gated,
                               long *sans_revenu, long *croissance_hors_limite);
/* LA PART DES CORPS DANS LA SOLDE (2026-09-04, A4 · PRINT-ONLY) : fraction [0..1] de la
 * solde du dernier tick de `cid` imputable aux CORPS DE CAMPAGNE (le reste = le host).
 * Le barème et les multiplicateurs étant les MÊMES des deux côtés, la part est celle des
 * assiettes typées — la chronique en déduit la part en or sans recalculer le moteur.
 * 0 si le pays n'a rien au front ou si WH_PAY_CORPS=0. RAZ par warhost_init. */
float warhost_corps_pay_share(int cid);

/* Affinité ÉTHOS→unité (0-3) de la table AFF — read-only, pour l'UI de construction
 * (« quel éthos favorise cette unité »). N'influe sur rien : pure lecture. */
float warhost_unit_affinity(int faction, int unit);

/* MAIN HUMAINE : désigne le pays JOUEUR — warhost_tick cesse de mobiliser/démobiliser
 * son armée tout seul (l'humain la compose au panneau ; il paie toujours la solde).
 * -1 = aucun (l'IA gère tout). Remis à -1 par warhost_init (chronique inchangée). */
void warhost_set_human(int cid);
/* ACTION JOUEUR : lève `packs` paquets d'un TYPE d'unité choisi (verbe absent de l'IA,
 * qui compose par AFF). Gates : tech, classe (élite), armes en stock. Renvoie le levé. */
long warhost_player_recruit(WarHost *h, const World *w, WorldEconomy *econ,
                            const TechState *ts, const Campaign *cmp,
                            int cid, UnitType t, long packs);

/* DÉMOBILISER la réserve levée (§4) : l'armée du pays se dissout, la jauge retombe
 * à GARDE (sinon le pied de guerre re-lève aussitôt). LOT 2 — aligné sur wh_shed (le
 * downsizing NATUREL de paix) : les ARMES (chaque RES_ARMS macro consommé à la levée)
 * sont RENDUES au stock macro de l'empire (econ_region_stock_add), pas perdues — le
 * disband joueur n'est plus un puits d'or silencieux. econ peut être NULL (repli :
 * armes perdues, ancien comportement — utile aux bancs qui n'ont pas d'économie sous
 * la main). Renvoie les paquets dissous. */
long warhost_disband(WarHost *h, WorldEconomy *econ, int cid);

#endif /* SCPS_WARHOST_H */
