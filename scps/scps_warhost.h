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
 * guerre ; la force se dépose en armes sur sa capitale (→ mil_power). */
void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, const TechState *ts, float dt_years);  /* ts[SCPS_MAX_COUNTRY] : F8 gate de variété */

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
                            const TechState *ts, int cid, UnitType t, long packs);

/* DÉMOBILISER la réserve levée (§4) : l'armée du pays se dissout, la jauge retombe
 * à GARDE (sinon le pied de guerre re-lève aussitôt). LOT 2 — aligné sur wh_shed (le
 * downsizing NATUREL de paix) : les ARMES (chaque RES_ARMS macro consommé à la levée)
 * sont RENDUES au stock macro de l'empire (econ_region_stock_add), pas perdues — le
 * disband joueur n'est plus un puits d'or silencieux. econ peut être NULL (repli :
 * armes perdues, ancien comportement — utile aux bancs qui n'ont pas d'économie sous
 * la main). Renvoie les paquets dissous. */
long warhost_disband(WarHost *h, WorldEconomy *econ, int cid);

#endif /* SCPS_WARHOST_H */
