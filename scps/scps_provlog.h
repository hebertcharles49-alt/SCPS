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

#endif
