#ifndef SCPS_PROVLOG_H
#define SCPS_PROVLOG_H
/* ──────────────────────────────────────────────────────────────────────────
 * Journal d'évènements PROVINCIAL (display/UI). Un tampon par région — l'ANNEAU
 * des dernières entrées —, GLOBAL et RUNTIME (RAZ par sim, comme g_flux) : RIEN
 * de sérialisé. On y POUSSE :
 *   • les ÉVÈNEMENTS du directeur (inondation, séisme, sécheresse, peste…),
 *     depuis fire_event quand la portée est EV_PROVINCE ;
 *   • l'APPARITION d'un MODIFICATEUR (conquête récente, terre d'abondance,
 *     cicatrice de révolte, ferveur, reconstruction), via un diff ANNUEL.
 * Aucune lecture ne retourne dans la sim → le déterminisme reste intact. */

#define PROVLOG_CAP 12   /* entrées gardées par région (anneau) */

/* stats touchées par une entrée (pour le HOVER « complet ») — 2 bits chacune dans
 * eff_dir : 0 inchangé · 1 hausse · 2 baisse. Les ÉVÈNEMENTS portent un eff_dir ;
 * les MODIFICATEURS portent plutôt un eff_str (leur ligne d'effet déjà rédigée). */
enum { JEFF_POP=0, JEFF_PROD, JEFF_AGIT, JEFF_LEGIT, JEFF_TRESOR, JEFF_N };

typedef struct {
    int          year;
    int          str_id;   /* libellé via tr() (>=0) ; -1 si on lit `lit` */
    const char  *lit;      /* libellé littéral (nom d'évènement) ; NULL si str_id>=0 */
    signed char  sign;     /* +1 fléau · -1 faveur · 0 neutre */
    int          eff_str;  /* MODIFICATEUR : sa ligne d'effet (STR_*) ; -1 sinon */
    unsigned     eff_dir;  /* ÉVÈNEMENT : directions par stat (2 bits/JEFF_*) ; 0 sinon */
} ProvLogEntry;

void provlog_reset(void);                 /* RAZ pleine (appelée par sim_init) */
void provlog_set_year(int year);          /* fixe l'an courant (sim_day, 1×/tick) */
/* MODIFICATEUR : nom STR_* + signe + sa ligne d'effet STR_* (-1 si aucune). */
void provlog_push_mod(int region, int str_id, int sign, int eff_str);
/* ÉVÈNEMENT : nom littéral + signe + directions d'effet par stat (2 bits/JEFF_*). */
void provlog_push_event(int region, const char *lit, int sign, unsigned eff_dir);

/* bits du masque de modificateurs DYNAMIQUES suivis par le diff annuel */
enum { JMOD_CONQUEST=0, JMOD_SCAR, JMOD_ABONDANCE, JMOD_FERVEUR, JMOD_RECONSTRUCT, JMOD_COUNT };
/* diff du masque ACTIF d'une région vs l'an passé : POUSSE les APPARITIONS.
 * Le PREMIER appel par région AMORCE (pose la base sans rien logguer → pas de
 * spam an-0). Appelé 1×/an/région. */
void provlog_modifier_diff(int region, unsigned mask);

int  provlog_count(int region);                    /* entrées valides (<=PROVLOG_CAP) */
const ProvLogEntry *provlog_at(int region, int i); /* i=0 = la PLUS RÉCENTE ; NULL hors borne */

/* ── LE FIL D'ÉVÈNEMENTS GLOBAL (display/UI) — la voie « ce qui ARRIVE » des alertes
 * (victoires, pertes, guerre/paix, pillages, révoltes, sécessions…). MÊME CHARTE que
 * le journal : anneau RUNTIME (RAZ par provlog_reset), le moteur y POUSSE et ne le
 * RELIT JAMAIS → déterminisme intact ; rien de sérialisé. Les pushes vivent dans
 * scps_sim.c par OBSERVATION d'état, GATÉS human_player ≥ 0 → la chronique ne pousse
 * RIEN (zéro coût, zéro bruit).
 *
 * AJOUTER UN ÉVÈNEMENT PAS ENCORE PRÉVU (le mode d'emploi, 3 gestes) :
 *   1. une valeur FeedKind ci-dessous ;
 *   2. un feed_push(...) au site d'OBSERVATION (scps_sim.c, gaté joueur) — ou, si
 *      l'évènement n'est pas observable d'état, au site moteur qui le produit
 *      (même régime que provlog_push_event : write-only, jamais relu) ;
 *   3. une entrée dans la table du front (alerts.gd : icône · couleur de domaine ·
 *      libellé). C'est TOUT — le poll, l'anneau et la membrane sont déjà là. */
#define FEED_CAP 64
typedef enum {
    FEED_NONE = 0,
    FEED_WAR_DECLARED,   /* la GUERRE : a = l'autre pays, b = le joueur */
    FEED_PEACE,          /* la PAIX signée : a = l'autre pays · v = SCORE DE GUERRE final
                          * (±100, point de vue du JOUEUR — le verdict gagné/perdu/blanche) */
    FEED_SIEGE_FALLEN,   /* une place est TOMBÉE : a = le preneur, b = l'ancien tenant, region */
    FEED_LIBERATED,      /* une place REPRISE par les armes : a = le libérateur, region */
    FEED_PILLAGE,        /* une côte/province BALAFRÉE (sac) : region */
    FEED_REVOLT,         /* un soulèvement ÉCLATE : region */
    FEED_SECESSION,      /* un pays NAÎT d'une sécession : a = le nouveau pays */
    FEED_BATTLE_WON,     /* BATAILLE RANGÉE gagnée : b = l'adversaire (-1 si inconnu), region */
    FEED_BATTLE_LOST,    /* … perdue (l'ost est brisé) : b = l'adversaire, region */
    FEED_DIRECTOR,       /* ÉVÈNEMENT DU DIRECTEUR (inondation, peste, créuset…) : v = EvId
                          * (nom résolu à la façade) · region si provincial, sinon a = pays visé.
                          * Poussé par scps_events (non gaté — le FRONT filtre la pertinence). */
    FEED_COUNT
} FeedKind;
typedef struct { int seq, year, kind, a, b, region, v; } FeedEntry;   /* v = valeur libre (score…) */
/* FOCUS du fil = le pays SUIVI (le joueur) : -1 (défaut/chronique) → TOUT est jeté au
 * push (zéro écriture) ; sinon on ne garde que les entrées qui le CONCERNENT (a==focus,
 * b==focus, ou a<0 = déjà scopé par l'appelant). Filtrer À L'ENTRÉE évite qu'un monde de
 * 200 pays évince les évènements du joueur de l'anneau. Posé par la façade/le viewer
 * après generate/load (jamais par la chronique). */
void feed_set_focus(int cid);
void feed_push(int kind, int a, int b, int region, int v);/* write-only (l'an vient de provlog_set_year) */
int  feed_poll(int after_seq, FeedEntry *out, int max);   /* entrées seq > after_seq, ordre chrono */

#endif
