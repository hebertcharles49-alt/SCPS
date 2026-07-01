#ifndef SCPS_SAVE_H
#define SCPS_SAVE_H
/*
 * scps_save.h — LA SAUVEGARDE (extraite de viewer.c) : en-tête versionné + sections
 * TAGUÉES, chiffrement ChaCha20 (scps_crypt), écriture atomique (scps_save_io).
 * PARTAGÉE par le viewer SDL ET la façade Godot (scps_api) : un seul format, une
 * seule vérité. Le moteur reste C ; ce module ne fait qu'aspirer/restaurer l'état.
 *
 * Le front fournit son IDENTITÉ DE CULTURE de setup (setup_heritage/setup_ethos) — le
 * viewer ses globals g_player_heritage/g_setup_ethos, la façade le slot 0 du joueur —
 * pour que la membrane de SaveMisc reste neutre (aucun global de front ici).
 *
 * ── Historique des versions (bump = struct sérialisée plus large ⇒ « ère antérieure ») ──
 * v47 : ÉCONOMIE PAR-PROVINCE — WorldEconomy gagne prov[SCPS_MAX_PROV]+n_prov (LA VÉRITÉ
 *       économique, charte PROVINCE_MODEL.md) ; region[] n'est plus qu'un AGRÉGAT ; +prov_adj
 *       (POINTEUR TAS, adjacence de provinces — JAMAIS sérialisé comme adresse valide : écrasé
 *       à NULL puis REBÂTI par econ_build_adjacency au chargement, avec region_rep_prov).
 *       sizeof(WorldEconomy) ~2.5 Mo → ~7.9 Mo. <v47 refusé.
 * v39 : LETTRÉS — la section RELG gagne g_scholar[] (agents religieux déployés). <v39 refusé.
 * v38 : RELIGION par RÉGION — la section RELG gagne g_region_religion[] (fracture/sécession). <v38 refusé.
 * v37 : RELIGION — section RELG (religion_save/load : registre g_religions + liens pays→religion).
 *       État en GLOBAL du module religion (pas de struct partagée touchée). <v37 refusé.
 * v36 : CRÉATEUR DE CULTURE — section CULT (culture_slots_save/load : g_slot[]+map cid→slot)
 *       ⇒ un monde chargé garde les cultures composées (joueur ET IA). <v36 refusé.
 * v35 : MONDE QUI SCALE — plafonds SCPS_MAX_PROV/REG/COUNTRY relevés (le monde suit le nb
 *       d'empires, presets tiny 2…huge 12). v34 : is_capital. v33 : opinion_mem (#26). v32 :
 *       vassalité durée. v31 : tune_ck. v30 : roster 22. v29 : merge des deux lignées. (détail
 *       complet : git log / commits antérieurs.)
 */
#include "scps_sim.h"     /* Sim, World, WorldParams + tous les modules sérialisés */
#include <stdbool.h>
#include <stdint.h>

#define SAVE_MAGIC   0x53504353u   /* "SCPS" */
#define SAVE_VERSION 48u           /* v48 : section WILD (compteurs de contact des hameaux libres — le ralliement est du GAMEPLAY, un load en processus frais le remettait à zéro) + SaveMisc.player_age_engaged (§7 : l'engagement d'âge du joueur, verbe CMD_AGE_ENGAGE) ⇒ sizeof(SaveMisc) change → <v48 refusé. v47 : ÉCONOMIE PAR-PROVINCE — WorldEconomy.prov[]+n_prov (vérité) + prov_adj (heap, rebuilt) ; region[] agrégat. v46 : APEX TRIPLES — +3 nœuds tier-5 (fusion de 3 héritages) ⇒ TECH_COUNT grandit (TechState) ; + ArmyDoctrine.firearm_power (ArmyState block sérialisé sizeof). v45 : COMBOS — +14 nœuds de tech tier-4 (fusion de 2 héritages) ⇒ TECH_COUNT grandit → TechState.unlocked[TECH_COUNT] change de taille. v44 : ÉTOFFE — +12 nœuds de tech (branches culturelles d'héritage tier 1-2) ⇒ TECH_COUNT grandit → TechState.unlocked[TECH_COUNT] change de taille. v43 : RES_METAL SUPPRIMÉ (outils = fer+bois DIRECT ; coques = cuivre) ⇒ RES_COUNT change → tableaux [RES_COUNT] de RegionEconomy rétrécissent. v42 : ALLOCATION main-d'œuvre. v41 : EXPLOITATION (raw_boost) */
#define SAVE_F_CRYPT 1u

typedef struct {
    uint32_t magic, version;
    uint32_t seed;
    int32_t  day, year, player;
    WorldParams params;
    int64_t  stamp;            /* horodatage (time) */
    char     line[96];         /* « An 87 — Empire de X, 12 régions » (écran Charger) */
    uint32_t payload;          /* taille attendue après l'en-tête (intégrité) */
    uint32_t flags;            /* SAVE_F_CRYPT : sections chiffrées (l'en-tête reste en CLAIR) */
    uint64_t nonce;            /* nonce ChaCha20 — unique par sauvegarde */
    uint64_t plain_ck;         /* empreinte FNV-1a du CLAIR : un octet altéré = refus net */
    uint32_t tune_ck;          /* FNV des surcharges SCPS_TUNE résolues (autres règles ⇒ AVERTIT) */
} SaveHeader;

/* chemin du slot (saves/slot_N.scps). */
const char *save_slot_path(int slot);
/* lit l'en-tête CLAIR d'un slot (pour l'écran Charger) ; false si absent/mauvais magic. */
bool scps_save_slot_info(int slot, SaveHeader *out);
/* sauve la partie ENTIÈRE (atomique). setup_heritage∈[0,HERITAGE_COUNT)·setup_ethos∈[0,ETHOS_COUNT)
 * = l'identité de culture du front (mémorisée dans SaveMisc). false si l'écriture échoue. */
bool scps_save_game(int slot, World *w, Sim *s, const WorldParams *params, int setup_heritage, int setup_ethos);
/* charge un slot : 0 ok · 1 absent/corrompu · 2 « ère antérieure » (version). out_heritage et
 * out_ethos ← l'identité de setup restaurée (le front y remet ses globals) ; NULL si indifférent. */
int  scps_load_game(int slot, World *w, Sim *s, WorldParams *params, int *out_heritage, int *out_ethos);
/* garde-fou post-chargement : revalide tous les comptes/indices désérialisés (refus net). */
bool scps_save_sane(const World *w, const Sim *s, int player);

#endif /* SCPS_SAVE_H */
