#ifndef SCPS_SAVE_H
#define SCPS_SAVE_H
/*
 * scps_save.h — LA SAUVEGARDE (extraite de viewer.c) : en-tête versionné + sections
 * TAGUÉES, chiffrement ChaCha20 (scps_crypt), écriture atomique (scps_save_io).
 * PARTAGÉE par le viewer SDL ET la façade Godot (scps_api) : un seul format, une
 * seule vérité. Le moteur reste C ; ce module ne fait qu'aspirer/restaurer l'état.
 *
 * Le front fournit son IDENTITÉ DE CULTURE de setup (setup_race/setup_ethos) — le
 * viewer ses globals g_player_race/g_setup_ethos, la façade le slot 0 du joueur —
 * pour que la membrane de SaveMisc reste neutre (aucun global de front ici).
 *
 * ── Historique des versions (bump = struct sérialisée plus large ⇒ « ère antérieure ») ──
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
#define SAVE_VERSION 38u
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
/* sauve la partie ENTIÈRE (atomique). setup_race∈[0,HERITAGE_COUNT)·setup_ethos∈[0,ETHOS_COUNT)
 * = l'identité de culture du front (mémorisée dans SaveMisc). false si l'écriture échoue. */
bool scps_save_game(int slot, World *w, Sim *s, const WorldParams *params, int setup_race, int setup_ethos);
/* charge un slot : 0 ok · 1 absent/corrompu · 2 « ère antérieure » (version). out_race et
 * out_ethos ← l'identité de setup restaurée (le front y remet ses globals) ; NULL si indifférent. */
int  scps_load_game(int slot, World *w, Sim *s, WorldParams *params, int *out_race, int *out_ethos);
/* garde-fou post-chargement : revalide tous les comptes/indices désérialisés (refus net). */
bool scps_save_sane(const World *w, const Sim *s, int player);

#endif /* SCPS_SAVE_H */
