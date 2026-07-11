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
#define SAVE_VERSION 78u           /* v78 : ÂGES SANS ORDRE (2026-07-11, docs/AGES_FINS) — trois
                                    * structs fwrite BRUTES changent de taille : AgesState
                                    * (+year_eligible[8], −tier_open/−research_mult/−integ_mult),
                                    * WorldProsperity (+5 champs age_*), MissionsState
                                    * (+just_completed +hero_bonus[][]). <v78 refusé.
                                    * v77 : ORIENTATIONS POLITIQUES (2026-07-10, scps_decrees.{h,
                                    * c}) — la DÉCISION « Audit des offices » ajoute un cooldown
                                    * PAR PAYS (g_audit_cd[SCPS_MAX_COUNTRY], accumulateur inter-
                                    * ticks — jurisprudence EMOB/COLC/TXYR/RVLT) à la section DCRE
                                    * (fwrite propre au module, pas une struct partagée). Le
                                    * catalogue des 9 orientations légères RENUMÉROTE l'enum
                                    * DecreeId (DECREE_TRIBUT retiré) : g_decree_mask garde la
                                    * MÊME taille mais un save <v77 y lirait des bits au mauvais
                                    * sens ⇒ refusé par le contrôle de version, comme toute ère
                                    * antérieure. <v77 refusé. */
                                   /* v76 : MANUFACTURES SIGNATURE D'ÉTHOS (2026-07-09) — les 6
                                    * biens/6 ateliers du désir croisé (docs/DESIGN_manufactures_
                                    * ethos.md) appendent RES_HEAUMES…RES_OUVRAGES à Resource et
                                    * BLD_HEAUMERIE…BLD_ATELIER_SEREIN à BuildingType ⇒ les tableaux
                                    * [RES_COUNT]/[BLD_TYPE_COUNT] de ProvinceEconomy ET RegionEconomy
                                    * grandissent ⇒ sizeof(WorldEconomy) change (section ECON, fwrite
                                    * BRUT de *s->econ) — un save v75 est INCOMPLET pour v76 (refusé
                                    * par le contrôle de version). Aucune section neuve, aucune
                                    * sémantique de champ existant touchée. <v76 refusé. */
                                   /* v75 : BROUILLARD DE GUERRE (étape 1/2, 2026-07-09) — nouvelle
                                    * section FOGV (fog_save/load, motif WILD/DCRE) : known[SCPS_MAX_
                                    * COUNTRY][SCPS_MAX_COUNTRY] (la connaissance CUMULATIVE qu'un
                                    * empire a d'un autre, radius 2, découverte sur le terrain). Pas de
                                    * struct existante agrandie ⇒ un save v74 est juste INCOMPLET pour
                                    * v75 (refusé par le contrôle de version, comme toute ère
                                    * antérieure). VISUEL seulement : rien dans la sim ne LIT encore
                                    * cette connaissance (câblage IA/diplo = mission séparée) ⇒ golden/
                                    * déterminisme intacts par construction. <v75 refusé.
                                    * v74 : FIN_CHAUD (réchauffement §27, 2026-07-08) — le combustible
                                    * brûlé (bois de feu servi + charbon consommé) est un ACCUMULATEUR
                                    * inter-ticks : cumuls WorldEconomy.fuel_wood_cum/fuel_coal_cum
                                    * (blob ECON grandit) + EndgameState.fuel_seen_wood/fuel_seen_coal/
                                    * fuel_charge/heat_offset (section EGAM grandit) — jurisprudence
                                    * EMOB/COLC/TXYR : non sérialisé ⇒ --savetest diverge. FIN_CHAUD
                                    * appendue à FinType (valeurs existantes stables). <v74 refusé. */
                                   /* v73 : CONTRAT DE SAVE (défaut #1, audit 2026-07-06) — LES
                                    * TROIS GRÂCES DE RÉVOLTE rapatriées SUR RevoltState :
                                    * revolt_grace/coup_grace/concede_cd[SCPS_MAX_COUNTRY] (ex-
                                    * `static float` module-hors-struct dans scps_revolt.c) GATENT
                                    * une décision moteur (allumage empire-wide/coup/concession),
                                    * RAZ seulement par revolt_init (sim_init) et JAMAIS restaurées
                                    * au chargement ⇒ save/reload ≠ continuation (classe EMOB(v57)/
                                    * COLC(v61)/TXYR(v65) — invisible au --savetest same-process,
                                    * capté par l'audit correctness/save). Section RVLT existante
                                    * (fwrite BRUT de *s->rs) ⇒ sizeof(RevoltState) change ; les
                                    * trois tableaux sont revalidés au load (save_sane, ∈[-31,40×365]
                                    * jours — le repos post-expiration est UN pas de scan sous zéro ;
                                    * hors-borne refusé net). <v73 refusé.
                                    * v72 : #32 (LE SANG SIGNE TON RÈGNE) — EndgameState gagne
                                    * war_dead_player/war_dead_player_seen (2 double, jumeau du ratio
                                    * de sang MONDIAL ne comptant que les morts DU joueur) ⇒
                                    * sizeof(EndgameState) change (section EGAM, fwrite BRUT du
                                    * struct entier). */
/* v71 : V2b — LES ÉVÉNEMENTS (Merveille 3 étapes · Conseil ·
                                    * contenu débloqué). EventsState gagne 19 EvId neufs (Merveille
                                    * fondation/sacrifice/ascension, Conseil trahison/succession/
                                    * inter-conseillers R1-R3/A1-A2/C1, contenu débloqué B2/B3/B6/
                                    * C2/C3/C4) ⇒ EVID_COUNT grandit ⇒
                                    * fires[EVID_COUNT]/fire_cap[EVID_COUNT] (section EVNT, fwrite BRUT
                                    * du struct entier) changent de taille ; +2 AnnalKind (ANNAL_TRAHISON/
                                    * ANNAL_MERVEILLE_ETAPE, valeurs d'enum seulement — ne changent PAS
                                    * sizeof(AnnalEntry), pas de bump à ce titre seul). <v71 refusé.
                                    * v70 : V2a — LE CONSEIL VIVANT. Statecraft gagne
                                    * loyalty[SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS] +
                                    * pay[SCPS_MAX_COUNTRY][SC_COUNCIL_SEATS] (float, section
                                    * STAT, fwrite BRUT du struct entier) ⇒ sizeof(Statecraft)
                                    * change. <v70 refusé.
                                    * v69 : W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ. DiploState
                                    * gagne fab_state/fab_days/fab_cb[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY]
                                    * (l'état des intrigues fabriquées par (fabricant,cible),
                                    * section DIPL, fwrite BRUT du struct entier) ⇒ sizeof(DiploState)
                                    * change. <v69 refusé.
                                    * v68 : CLASS_SLAVE — la strate esclave. SocialClass gagne
                                    * CLASS_SLAVE (APPENDU, valeurs 0-2 stables) ⇒
                                    * PopStratum strata[CLASS_COUNT] grandit dans TOUTE
                                    * ProvinceEconomy ET RegionEconomy ⇒ sizeof(WorldEconomy)
                                    * change (blob ECON, fwrite BRUT). + le pool mondial du
                                    * marché des Centres (g_slave_pool[HERITAGE_COUNT], PAR
                                    * héritage) rejoint la section ITRD existante (intertrade_
                                    * save/load), sans nouveau tag de section. <v68 refusé.
                                    * v67 : ENDGAME UNIFIÉ (V1a) — EndgameState (section EGAM,
                                    * fwrite BRUT) gagne war_dead (MÉMOIRE des morts, décrue
                                    * SANG_MEMORY_HL) + war_dead_seen (dernier cumul Campaign lu)
                                    * + pop_ref + sang_scar[SCPS_MAX_REG] (cicatrice PERMANENTE)
                                    * ⇒ sizeof(EndgameState) change. (war_dead_seen ajouté DANS
                                    * la fenêtre v67, aucune save v67 antérieure n'existe.)
                                    * <v67 refusé.
                                    * v66 : PLAFOND DE TIRS À VIE — EventsState gagne
                                    * fires[EVID_COUNT][SCPS_MAX_COUNTRY] (nb de tirs par
                                    * évènement×pays ; les dilemmes récurrents plafonnent à
                                    * 3-5, les latches tech à 1) ⇒ la section EVNT (fwrite
                                    * BRUT) change de taille. <v66 refusé.
                                    * v65 : TXYR ÉTENDUE — l'instrument I0 de l'année EN COURS
                                    * (g_flux[][FX_COUNT] + compteurs de mois) rejoint la section :
                                    * sans lui, la capture annuelle post-reload lisait un flux
                                    * TRONQUÉ → d_treasury_mois (dilemmes/décrets) divergeait
                                    * (--savetest : dérive d'or seule). <v65 refusé.
                                    * v64 : DÉCRETS DU JOUEUR (civics) — nouvelle section DCRE
                                    * (g_decree_mask[SCPS_MAX_COUNTRY], bitmask état par pays).
                                    * Pas de struct partagée agrandie ⇒ un save v63 est juste
                                    * INCOMPLET pour v64 (refusé par le contrôle de version, comme
                                    * toute ère antérieure). <v64 refusé.
                                    * v63 : LES ANNALES DU RÈGNE — EventsState grossit (l'anneau
                                    * annals[96] + tête + compteur) ⇒ la section EVNT (fwrite BRUT
                                    * de sizeof(*s->ev)) change de taille. <v63 refusé.
                                    * v62 : LA MEMBRANE DE DÉCISION — EventsState grossit (cicatrices
                                    * scars[128] + cooldowns cds[96] + la file joueur pending[8]) ⇒
                                    * la section EVNT (fwrite BRUT de sizeof(*s->ev)) change de taille ;
                                    * + section TXYR (économie : g_tax_lastyear[] — un ACCUMULATEUR
                                    * inter-ticks, motif COLC v61 : sans lui un reload perdrait le
                                    * revenu annuel capté, --savetest divergerait sur d_treasury_mois).
                                    * <v62 refusé.
                                    * v61 : section COLC — le répit de colonisation g_colony_cd[] (F1,
                                    * cadence ∝ w_expand) est un ACCUMULATEUR inter-ticks : non sérialisé,
                                    * un reload le remettait à 0 → --savetest divergeait (seed 11). <v61 refusé.
                                    * v60 : §5 PUISSANCE COMMERCIALE — la section ITRD grandit (le pool
                                    * commercial MENSUEL : g_commerce_budget + g_commerce_spent, l'état
                                    * intra-mois qui borne les achats au marché). <v60 refusé.
                                    * v59 : DISSOLUTION LaborEcon — la section LABO DISPARAÎT (le module
                                    * s'est dissous : la levée LIT désormais les strates econ du pays ; la
                                    * mobilité/marché/pop_in_army labor étaient morts, la topbar viewer retirée).
                                    * <v59 refusé (le bloc LABO n'existe plus dans le flux).
                                    * v58 : SAVETEST FIX (suite) — ITRD sérialise la CARTE DES HUBS
                                    * (g_hub_of/g_hub_dist/g_hub_dirty + g_global_cache : caches dérivés lus
                                    * quotidiennement par agency_build_gold mais rebâtis SEULEMENT à un changement
                                    * de Centres — non dérivés du tick, donc stale après reload). ITRD grandit → <v58 refusé.
                                    * v57 : SAVETEST FIX — section EMOB (économie/mobilité de classe :
                                    * g_friche[SCPS_MAX_PROV] + g_lowsat_streak[SCPS_MAX_PROV][CLASS_COUNT],
                                    * scps_econ.c). Ce sont des ACCUMULATEURS croisant les ticks (friche lue
                                    * au tick SUIVANT son calcul ; mobilité de classe exige 2 mois consécutifs
                                    * sous le seuil) qui pilotent une vraie mutation économique (prod_mult,
                                    * mobility_move) — econ_mobility_reset() les remet à zéro sur un DÉMARRAGE
                                    * FRAIS (via econ_init/sim_init) mais scps_load_game ne les restaurait NI
                                    * ne les remettait à zéro : un reload gardait silencieusement la valeur
                                    * laissée par la FIN du run précédent (pas celle du jour de save), d'où une
                                    * divergence tick-side (--savetest A≠B, Σtreasury dérivant de centièmes à
                                    * quelques unités sur les 400 jours suivant un reload — pris par la preuve
                                    * checksum @save vs @post-load == @finA-avant-load). Nouvelle section, pas
                                    * de struct existante agrandie ⇒ un save v56 est juste INCOMPLET pour v57
                                    * (refusé par le contrôle de version, comme toute ère antérieure). <v57 refusé.
                                    * v56 : SOUTIEN ÉTRANGER aux rebelles — Rebellion gagne `backing_tried`
                                    * (latch SÉRIALISÉ : le rate-limit du soutien étranger vit sur la guerre civile,
                                    * pas un module-static ; un reload mid-guerre ne ré-offre plus le renfort ⇒
                                    * déterminisme, même classe que g_wild_contact). sizeof(RevoltState) change → <v56 refusé.
                                    * v55 : RÉVOLTE = VRAIE GUERRE (Phase 3a) — Rebellion gagne `rebel_country`
                                    * (le pays rebelle incarné qui déclare la guerre à la couronne ; -1 = aucun,
                                    * repli sur la résolution instantanée) + `war_days` (durée de la guerre civile,
                                    * plafond de patience). L'issue suit désormais le SCORE DE GUERRE (campagne/
                                    * bataille), plus un compare garnison/rebelles. RevoltState est sérialisé en UN
                                    * blob (sv_w SVT_RVLT) ⇒ sizeof(RevoltState) change → <v55 refusé. v54 : FOI PAR GROUPE — PopGroup gagne `faith` (id de religion PORTÉ par le
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
