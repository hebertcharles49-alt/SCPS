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
#include <stdint.h>

void  tune_init(void);                 /* idempotent ; appelé au démarrage des outils */
float tune_f(const char *name, float def);
/* Surcharge PROGRAMMATIQUE (bancs : fixtures stables sans bricoler l'environnement).
 * API historique : les fautes restent silencieuses via tune_set(). Les nouveaux
 * appelants (UI/bancs) utilisent tune_set_checked() pour distinguer une mutation
 * acceptée d'un nom inconnu ou d'une valeur non finie. */
void  tune_set(const char *name, float val);
int   tune_set_checked(const char *name, float val); /* 1=accepté, 0=refusé sans mutation */
int   tune_reset(const char *name);            /* 1=réinitialisé au défaut, 0=nom/inactif */
void  tune_list(FILE *out);            /* nom + défaut + valeur active (toutes les entrées) */
void  tune_print_active(FILE *out);    /* uniquement les surcharges actives (rien si aucune) */
int   tune_n_active(void);             /* nb de surcharges actives (0 = run nominal) */
const char *tune_active_string(void);  /* "N=V,N=V" des surcharges actives ("" si aucune) — pour le CSV */
uint32_t    tune_fingerprint32(void);  /* empreinte de tous les noms et bits des valeurs effectives */
uint32_t    tune_legacy_fingerprint32(void); /* compat. anciennes chaînes tronquées à 1023 octets */
unsigned    tune_revision(void);       /* progresse à chaque surcharge acceptée */

/* Énumération du registre (panneau dev Godot : lister/éditer en direct). */
int         tune_count(void);
const char *tune_name_at(int i);
float       tune_value_at(int i);      /* valeur ACTIVE (surcharge ou défaut) */
float       tune_default_at(int i);
int         tune_overridden_at(int i);
int         tune_is_active(const char *name); /* 0 pour une clé conservée mais explicitement inactive */
const char *tune_phase(const char *name);     /* phase moteur : inactive/diagnostic/new_world/rule_read/next_action */

#endif /* SCPS_TUNE_H */
