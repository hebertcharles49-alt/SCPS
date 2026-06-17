#ifndef SCPS_LANG_H
#define SCPS_LANG_H
/*
 * scps_lang.h — LOCALISATION COMPILÉE (brief table de chaînes)
 *
 * Doctrine : binaire-unique-zéro-asset. Les langues sont des TABLES compilées
 * dans l'exe, commutables au runtime (lang_set + redraw — aucune chaîne n'est
 * cachée dans un état persistant). Le français est la langue de référence ;
 * l'anglais est une table jumelle née copie du FR.
 *
 * Le contrat du COMPILATEUR : strings_ids.h est l'unique liste (X-macro) ;
 * chaque table couvre STR__COUNT par initialiseurs désignés — une entrée
 * manquante ou orpheline casse le build. Pas de clé fantôme, pas de runtime
 * check, pas de fichier externe.
 *
 * PÉRIMÈTRE (clôture, cf. CLAUDE.md) : seules les surfaces FACE-JOUEUR se
 * localisent (viewer, membrane readout, shell, previews). chronicle/econ_scan/
 * batch/télémétrie/commentaires : français pour toujours — l'outillage de
 * l'ingénieur, pas le jeu. Le défaut est LANG_FR : un binaire headless qui
 * n'appelle jamais lang_set est octet-pour-octet inchangé.
 */
#include <stddef.h>
#include "strings_ids.h"

typedef enum {
#define X(id, txt) id,
    SCPS_STRINGS(X)
#undef X
    STR__COUNT
} StrId;

typedef enum { LANG_FR = 0, LANG_EN, LANG_COUNT } Lang;

const char *tr(StrId id);
void lang_set(Lang l);
Lang lang_get(void);
const char *lang_name(Lang l);   /* « Français » / « English » (face-joueur) */

/* Une PLAGE d'ids (mots de bande, pages de tuto) : base + idx, borné. */
const char *tr_band(StrId base, int idx, int count);

/* §SURCHARGE PAR FICHIER (brief « tout le texte joueur éditable » ; rupture
 * ASSUMÉE de zéro-asset). Un fichier externe surcharge n'importe quel STR_* par
 * son ID ; les défauts compilés restent (le binaire tourne sans le fichier).
 * Display-only : le moteur/déterminisme n'y touchent pas. Sert aussi de TRADUCTION.
 *   - lang_load_file : charge les surcharges ; renvoie le nb chargé, -1 si absent.
 *   - lang_dump_file : écrit la liste ÉDITABLE complète (point de départ).
 *   - lang_id_name   : "STR_…" d'un id (l'« adresse » du brief). */
int  lang_load_file(const char *path);
int  lang_dump_file(const char *path);
void lang_clear_overrides(void);
const char *lang_id_name(StrId id);

/* Chaînes PARAMÉTRÉES : emplacements POSITIONNELS {0}..{9} — l'ordre des mots
 * n'est pas universel (l'anglais peut inverser sans toucher l'appelant).
 * Substitution bornée, sans allocation. Les arguments sont des chaînes ; le
 * site d'appel en passe autant que le PLUS GRAND indice utilisé par la clé
 * (dans TOUTES les langues). Pluriels : deux clés (STR_X_UN/STR_X_PLUSIEURS)
 * là où c'est nécessaire, et seulement là.
 *
 * MINI-SPEC `{n|spec}` (rétro-compatible : `{0}` inchangé). Après la barre, des
 * lettres mettent en FORME l'argument (qui reste une CHAÎNE — le contrat varargs
 * ne bouge pas, le moteur passe ses nombres déjà rendus en texte) :
 *   n  — groupe les chiffres par milliers, séparateur ESPACE FINE insécable
 *        (« 48000 » → « 48 000 ») ; ne touche qu'aux chiffres, signe/suffixe gardés ;
 *   +  — force un signe explicite devant un nombre positif (« 12 » → « +12 ») ;
 *   %  — appose un « % » (« 40 » → « 40 % » en FR, « 40% » en EN — l'espace fine
 *        suit la langue).
 * Combinables : `{0|n+}`, `{2|n%}`. La barre+spec est une convention d'AFFICHAGE,
 * jamais lue par le moteur ; une clé sans barre est traitée à l'identique. */
void tr_fmt(char *out, size_t n, StrId id, ...);

/* ====================================================================== *
 * EMPREINTE PAR STR_* (FNV-1a 32 bits) — le SCEAU d'une chaîne.
 *
 * Sert le CLIQUET de traduction : un scps_lang.txt édité référence des IDs ;
 * l'empreinte du DÉFAUT compilé permet de repérer qu'une clé a CHANGÉ de sens
 * (texte FR de référence modifié) entre deux versions — l'override visant
 * l'ancienne formulation est alors PÉRIMÉ et doit être relu.
 *   - lang_fnv      : empreinte du texte de RÉFÉRENCE (FR) d'un id.
 *   - lang_fnv_str  : empreinte d'une chaîne arbitraire (même algo).
 *   - lang_dump_fingerprints : « STR_ID<TAB>hash<TAB>texte » pour audit/diff.
 *   - lang_audit_file : compare un scps_lang.txt au set COMPILÉ et signale les
 *     IDs INCONNUS (périmés/supprimés) et MANQUANTS. Renvoie le nb d'anomalies
 *     (0 = sain), -1 si le fichier est absent. Écrit le détail sur `rep_FILE`
 *     (un FILE* ; stderr si NULL). N'altère PAS les overlays courants (audit pur). */
unsigned long lang_fnv(StrId id);
unsigned long lang_fnv_str(const char *s);
int  lang_dump_fingerprints(const char *path);
int  lang_audit_file(const char *path, void *rep_FILE);

/* ====================================================================== *
 * GLOSSAIRE DES CONCEPTS (hover_*) — un registre DÉFINITION+ALIAS+CATÉGORIE.
 *
 * Les survols (hover_*) parlent de CONCEPTS (Stabilité, Légitimité, Marché…) ;
 * ce registre en donne la définition canonique, des alias (mots employés dans
 * le readout) et une catégorie. Display-only, indépendant du moteur. Exposé pour
 * l'outillage readout (le `--dump-readout` de l'agent 1 le parcourt). */
typedef enum {
    GLOSS_CAT_ETAT = 0,   /* la couronne : stabilité, légitimité, cohésion… */
    GLOSS_CAT_ECONOMIE,   /* richesse, marché, ressources, commerce          */
    GLOSS_CAT_PROVINCE,   /* la province : humeur, lignée, agitation          */
    GLOSS_CAT_SAVOIR,     /* recherche, archétypes, arcane, présage           */
    GLOSS_CAT_COUNT
} GlossCat;

typedef struct {
    StrId       term;     /* le mot-titre du concept (un STR_* face-joueur)   */
    StrId       def;      /* sa définition (typiquement un STR_HOVER_*)        */
    GlossCat    cat;      /* la rubrique                                       */
    const char *alias;    /* synonymes séparés par '|' (NULL si aucun) — bruts,
                           * hors-table À DESSEIN (clés de recherche, pas du
                           * texte affiché ; l'audit lang-check les ignore).  */
} GlossEntry;

int                 glossary_count(void);            /* nb de concepts          */
const GlossEntry   *glossary_at(int i);              /* l'entrée i, ou NULL      */
const GlossEntry   *glossary_find(const char *key);  /* par terme/alias, ou NULL */
const char         *glossary_cat_name(GlossCat c);   /* libellé de rubrique (FR) */

#endif /* SCPS_LANG_H */
