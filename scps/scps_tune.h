#ifndef SCPS_TUNE_H
#define SCPS_TUNE_H
/*
 * scps_tune.h — LE REGISTRE DE CONSTANTES (Arc J1).
 *
 * Surcharge de constantes de calibrage par l'environnement, pour le balayage de
 * grille (tools/calibrate.py). Sans SCPS_TUNE : valeurs compilées, lecture à
 * l'init, coût nul, sortie BYTE-IDENTIQUE à avant.
 *
 *   - tune_init()   : parse SCPS_TUNE une fois ; nom INCONNU → stderr + exit(2).
 *   - tune_f(n,d)   : la valeur active (surcharge env, sinon défaut compilé d).
 *   - tune_list()   : `--tunables` — nom · défaut · valeur active.
 *   - tune_print_active(f) : en-tête d'un run — les surcharges actives (auto-doc).
 *
 * Membrane : aucun de ces outils ne touche le viewer.
 */
#include <stdio.h>

void  tune_init(void);                 /* idempotent ; appelé au démarrage des outils */
float tune_f(const char *name, float def);
/* Surcharge PROGRAMMATIQUE (bancs : fixtures stables sans bricoler l'environnement).
 * Marque le tunable comme surchargé (apparaît dans tune_active_string). No-op si nom inconnu. */
void  tune_set(const char *name, float val);
void  tune_list(FILE *out);            /* nom + défaut + valeur active (toutes les entrées) */
void  tune_print_active(FILE *out);    /* uniquement les surcharges actives (rien si aucune) */
int   tune_n_active(void);             /* nb de surcharges actives (0 = run nominal) */
const char *tune_active_string(void);  /* "N=V,N=V" des surcharges actives ("" si aucune) — pour le CSV */

#endif /* SCPS_TUNE_H */
