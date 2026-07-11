#ifndef SCPS_ENDGAME_H
#define SCPS_ENDGAME_H
/*
 * scps_endgame.h — Capstone §27 : Entropie mondiale + 4 fins + Merveille.
 *
 * Ce module appartient au MOTEUR (jamais inclus par viewer.c).
 * Communication avec le renderer : EndgameReadout (scps_readout.h).
 *
 * La struct EndgameState est PLATE (aucun pointeur) : compatible sv_w.
 */
#include <stdint.h>
#include <stdbool.h>
#include "scps_types.h"
#include "scps_econ.h"
#include "scps_prosperity.h"
#include "scps_tech.h"
#include "scps_trade.h"
#include "scps_navy.h"
#include "scps_diplo.h"
#include "scps_campaign.h"   /* FieldArmy : la carve échoue les armées sur sol englouti */

/* ---- Tailles (capacités — PAS dans le registre J) --------------------- */
#define SCPS_THORN_FRONT_MAX 16384  /* cellules max dans le front BFS des ronces */

/* ---- Enums ------------------------------------------------------------ */
/* FIN_SANG APPENDUE APRÈS FIN_ASCENSION (2026-07-06, V1a) : les valeurs existantes
 * (0..4) restent STABLES — un save v66 (fin ≤ FIN_ASCENSION) reste valide ; côté
 * façade, fin=5 (RFIN_SANG, à ajouter côté membrane hors ce lot) passera sans
 * toucher scps_api.c. */
/* FIN_CHAUD APPENDUE APRÈS FIN_SANG (2026-07-08) : même motif que l'ajout de SANG —
 * valeurs existantes (0..5) STABLES ; côté façade, fin=6 (RFIN_CHAUD) passe tel quel.
 * Le RÉCHAUFFEMENT : la fin des mondes CALMES — leur feu domestique (bois de feu servi
 * au panier + charbon industriel) charge le ciel (fuel_charge, ci-dessous). */
typedef enum {
    FIN_AUCUNE = 0, FIN_EAU, FIN_FROID, FIN_RONCES, FIN_ASCENSION, FIN_SANG, FIN_CHAUD
} FinType;

typedef enum {
    MERV_NONE = 0,
    MERV_FORGE, MERV_FORGE_DONE,
    MERV_SOCIETE, MERV_SOCIETE_DONE,
    MERV_SAVOIR, MERV_SAVOIR_DONE,
    MERV_ASCENDED
} MervPhase;

/* ---- État cataclysme (struct PLATE — sans pointeur pour sv_w) ---------
 * Le tag `EndgameState` est NOMMÉ : la membrane (scps_readout.h) le
 * forward-déclare sans tirer ce header moteur. */
typedef struct EndgameState {
    FinType  fin;
    bool     fired;                           /* latch : un seul déclenchement */

    int  epicenter_reg;                       /* région-foyer (figée au fire) */
    int  fauteur_country;                     /* empire fauteur (figé) */
    int  fin_year;                            /* année du déclenchement */

    uint8_t  sunken[SCPS_MAX_REG];           /* EAU : bitmap des régions englouties */
    int      n_sunken;                        /* nb régions réellement englouties */
    int      sink_pending;                    /* régions encore à engloutir */

    float    cold_offset;                     /* FROID : décalage cumulé (plafond) */

    int      thorn_front[SCPS_THORN_FRONT_MAX]; /* RONCES : front BFS cellules */
    int      thorn_front_n;                   /* taille du front actuel */
    uint32_t thorn_rng;                       /* RNG dédié (déterminisme) */

    MervPhase merv;                           /* MERVEILLE : phase courante */
    int       merv_country;                   /* empire qui bâtit */
    int       merv_site_reg;                  /* région-chantier */
    float     merv_progress;                  /* progression [0..1] du palier courant */

    /* ── V1a — ENDGAME UNIFIÉ (2026-07-06) ────────────────────────────────
     * L'entropie a désormais QUATRE nourritures (une seule barre, décision
     * joueur #1) : la charge de tech faustienne (C1, déjà là), les
     * transmuteurs (déjà là, via wp->faust_consumed), la pression de l'Âge
     * de la Brèche (wp->age_breach_flux, déjà là — juste ADDITIONNÉE), et
     * LES MORTS DE GUERRE (neuf). war_dead/pop_ref sont le ratio qui nourrit
     * l'entropie ET sélectionne FIN_SANG — sérialisés pour que le ratio soit
     * stable au reload (la sélection au fire doit relire le MÊME chiffre). */
    double   war_dead;                        /* MÉMOIRE des morts de guerre — accumulateur de DELTAS
                                               * à DÉCRUE annuelle (demi-vie SANG_MEMORY_HL ans : « on
                                               * compte ceux qui se souviennent des vivants » — le monde
                                               * OUBLIE ; sans décrue le cumul à vie dépasse la pop qui
                                               * se renouvelle : 40-961 % mesurés ⇒ toute partie longue
                                               * serait SANG). */
    double   war_dead_seen;                   /* dernier cumul Campaign lu (delta = cum − seen) */
    double   pop_ref;                         /* Σ pop à la genèse (posée UNE fois par sim_init) */

    /* ── #32 (LE SANG SIGNE TON RÈGNE) — JUMEAU joueur (2026-07-06) ───────────
     * war_dead est MONDIAL : un joueur pacifiste dans un monde sanglant recevait
     * FIN_SANG sans avoir tiré une flèche. war_dead_player ne compte QUE les morts
     * des batailles où le joueur humain est un des deux belligérants
     * (Campaign.dead_choc_player/dead_pursuit_player, cumulés au MÊME site
     * per-bataille que le global) — même décrue SANG_MEMORY_HL, même
     * delta-tracking (war_dead_player_seen). En chronique/viewer sans joueur
     * (human_player=-1), Campaign ne cumule JAMAIS ces compteurs jumeaux ⇒
     * war_dead_player reste à 0 pour toujours (zéro impact monde IA). */
    double   war_dead_player;                 /* mémoire à décrue, morts DU joueur seulement */
    double   war_dead_player_seen;            /* dernier cumul Campaign.*_player lu */

    /* SANG (FIN_SANG, refonte 2026-07-11 « fins corrigées ») : un RATCHET sur
     * revolt_scar — une CICATRICE QUI NE GUÉRIT PLUS (contrairement à revolt_scar
     * qui décroît normalement). Chaque année (sang_step), toute région dont le
     * revolt_scar agrégé franchit SANG_SCAR_MIN fait MONTER sa marque (jamais
     * redescendue) ; la marque PLANCHE en retour le revolt_scar de CHAQUE province
     * de la région. AUCUN drain de pop, AUCUN modificateur d'habitabilité, AUCUNE
     * région supprimée — les moteurs EXISTANTS qui lisent revolt_scar (production,
     * croissance, reconstruction, valeur de province, exode via REFUGEE_FLEE_SCAR)
     * font tout le reste. */
    float    sang_scar[SCPS_MAX_REG];         /* SANG : plancher de la marque [0..1], PERMANENT */

    /* ── FIN_CHAUD (2026-07-08) — LE RÉCHAUFFEMENT, la fin des mondes calmes ─────
     * Le combustible RÉELLEMENT brûlé (bois de feu SERVI au panier des journaliers +
     * charbon CONSOMMÉ en intrant de manufacture — l'offre servie, jamais la demande)
     * est cumulé côté éco (WorldEconomy.fuel_wood_cum/fuel_coal_cum, sérialisés dans
     * le blob ECON) et lu ICI en DELTA — le MÊME motif que war_dead/Campaign.dead_choc :
     * *_seen = dernier cumul lu, fuel_charge = mémoire pondérée (FUEL_COAL_W ×charbon)
     * à décrue lente (FUEL_MEMORY_HL — le CO2 persiste plus longtemps que le souvenir
     * des morts). Sérialisés (jurisprudence EMOB/COLC/TXYR : tout accumulateur
     * inter-ticks non sérialisé fait diverger --savetest). */
    double   fuel_seen_wood;                  /* dernier cumul bois-de-feu lu (delta-tracking) */
    double   fuel_seen_coal;                  /* dernier cumul charbon lu (delta-tracking) */
    double   fuel_charge;                     /* mémoire pondérée à décrue (bois + FUEL_COAL_W·charbon) */
    float    heat_offset;                     /* CHAUD : décalage de température cumulé (miroir de cold_offset) */
} EndgameState;

/* ---- API -------------------------------------------------------------- */
void endgame_init(EndgameState *eg);

/* Pose pop_ref UNE fois, à la genèse (appelé par sim_init APRÈS gen_population,
 * juste après endgame_init — le point CANONIQUE : c'est la première fois que le
 * monde a une population réelle). No-op si déjà posé (>0) — un reload ne le
 * ré-amorce jamais (le ratio war_dead/pop_ref doit rester stable au reload). */
void endgame_set_pop_ref(EndgameState *eg, const WorldEconomy *econ);

/* Le ratio de sang CANONIQUE : mémoire décrue des morts / pop ACTUELLE (repli
 * pop_ref sans econ). Lu par l'entrée d'entropie, la sélection FIN_SANG et la
 * télémétrie chronicle — un seul chiffre partout. */
double endgame_blood_ratio(const EndgameState *eg, const WorldEconomy *econ);

/* FIN_CHAUD — le ratio de combustible CANONIQUE : mémoire décrue du feu brûlé /
 * pop ACTUELLE (repli pop_ref sans econ) — le per-capita de la civilisation qui
 * charge son ciel. Lu par l'entrée d'entropie, la sélection FIN_CHAUD et la
 * télémétrie chronicle — un seul chiffre partout (miroir d'endgame_blood_ratio). */
double endgame_fuel_ratio(const EndgameState *eg, const WorldEconomy *econ);

/* #32 — la PART du joueur dans le sang mondial : war_dead_player / war_dead (mémoires
 * décrues, même échelle) ∈ [0,1]. 0 si war_dead≤0 (rien à partager) OU eg NULL. Lu par
 * la sélection FIN_SANG (n'accorde la fin QUE si le joueur y est POUR QUELQUE CHOSE)
 * et par la membrane (ScpsEndgameInfo.blood_player_pct — le hover « c'est TA guerre »). */
double endgame_blood_player_share(const EndgameState *eg);

/* Tick orchestrateur : appelé UNE fois par an (après prosperity_tick) depuis
 * sim_step / chronicle. camp : pour échouer les armées sur sol englouti (et,
 * V1a, nourrir l'entropie des morts de guerre + la mutation SANG). */
void endgame_tick(EndgameState *eg, World *w, WorldEconomy *econ,
                  WorldProsperity *wp, const TechState ts[],
                  RouteNetwork *rn, NavyState *navy, DiploState *dp,
                  Campaign *camp, int player, int year);

/* Démarre la Merveille d'Ascension (ordre agency JOUEUR uniquement ; l'IA ne la
 * poursuit pas). No-op si déjà en cours. */
void endgame_start_wonder(EndgameState *eg, int player, int capital_region);

/* MÉTABOLISATION — nb d'héritages (sur HERITAGE_COUNT) « métabolisés » par cid :
 * natif (l'héritage de la capitale) + tout héritage digéré au tier 3 (même seuil
 * METAB_TIER3 que la barre d'accès tech — décision joueur #2 : PAS un nouveau
 * seuil). Gate les paliers de la Merveille (FORGE≥3, SOCIÉTÉ≥4, SAVOIR≥6). */
int endgame_metab_count(const World *w, const WorldEconomy *econ, int cid);

/* Requis de métabolisation du palier COURANT de la Merveille (3/4/6 ; 0 si
 * MERV_NONE/ASCENDED — aucun palier actif). Lecteur simple pour le front. */
int endgame_metab_required(MervPhase merv);

/* DÉTAIL PAR HÉRITAGE (P5) — UNE SEULE SOURCE DE VÉRITÉ POUR LA VICTOIRE : ce
 * qu'un héritage compte-t-il POUR LA MERVEILLE (endgame_metab_count_ts), et par
 * quelle voie ? Distinct de la barre d'accès TECH (ai_heritage_access / tier
 * 0..3, pop-share) — un héritage peut être "prêt" côté tech sans compter ici (le
 * seuil diaspora est individualisé, pas pop-share) et inversement. */
typedef struct {
    bool metabolized;      /* compte POUR LA MERVEILLE (gate endgame_metab_count_ts) */
    const char *voie;      /* "natif" | "gouvernance" | "diaspora" | "" (aucune) */
    int  progress_pct;     /* 0..100 : progression de la MEILLEURE voie disponible */
} EndgameHeritageDetail;

void endgame_heritage_detail(const World *w, const WorldEconomy *econ, const TechState ts[],
                             int cid, EndgameHeritageDetail out[HERITAGE_COUNT]);

/* INTENSITÉ D'UNE RÉGION [0..1] selon la fin latchée — pur, aucun état muté :
 * EAU (englouti=1 / programmé=0.6 / adjacent à une engloutie≈0.3), FROID (rampe
 * globale, un rien modulée par la température locale), RONCES (fraction de
 * cellules BIO_THORNS de la région), SANG (la marque sang_scar). 0 si aucune fin
 * ou région hors bornes. Lu par le front (V3, lavis par variante) — jamais par
 * viewer.c directement (passe par une façade). */
float endgame_region_intensity(const EndgameState *eg, const World *w,
                               const WorldEconomy *econ, int region);

/* LOT F (2026-07-08) — L'EXODE AVANT LA MORT : télémétrie (RAZ par sim dans
 * endgame_init, module-static, non sérialisée — comme demography_refugee_*). */
void endgame_exodus_reset(void);
long endgame_exodus_count(void);   /* âmes évacuées cumulées (EAU/FROID/RONCES/SANG) */

#endif /* SCPS_ENDGAME_H */
