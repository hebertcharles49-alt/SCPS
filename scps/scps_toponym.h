#ifndef SCPS_TOPONYM_H
#define SCPS_TOPONYM_H
/*
 * scps_toponym.h — TOPONYMIE DES VILLES (docs/DESIGN_TOPONYMIE_VILLES.md).
 *
 * PRINCIPE (doc §4) : sens → novlang → nom. Une ville tire sa localisation
 * (§10, géographie de sa province-ANCRE), un marqueur historique éventuel
 * (§8, un seul, priorité = ordre du tableau), une clé d'éthos éventuelle
 * (§7, ~1/4 de base, renforcée si un état RÉEL le prouve), puis traduit ces
 * clés en morphèmes via le jitter de novlang (§5 : hash(culture_id,clé) 80 %
 * / hash(province_id,clé) 20 %, DÉTERMINISTE — aucun rng_f(), aucun état de
 * flux consommé) et les assemble+lisse (§11-§12).
 *
 * GRAIN RÉGION (doc §1 : « son identité doit appartenir à la région »). La
 * VÉRITÉ vit ici, dans un tableau STATIC de MODULE — motif WILD/EMOB/COLC/
 * TXYR (scps_econ.c) : ni sur World.region[] (aurait fallu toucher
 * scps_types.h, hors périmètre de cette mission) ni sur WorldEconomy.region[]
 * (RegionEconomy est une VUE recomposée à CHAQUE econ_aggregate_regions —
 * un nom posé là serait EFFACÉ au tick suivant sans mirroring explicite).
 * Sérialisée en section TOPO (scps_save.c), motif exact de econ_colony_cd_*.
 *
 * ANCRE (province qui PROUVE la géographie/société de la ville) : la
 * province-CAPITALE de la région si elle en a une, sinon la première
 * province COLONISÉE dans l'ORDRE FIXE Region.province_ids[] (jamais un
 * pointeur dynamique du genre region_rep_prov[]/rep_pid[] qui pourrait
 * faire « migrer » la ville si la démographie régionale bascule — le nom
 * survit à tout, doc §14). Ce choix est figé UNE fois, au moment où le nom
 * naît, puis oublié : seule la CHAÎNE composée est mémorisée.
 *
 * CADENCE : `toponym_world_tick` est un BALAYAGE idempotent — il ne fait
 * QUE combler les régions encore sans nom (jamais de ré-tirage, doc §1/§14).
 * Appelé depuis world_tick (scps_world.c), lui-même invoqué UNE fois/an par
 * la sim (scps_sim.c) : au premier appel qui suit econ_init, TOUTES les
 * capitales de départ déjà colonisées reçoivent leur nom d'un coup — la
 * « genèse » et la « fondation en jeu » sont donc le MÊME mécanisme, pas
 * deux chemins à maintenir séparément.
 */
#include "scps_world.h"   /* World, WorldEconomy, Region, Province, Cell */
#include <stdio.h>
#include <stdbool.h>

#define TOPONYM_NAME_MAX 32

/* Balayage annuel (appelé par world_tick) : nomme toute région dont la
 * province-ancre est colonisée et qui n'a pas encore de nom. Idempotent. */
void toponym_world_tick(World *w, WorldEconomy *econ);

/* RAZ de l'état de MODULE — appelée par world_generate (nouvelle partie /
 * nouvelle Sim dans le même processus, motif g_colony_cd RAZ à econ_init,
 * mais ici au point d'entrée que possède ce module : world_generate). */
void toponym_reset(void);

/* Lecture pure (jamais de génération) : "" si la région n'a pas (encore) de
 * ville. Bornée [0, SCPS_MAX_REG). */
const char *toponym_region_name(int region);

/* Sérialisation — section TOPO (scps_save.c), motif WILD/EMOB/COLC/TXYR :
 * fwrite/fread BRUT du tableau de MODULE, validation (NUL-terminaison) au
 * chargement. */
void toponym_save(FILE *f);
bool toponym_load(FILE *f);

#endif /* SCPS_TOPONYM_H */
