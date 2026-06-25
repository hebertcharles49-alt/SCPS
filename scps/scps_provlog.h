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

typedef struct {
    int          year;
    int          str_id;   /* libellé via tr() (>=0) ; -1 si on lit `lit` */
    const char  *lit;      /* libellé littéral (nom d'évènement) ; NULL si str_id>=0 */
    signed char  sign;     /* +1 fléau · -1 faveur · 0 neutre */
} ProvLogEntry;

void provlog_reset(void);                 /* RAZ pleine (appelée par sim_init) */
void provlog_set_year(int year);          /* fixe l'an courant (sim_day, 1×/tick) */
void provlog_push_str(int region, int str_id, int sign);
void provlog_push_lit(int region, const char *lit, int sign);

/* bits du masque de modificateurs DYNAMIQUES suivis par le diff annuel */
enum { JMOD_CONQUEST=0, JMOD_SCAR, JMOD_ABONDANCE, JMOD_FERVEUR, JMOD_RECONSTRUCT, JMOD_COUNT };
/* diff du masque ACTIF d'une région vs l'an passé : POUSSE les APPARITIONS.
 * Le PREMIER appel par région AMORCE (pose la base sans rien logguer → pas de
 * spam an-0). Appelé 1×/an/région. */
void provlog_modifier_diff(int region, unsigned mask);

int  provlog_count(int region);                    /* entrées valides (<=PROVLOG_CAP) */
const ProvLogEntry *provlog_at(int region, int i); /* i=0 = la PLUS RÉCENTE ; NULL hors borne */

#endif
