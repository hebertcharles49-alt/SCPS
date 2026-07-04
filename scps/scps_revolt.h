#ifndef SCPS_REVOLT_H
#define SCPS_REVOLT_H
/*
 * scps_revolt.h — LA RÉVOLTE INCARNÉE : un soulèvement est un ACTEUR, pas un drapeau
 *
 * Hier une révolte n'était qu'un fanion de région (statecraft : agitation
 * soutenue → L↓, loi martiale). Personne ne se soulevait : le grief n'était
 * ancré nulle part. Ici la révolte naît d'un GROUPE démographique concret :
 *
 *   QUI se soulève   — le groupe au plus fort DÉFICIT (panier + sur-taxe +
 *                      aliénation à la couronne + répression + non-intégration) ;
 *   COMBIEN          — une fraction MOBILISÉE ∝ déficit qui QUITTE la main-
 *                      d'œuvre (choc économique réel : l'atelier perd ses bras) ;
 *   CE QU'ILS VEULENT— selon qui se lève : une CLASSE réclame (jacquerie), une
 *                      nation conquise fait SÉCESSION (nouveau pays), l'ÉLITE
 *                      renverse le trône (coup d'État) ;
 *   CE QU'IL ADVIENT — les rebelles (force réelle) affrontent la garnison :
 *                      écrasés → perte de pop + fragilité ; victorieux →
 *                      sécession (un pays NAÎT) / concession / coup.
 *
 * Discipline membrane : ce module est SIM (il lit des flottants SCPS comme
 * diplo/legitimacy). Son API de lecture ne renvoie que des nombres/mots de JEU.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_demography.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_diplo.h"
#include "scps_readout.h"
#include "scps_statecraft.h"  /* dédup Option B : revolt_scan lit statecraft_agitation (Statecraft*) */

/* Phase 3a — la révolte pousse un pays rebelle + une armée sur la carte : la
 * campagne (bataille/siège/score) est CÂBLÉE dans l'allumage et la résolution.
 * Forward-decl (comme campaign.h le fait pour NavyState) : pas de couplage d'en-tête,
 * le .c inclut scps_campaign.h. dp/camp NULL ⇒ repli sur la résolution instantanée. */
struct Campaign;

/* ---- Nature du soulèvement (qui se lève → ce qu'il veut) --------------- */
typedef enum {
    REBEL_NONE = 0,
    REBEL_CLASS,        /* une classe réclame des concessions (jacquerie)      */
    REBEL_SECESSION,    /* une nation conquise/étrangère se détache → nouveau pays */
    REBEL_COUP,         /* l'élite renverse la couronne (affaire de palais)    */
    REBEL_HERESIE,      /* SCHISME de la foi d'État (même racine) : Réforme intérieure
                         * — écrasé → Contre-Réforme (reconverti), vainqueur → paix
                         * d'Augsbourg (garde sa foi schismatique, L royale ↓) */
    REBEL_ZELOTE        /* foi EXTÉRIEURE (racine ≠ celle de l'État) : guerre sainte —
                         * zélote plus ardent (ZEAL↑), même issue foi (reconv./tolérance)
                         * mais l'humiliation d'un Dieu ÉTRANGER creuse plus la L royale */
} RebelKind;

/* ---- Verdict d'un soulèvement (rempli à la résolution) ----------------- */
typedef enum {
    OUT_ONGOING = 0,
    OUT_CRUSHED,        /* écrasés : morts + le pays se fragilise (coercition)  */
    OUT_SECEDED,        /* sécession réussie : un pays est né                   */
    OUT_CONCESSION,     /* la couronne cède : satisfaction ↑, agitation ↓       */
    OUT_COUP            /* l'élite prend le trône : la couronne change de mains */
} RevoltOutcome;

#define REVOLT_MAX 64
/* LISIBILITÉ FIL (feed) : borne du tampon des guerres civiles incarnées démarrées au
 * dernier revolt_scan (cf. revolt_new_civilwar_count/_at) — exposée ici pour que
 * sim_day puisse dimensionner son propre petit tableau de régions déjà rapportées. */
#define NEW_CIVILWAR_CAP 16

/* ---- Un soulèvement incarné ------------------------------------------- */
typedef struct {
    bool             active;
    int              region;     /* foyer du soulèvement */
    int              owner;      /* la couronne visée */
    RebelKind        kind;
    /* identité du groupe soulevé — pour la résolution ET le chroniqueur */
    Heritage heritage;
    SocialClass      klass;
    PopCulture       culture;    /* fiche effective au moment du soulèvement */
    int              drift_id;   /* clé du groupe (le retrouver dans la province) */
    long             mobilized;  /* combattants partis du travail = force rebelle */
    float            deficit;    /* ce qui les a poussés [0..1] (mémoire) */
    int              days;       /* durée du soulèvement */
    int              outcome;    /* RevoltOutcome (0 = en cours) */
    int              spawned;    /* pays né de la sécession (-1 sinon) */
    /* Phase 3a — LA RÉVOLTE EST UNE VRAIE GUERRE : le soulèvement fait NAÎTRE un
     * pays rebelle + une armée de campagne qui DÉCLARE la guerre à la couronne et
     * se bat par le système campagne/bataille/score-de-guerre EXISTANT. L'issue
     * suit le SCORE DE GUERRE (plus un compare instantané garnison/rebelles). */
    int              rebel_country; /* pays rebelle incarné (-1 = aucun → résolution INSTANTANÉE de repli) */
    int              war_days;      /* durée de la guerre civile (jours) — plafond de patience */
} Rebellion;

typedef struct {
    Rebellion list[REVOLT_MAX];
    int       count;             /* slots actifs (liste compactée) */
    float     desperation_days[SCPS_MAX_REG];  /* misère SOUTENUE par région → allumage (≥0, sens UNIQUE) */
    float     revolt_cooldown[SCPS_MAX_REG];   /* RÉPIT post-révolte (jours restants, ≥0) : BLOQUE le
                                                * ré-allumage tant qu'il n'est pas purgé → pas de boucle
                                                * écrasement/rallumage. Séparé de desperation_days pour ne
                                                * pas surcharger un même champ de deux sémantiques (l'ancien
                                                * sentinel négatif était effacé au moindre calme → bug). */
    float     revanchism_days[SCPS_MAX_REG];   /* SÉPARATISME post-conquête (≈10 ans) :
                                                * une province fraîchement soumise peut se
                                                * soulever pour l'indépendance « quoi qu'il arrive » */
    /* compteurs de chronique (cumul sur la partie) */
    int       n_ignited, n_crushed, n_seceded, n_concession, n_coup, n_heresy, n_zelote;
    long      pop_lost;          /* morts au total */
    int       last_spawned;      /* dernier pays né (-1) — pour brancher l'IA */
} RevoltState;

void revolt_init(RevoltState *rs);

/* ---- Le DÉFICIT d'un groupe [0..1] : la pression qui pousse au soulèvement
 * panier vivrier+social (1-satisfaction) + sur-taxe + aliénation à la couronne
 * (distance culturelle) + répression (coercition/H) + (1-intégration). */
float revolt_group_deficit(const PopGroup *g, const ModifierStack *drift,
                           const PopCulture *crown, float food_sat, float society_sat,
                           float tax_pressure, float coercion);

/* ---- La fraction MOBILISÉE : combien quittent le travail pour se battre - */
long  revolt_mobilized(const PopGroup *g, float deficit);

/* ---- La nature : qui se lève détermine ce qu'il veut ------------------- */
RebelKind revolt_classify(const PopGroup *g, const ModifierStack *drift, const PopCulture *crown);

/* ---- SCAN : la misère SOUTENUE d'une région allume un soulèvement ------
 * Chaque région possédée : on lit le pire déficit de groupe ; au-delà du seuil,
 * la désespérance s'accumule ; soutenue assez longtemps → revolt_ignite. C'est
 * le déclencheur ANCRÉ sur les groupes (un conquis non-intégré finit par se lever).
 * `days` = pas écoulé.
 * Dédup Option B (2026-07-04) — CE module est désormais le SEUL acteur de révolte :
 * `sc` (Statecraft, peut être NULL — bancs) replie le SIGNAL d'agitation legacy
 * (statecraft_agitation, 0-100 : L/coercion/choc de conquête/stabilité/garnison)
 * dans le `worst` comme un grief politique SUPPLÉMENTAIRE (aux côtés de la misère
 * de groupe), pondéré par le tunable W_AGITATION_UNREST. statecraft ne fire plus
 * lui-même — cette lecture est la SEULE voie par laquelle son signal atteint
 * encore une révolte réelle.
 * dp/camp (Phase 3a) : passés à l'allumage pour DÉCLARER la guerre civile et
 * DÉPLOYER l'armée rebelle ; NULL ⇒ résolution instantanée de repli (bancs). */
void  revolt_scan(RevoltState *rs, World *w, WorldEconomy *econ,
                  const ModifierStack *drift, const Statecraft *sc,
                  DiploState *dp, struct Campaign *camp, int days);

/* ---- REVANCHISME : subir la conquête arme le séparatisme (≈10 ans) ------
 * À appeler quand une province passe sous une couronne ÉTRANGÈRE. Pendant la
 * fenêtre : le soulèvement s'allume « quoi qu'il arrive », les rangs rebelles
 * grossissent et la garnison d'occupation tient moins bien — une vraie chance
 * d'indépendance, qui s'éteint à mesure que la blessure se referme. */
void  revolt_on_conquest(RevoltState *rs, int region);

/* ---- ALLUMAGE : une région a basculé → on incarne le soulèvement -------
 * Choisit le groupe au plus fort déficit ; s'il dépasse le seuil, MOBILISE
 * (les combattants QUITTENT la main-d'œuvre → choc économique) et enregistre un
 * Rebellion. Renvoie l'index du soulèvement, ou -1 si aucun groupe ne se lève.
 * dp/camp (Phase 3a) : si fournis, incarne un PAYS rebelle + une armée de campagne
 * qui déclare la guerre à la couronne ; NULL ⇒ pas de guerre (repli instantané). */
int   revolt_ignite(RevoltState *rs, World *w, WorldEconomy *econ,
                    const ModifierStack *drift, DiploState *dp, struct Campaign *camp,
                    int region, float tax_pressure);

/* ---- RÉSOLUTION : un pas (jours). Les rebelles affrontent la garnison ---
 * garnison = pop loyale levée + H local + renforts de la couronne (mil_power) ;
 * force rebelle = mobilisés × zèle (le coup frappe fort, peu nombreux). Verdict :
 *   écrasés  → perte de pop + coercition (le pays se raidit) ;
 *   victoire → sécession (un PAYS NAÎT) / concession / coup (couronne changée).
 * dp/camp (Phase 3a) : quand un soulèvement porte un pays rebelle, l'issue suit le
 * SCORE DE GUERRE (diplo_war_score) au lieu du compare instantané ; NULL ⇒ repli. */
void  revolt_tick(RevoltState *rs, World *w, WorldEconomy *econ, ModifierStack *drift,
                  WorldLegitimacy *wl, const WorldProsperity *wp,
                  DiploState *dp, struct Campaign *camp, int days);

/* ---- Membrane : des mots/nombres de JEU, jamais un flottant SCPS ------- */
int          revolt_active_count(const RevoltState *rs);
/* TÉLÉMÉTRIE guerre civile (Phase 3a) : cumuls PAR SIM (RAZ dans revolt_init) — guerres civiles
 * ENGAGÉES (armée rebelle déployée) et remportées par les rebelles. Statiques, non sérialisés. */
long         revolt_civilwar_count(void);
long         revolt_rebel_victory_count(void);
/* LISIBILITÉ FIL (feed) : les guerres civiles INCARNÉES démarrées au DERNIER revolt_scan
 * (tampon statique, RAZ en tête de scan, hors RevoltState ⇒ non sérialisé). sim_day les
 * lit juste après le tick pour pousser un FEED_REVOLT nommé ("Rebelles de X") au lieu
 * du générique. revolt_new_civilwar_at(i,&owner,&region) renvoie le rebel_country (-1
 * hors borne) ; owner/region NULL-safe. */
int          revolt_new_civilwar_count(void);
int          revolt_new_civilwar_at(int i, int *owner, int *region);
const char  *revolt_kind_word   (RebelKind k);   /* "jacquerie"/"sécession"/"coup d'État" */
const char  *revolt_outcome_word(int outcome);   /* "écrasée"/"indépendance"/… */
const char  *revolt_class_word  (SocialClass k); /* "Laboureurs"/"Artisans"/"Noblesse" */

#endif /* SCPS_REVOLT_H */
