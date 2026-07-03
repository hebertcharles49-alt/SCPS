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
#define SAVE_VERSION 54u           /* v54 : FOI PAR GROUPE — PopGroup gagne `faith` (id de religion PORTÉ par le
                                    * groupe ; la foi descend au niveau de la pop comme la culture : migration la
                                    * porte, une région mêle les cultes, hérésie = groupe dissident). g_region_religion
                                    * devient un CACHE dérivé (culte dominant). ⚠ sizeof(PopGroup)→sizeof(WorldEconomy)
                                    * change (fwrite BRUT) → <v54 refusé. v53 : COOLDOWN DE RÉVOLTE PROPRE + DIMENSION FOI — RevoltState gagne `revolt_cooldown[SCPS_MAX_REG]` (le répit post-révolte, champ SÉPARÉ du compteur de misère desperation_days : l'ancien sentinel négatif surchargé était effacé au moindre calme → boucle écrasement/rallumage → révolte perpétuelle) ET les compteurs `n_heresy`/`n_zelote` (révolte de FOI : hérésie = schisme de la foi d'État ; zélote = culte étranger — reconvertis si écrasés, tolérés si vainqueurs) ⇒ sizeof(RevoltState) change → <v53 refusé. v52 : RÉFUGIÉS — PopGroup gagne `home_reg` (RÉGION d'origine d'un déplacé — migrant/réfugié — le foyer où RESPIRER : retour partiel quand il s'apaise, AUCUNE migration définitive) ⇒ sizeof(PopGroup) → sizeof(WorldEconomy) change (fwrite BRUT) → <v52 refusé. v51 : BRASSAGE — PopGroup gagne `arrival` (mode d'arrivée natif/migrant/soumis/déporté → coeff de diffusion du savoir à la métabolisation) ; le blob WorldEconomy est un fwrite BRUT donc son padding change de sémantique → <v51 refusé. v50 : CADENCE DE COLONISATION — WorldEconomy gagne colony[SCPS_MAX_COUNTRY] (le chantier joueur : une colonie MÛRIT ~1 an frontalier, rendement log-distance capitale, 1 ordre/an) ⇒ sizeof(WorldEconomy) change ; + SaveMisc.diplo_ready_day (le DIPLOMATE : 1 acte / 2 mois) ⇒ sizeof(SaveMisc) change → <v50 refusé. v49 : CONSEIL À GÉNÉRATIONS — Statecraft gagne council_gen[][] (la génération du ministre ASSIS : identité épinglée, il VIEILLIT — âge dérivé base 30-51 + années — et part à la retraite 66-73 ; la pool de candidats tourne par génération de 20 ans, toujours 3 candidats dispo) ⇒ sizeof(Statecraft) change → <v49 refusé. v48 : section WILD (compteurs de contact des hameaux libres — le ralliement est du GAMEPLAY, un load en processus frais le remettait à zéro) + SaveMisc.player_age_engaged (§7 : l'engagement d'âge du joueur, verbe CMD_AGE_ENGAGE) ⇒ sizeof(SaveMisc) change → <v48 refusé. v47 : ÉCONOMIE PAR-PROVINCE — WorldEconomy.prov[]+n_prov (vérité) + prov_adj (heap, rebuilt) ; region[] agrégat. v46 : APEX TRIPLES — +3 nœuds tier-5 (fusion de 3 héritages) ⇒ TECH_COUNT grandit (TechState) ; + ArmyDoctrine.firearm_power (ArmyState block sérialisé sizeof). v45 : COMBOS — +14 nœuds de tech tier-4 (fusion de 2 héritages) ⇒ TECH_COUNT grandit → TechState.unlocked[TECH_COUNT] change de taille. v44 : ÉTOFFE — +12 nœuds de tech (branches culturelles d'héritage tier 1-2) ⇒ TECH_COUNT grandit → TechState.unlocked[TECH_COUNT] change de taille. v43 : RES_METAL SUPPRIMÉ (outils = fer+bois DIRECT ; coques = cuivre) ⇒ RES_COUNT change → tableaux [RES_COUNT] de RegionEconomy rétrécissent. v42 : ALLOCATION main-d'œuvre. v41 : EXPLOITATION (raw_boost) */
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
