#ifndef SCPS_DEMOGRAPHY_H
#define SCPS_DEMOGRAPHY_H
/*
 * scps_demography.h — LA CLÉ DE VOÛTE : une province contient des GROUPES
 *
 * Hier une province était une fiche HOMOGÈNE → D interne nul, H injouable,
 * assimilation orpheline. Ici une province contient des groupes
 * (heritage, culture, classe, effectif). Un seul changement rend réels d'un coup :
 *   - D interne PAR province (distance ENTRE groupes) ;
 *   - H jouable (on réprime UNE province et ses minorités restives) ;
 *   - l'assimilation incarnée (la culture d'une minorité DÉRIVE vers la
 *     dominante — la pile scps_modifier trouve sa pâture) ;
 *   - légitimité & fracture VÉCUES (le conquis a une L basse, les natifs loyaux).
 *
 * Discipline : H SUPPRIME (réversible) ; seuls P+K+L+temps MÉTABOLISENT (durable).
 * Rétro-compat : une province MONO-GROUPE reproduit les nombres d'aujourd'hui.
 * Le verdict reste au PAYS (scps_order inchangé) ; les métriques par province
 * (D, L, agitation) suffisent au local et remontent au pays.
 */
#include "scps_world.h"      /* World, WorldEconomy (l'intégration au moteur) */
#include "scps_econ.h"       /* PopGroup, ProvincePop, PopCulture, SocialClass */
#include "scps_heritage.h"    /* Heritage, Sphere */
#include "scps_modifier.h"   /* la pile de dérive (assimilation/suppression) */
#include "scps_routes.h"     /* S2 : RouteNetwork (contact commercial) */
#include "scps_diplo.h"      /* S2 : DiploState (la guerre coupe le contact) */
#include "scps_prosperity.h" /* attracteurs endogènes : K du pays */
#include "scps_readout.h"    /* BandHumeur — la loyauté en MOT (membrane) */

/* PopGroup / ProvincePop sont définis dans scps_econ.h (bas niveau) : ainsi
 * RegionEconomy les porte et prosperity/legitimacy les LISENT sans cycle. */
#define DEMO_MAX_GROUPS SCPS_MAX_GROUPS

/* ---- Fiche EFFECTIVE = origine + pile (recalcul, pas mutation) -------- */
PopCulture group_culture_effective  (const PopGroup *g, const ModifierStack *drift);
float      group_agitation_effective(const PopGroup *g, const ModifierStack *drift);
/* Même porte que demography_contact_tick, appliquée à deux fiches de population
 * déjà effectives. Évite à la façade de recopier la conversion PopCulture→Culture. */
SyncFeasibility pop_culture_can_syncretize(const PopCulture *a, const PopCulture *b,
                                           float P, float K);

/* ---- Lectures de province (§2) --------------------------------------- */
const PopGroup *province_dominant (const ProvincePop *pp);
long            province_total_pop(const ProvincePop *pp);
float province_Dbar     (const ProvincePop *pp, const ModifierStack *drift);  /* moyenne pondérée inter-groupes */
float province_Dinf     (const ProvincePop *pp, const ModifierStack *drift);  /* MAILLON FAIBLE : max */
float province_L        (const ProvincePop *pp);
float province_agitation(const ProvincePop *pp, const ModifierStack *drift);

/* ---- Légitimité PAR GROUPE (formule existante, clé sur culture vs couronne) */
float group_L_target(const PopGroup *g, const ModifierStack *drift, const PopCulture *crown,
                     float satisfaction, float integ, float country_H, float coercion, float build_H);
void  group_L_tick  (PopGroup *g, const ModifierStack *drift, const PopCulture *crown,
                     float satisfaction, float country_H, float coercion, float build_H);

/* ---- H jouable — SUPPRIME (réversible), n'assimile pas (§3) ----------- */
typedef struct { float agitation_drop, L_drop, fragility_rise; } CoercionEffect;
CoercionEffect province_apply_coercion(ProvincePop *pp, ModifierStack *drift, float H);
void           province_lift_coercion (ProvincePop *pp, ModifierStack *drift);  /* la botte se lève (Kuran) */

/* ---- Assimilation — DÉRIVE durable, timer ∝ D∞ (gouffre, §5) ---------- */
float assimilation_years(float Dinf, float P, float K);   /* Agraire ~20 ans, Clanique 80-150 */
/* Fait dériver chaque minorité vers le dominant d'un pas (years_per_tick). Fusion
 * quand la distance < EPS. Renvoie le nb de groupes fusionnés ce tick. */
/* + CANAL ÉTATIQUE (2026-08-11) : crown/state_w — un État capable assimile vers la
 * COURONNE au-delà de son poids démographique. crown=NULL ou state_w=0 = l'hier exact. */
int   assimilation_tick(ProvincePop *pp, ModifierStack *drift, float P, float K, float years_per_tick,
                        const PopCulture *crown, float state_w);

/* ---- Conversion religieuse (§2) — la FOI converge vers le TRÔNE ------- *
 * Distincte de l'assimilation (qui tire vers la dominante LOCALE) : l'axe
 * doctrinal de chaque groupe dérive vers la couronne, et la BRANCHE sacrée
 * bascule une fois la foi enracinée (`years_held`) et l'axe convergé — mais
 * seulement sous un trône PROSÉLYTE (credo ≠ pluraliste). Pluraliste : nulle
 * conversion, l'empire reste multi-confessionnel. Appelé par demography_tick. */
void  faith_convert_tick(ProvincePop *pp, const PopCulture *crown,
                         float years_held, float years_per_tick);

/* ---- Migration passive — emporte heritage + culture (§4) ----------------- */
/* Déplace `amount` du groupe `gi` de `from` vers `to` (adjacence/prospérité
 * jugées par l'appelant). Crée une minorité/diaspora à l'arrivée → du D interne.
 * `new_drift_id` : clé fraîche si une diaspora doit être créée. `mode` (Arrival) :
 * ARR_MIGRANT (migration/pacte) / ARR_REFUGIE (fuite de guerre) / ARR_DEPORTE (esclave).
 * `home_reg` : RÉGION d'origine inscrite sur la diaspora créée (le foyer où RESPIRER, -1 =
 * aucun) ; un groupe DÉJÀ déplacé garde son home d'origine (jamais écrasé). */
bool migration_move(ProvincePop *from, ProvincePop *to, int gi, long amount, int new_drift_id, int mode, int home_reg);

/* ══ L'INVARIANT DES SIÈGES : Σ pop_by_class == count (P2, rapport CALIB POPULATION §4.2) ══
 * `count` (les ÂMES du groupe) et `pop_by_class[]` (les SIÈGES d'emploi qu'elles occupent)
 * sont écrits par des chemins DIFFÉRENTS : l'émergence de classe pose les seconds une fois
 * par mois, mais migration/fusion/essaimage/mobilisation bougent le PREMIER entre-temps —
 * un groupe de 200 âmes héritait ainsi des sièges d'un groupe de 5 000. Là où l'émergence
 * tourne, l'écart se répare au tick suivant ; ailleurs il est PERMANENT et CUMULATIF.
 * Ce helper RE-PROPORTIONNE les sièges existants sur le nouveau `count`, sans inventer un
 * seul nombre : mêmes PARTS de classe, même convention de paquets de 100 que
 * `demography_emerge_classes`, le reste au journalier. Un groupe TENU (CLASS_SLAVE) porte
 * toutes ses âmes dans sa propre classe. À appeler APRÈS toute écriture de `count`. */
void demography_group_seats_rescale(PopGroup *g);

/* ATTRACTIVITÉ MIGRATOIRE = prospérité + BÂTI (institutions). Un empire ultra-bâti ultra-prospère
 * est un AIMANT : la migration échelonne avec l'attractivité. (exposé pour l'auto-vérif) */
float migration_attractivity(float prosperity, float K_inst);

/* ---- Agrégation PAYS (§2, §6) — alimente scps_order (inchangé) -------- */
float country_Dbar(const ProvincePop *provs, int n, const ModifierStack *drift);
float country_Dinf(const ProvincePop *provs, int n, const ModifierStack *drift);  /* maillon faible pays */
float country_L   (const ProvincePop *provs, int n);

/* ---- Composition (§6) — la membrane : mots, jamais de SCPS brut ------- */
typedef struct {
    const char *heritage;      /* "Humain", "Clanique"… (diégétique) */
    const char *culture;   /* nom de culture (diégétique) */
    const char *lineage;   /* parents/racines du nom, vide pour une culture fondatrice */
    const char *religion;  /* branche de foi (diégétique) — pour le camembert Religion */
    const char *klass;     /* "Noblesse" / "Artisans" / "Laboureurs" */
    int         percent;   /* part de la province */
    BandHumeur  loyaute;   /* L du groupe → MOT (membrane) */
    const char *etat;      /* "natif" / "en assimilation (N ans)" / "diaspora" */
} GroupReadout;
int province_composition(const ProvincePop *pp, const ModifierStack *drift,
                         const PopCulture *crown, float P, float K,
                         GroupReadout out[], int max);
const char *labor_class_word(SocialClass k);   /* Noblesse / Artisans / Laboureurs */

/* ===================================================================== */
/* INTÉGRATION AU MOTEUR VIVANT (§7) — la province RÉELLE porte des groupes */
/* ===================================================================== */
/* Attache à chaque région peuplée UN groupe substrat (sa culture, sa pop) →
 * rétro-compatible : mono-groupe = les nombres d'hier. À appeler après
 * gen_population/worldgen_seed_peoples. drift = la pile du monde (réinitialisée). */
void demography_attach(World *w, WorldEconomy *econ, ModifierStack *drift);

/* Un pas (un an) sur la démographie VIVANTE : rafraîchit la fiche effective de
 * chaque groupe (cache), fait la L par groupe, l'assimilation (dérive durable),
 * la migration (groupes vers la prospérité), puis SYNCHRONISE RegionEconomy.culture
 * (= groupe dominant). Le verdict reste au pays (scps_order inchangé).
 * `integ_mult` (raccord 2, Âge des Empires — PERMANENT, défaut 1) accélère
 * PROPORTIONNELLEMENT le pas d'assimilation (assimilation_tick reçoit dt*integ_mult
 * au lieu de dt) — même levier que P/K, pas un second système. */
void demography_tick(World *w, WorldEconomy *econ, WorldLegitimacy *wl,
                     ModifierStack *drift, float P, float K, float dt, float integ_mult);

/* S2 — LA CRISTALLISATION CULTURELLE PAR CONTACT (réveille `culture_syncretize`) : une
 * région en contact COMMERCIAL soutenu (route ouverte, à la paix) avec un autre pays voit
 * sa culture dominante dériver vers la sienne (la MER porte plus loin), jugée par la porte
 * métabolique INCHANGÉE ; au franchissement du seuil, l'hybride cristallise. Renvoie le
 * nombre de cristallisations CE pas. À appeler au pas ANNUEL (ypt = années/pas). */
int demography_contact_tick(WorldEconomy *e, ModifierStack *drift, const RouteNetwork *rn,
                            const DiploState *dp, float P, float K, float ypt);
void demography_contact_reset(void);   /* RAZ du compteur de cristallisations (par sim) */
long demography_contact_count(void);   /* cristallisations par contact cumulées (télémétrie) */
/* L'AFFRANCHI PAR MÉTABOLISATION (2026-08-11) : un groupe servile intégré ≥ MANUMIT_INTEG
 * est affranchi (sauf couronne dominatrice) — la sortie du puits servile hors abolition. */
long demography_manumit_integrated(WorldEconomy *econ, const World *w);
/* TURCHIN (2026-08-11) : l'excédent d'élite au-delà des positions réelles [0..1]. */
float demography_elite_rival(const ProvinceEconomy *pe);
/* ATTRACTEURS ENDOGÈNES (2026-08-11) : les valeurs du peuple dominant dérivent vers un
 * attracteur composite (ancre du mode de vie · confort→mercantile · guerre→dominateur ·
 * K→bureaucrate) ; l'éthos cristallise avec hystérésis. Une culture bifurque ENFIN seule. */
int  demography_values_tick(WorldEconomy *e, ModifierStack *drift,
                            const WorldProsperity *wp, const DiploState *dp, float ypt);
void demography_values_reset(void);
long demography_values_count(void);
void demography_migration_pact_reset(void);
long demography_migration_pact_count(void);   /* flux de pacte migratoire cumulés (télémétrie) */
long demography_migration_pact_souls(void);   /* ÂMES déplacées par pacte cumulées (le volume, pas l'événement) */

/* RÉFUGIÉS (BRASSAGE) — la violence (revolt_scar haut : sac/révolte) fait FUIR vers une région
 * voisine SÛRE (si possible), diaspora ARR_REFUGIE au FOYER inscrit ; puis, foyer apaisé, une
 * part RENTRE (décroissante avec l'intégration — le fixé reste). Annuel. Aucun déplacé n'est
 * définitif : le migrant économique respire aussi (retour ténu). Renvoie fuites+retours ce pas. */
int  demography_refugee_tick(World *w, WorldEconomy *e, const DiploState *dp);
void demography_refugee_reset(void);           /* RAZ des compteurs (par sim) */
long demography_refugee_fled(void);            /* réfugiés partis cumulés (télémétrie) */
long demography_refugee_returned(void);        /* rentrés au foyer cumulés (télémétrie) */
long demography_refugee_fled_souls(void);      /* ÂMES en fuite cumulées */
long demography_refugee_returned_souls(void);  /* ÂMES rentrées cumulées */

/* Dépose `amount` du groupe dominant du pays `cid` (sa culture régnante) dans la
 * région conquise `region` → minorité de colons sous une couronne étrangère, OU
 * laisse les locaux conquis en minorité restive. Crée du D INTERNE vécu. */
void demography_on_conquest(World *w, WorldEconomy *econ, ModifierStack *drift, int region, int conqueror);

/* drift_id DYNAMIQUES (migration/conquête) : compteur unique et monotone — deux
 * groupes vivants ne partagent jamais une identité de dérive. rebase : à appeler
 * après un chargement (le compteur n'est pas sérialisé) — repart au-dessus du
 * plus grand drift_id vivant. */
int  demography_dyn_id_next(void);
/* AUDIT 2026-08-12 — la mort d'un groupe rend sa dérive : les sites SANS pile
 * déposent la clef (retire), demography_tick draine ; le scrub post-load balaie
 * les orphelines de la pile sérialisée (résurrection impossible). */
void demography_drift_retire(int drift_id);
void demography_drift_drain(ModifierStack *drift);
void demography_drift_scrub(const WorldEconomy *econ, ModifierStack *drift);
void demography_dyn_id_rebase(const WorldEconomy *econ);

/* ---- ESCLAVAGE — L'AFFRANCHISSEMENT (CMD_MANUMIT, granularité PAYS) --------- *
 * Bascule TOUS les groupes esclaves (klass==CLASS_SLAVE) du pays `cid` en pop
 * LIBRE : klass→CLASS_LABORER, arrival ARR_DEPORTE→ARR_MIGRANT (la diffusion du
 * savoir passe de FAIBLE à PLEINE — la manœuvre pacifiste), `integration` GARDÉE
 * (ils restent peu intégrés : la friction devient RÉELLE, c'est le prix du choix).
 * Bascule aussi la strate économique (strata[CLASS_SLAVE]→CLASS_LABORER, sur la
 * PROVINCE représentative de chaque région où vivent des esclaves de ce pays).
 * Renvoie le nombre d'âmes affranchies. */
long demography_manumit_country(WorldEconomy *econ, int cid);
void demography_manumit_reset(void);   /* RAZ du compteur (par sim) */
long demography_manumit_count(void);   /* âmes affranchies cumulées (télémétrie) */

/* LOT H — la révolte SERVILE VICTORIEUSE affranchit DE FORCE (granularité RÉGION,
 * même bascule klass→CLASS_LABORER / arrival DÉPORTÉ→MIGRANT que l'affranchissement
 * pacifiste, mais bornée à UNE région — la région du soulèvement, pas tout le pays).
 * Renvoie le nombre d'âmes affranchies. */
long demography_manumit_region(WorldEconomy *econ, int region);

/* ---- RÉINCORPORATION DE POP (CMD_POP_TRANSFER, RE-KEY PROVINCE, JOUEUR seul) ----- *
 * Déplace `count` âmes de la classe `klass` de `src_prov` vers `dst_prov` — PID
 * DIRECT des DEUX côtés (aucune indirection econ_region_rep_province : seul
 * appelant, le drain CMD_POP_TRANSFER joueur — pas de chemin IA à préserver ici).
 * Réutilise migration_move : le déplacé GARDE heritage/arrival/integration/klass
 * (une diaspora suit qui elle est — seul le FOYER, home_reg=région de la province
 * source, change), fusion dans un groupe compatible à l'arrivée sinon nouvelle
 * minorité. PLANCHER ANTI-VIDAGE : jamais plus de la MOITIÉ de la classe ciblée
 * dans la province source (même discipline qu'econ_relocate_pop : 50%). LE COÛT :
 * le canal de coercition forcée existant (RELOC_COERCION_BASE, miroir
 * econ_relocate_pop) frappe la province SOURCE — SAUF si klass==CLASS_SLAVE (on
 * déplace une propriété, pas un déplacement forcé de sujets libres ; le sort de
 * l'esclave n'est pas le sien à contester). Renvoie les âmes réellement déplacées. */
long demography_pop_transfer(WorldEconomy *econ, int src_prov, int dst_prov,
                             int klass, long count);

#endif /* SCPS_DEMOGRAPHY_H */
