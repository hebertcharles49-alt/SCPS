/*
 * scps_lang.c — les tables compilées + tr/tr_fmt (voir scps_lang.h).
 *
 * LE CONTRAT DU COMPILATEUR : TABLE_FR sort de la MÊME X-liste que l'enum
 * (divergence impossible) ; TABLE_EN se construit positionnellement sur sa
 * liste jumelle et l'assert de taille (C99, typedef tableau négatif) casse le
 * build si une ligne manque ou déborde. Pas de clé fantôme, pas de fichier
 * externe : `strings` de l'exe contient les deux langues.
 */
#include "scps_lang.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "strings_en.h"

static const char *TABLE_FR[] = {
#define X(id, txt) txt,
    SCPS_STRINGS(X)
#undef X
};
static const char *TABLE_EN[] = {
#define X(id, txt) txt,
    SCPS_STRINGS_EN(X)
#undef X
};
/* Le NOM de chaque id ("STR_…"), né de la MÊME X-liste → l'adresse éditable du
 * brief « un ID = une ligne du fichier ». Sert au chargement et au dump. */
static const char *TABLE_NAME[] = {
#define X(id, txt) #id,
    SCPS_STRINGS(X)
#undef X
};
/* complétude vérifiée À LA COMPILATION : retirer une ligne → taille négative */
typedef char scps_assert_fr_complete[(sizeof TABLE_FR/sizeof *TABLE_FR==STR__COUNT)?1:-1];
typedef char scps_assert_en_complete[(sizeof TABLE_EN/sizeof *TABLE_EN==STR__COUNT)?1:-1];

static Lang g_lang = LANG_FR;   /* la référence — un binaire headless n'y touche jamais */

/* §SURCHARGE RUNTIME (brief « tout le texte joueur éditable »). Un fichier externe
 * (scps_lang.txt) surcharge n'importe quel STR_* par son ID. Les défauts compilés
 * restent (le binaire tourne SEUL, sans le fichier) ; le fichier ne fait que
 * REMPLACER l'affichage. Display-only : le moteur/les bancs/le déterminisme n'y
 * touchent jamais. Sert AUSSI de traduction (un fichier = une langue surchargée).
 * On assume la rupture de « zéro asset » : c'est un choix, pour l'édition à chaud. */
static char *g_override[STR__COUNT];   /* NULL = défaut compilé ; sinon copie possédée */

const char *lang_id_name(StrId id){ return (id>=0&&id<STR__COUNT)?TABLE_NAME[id]:"?"; }

void lang_clear_overrides(void){
    for (int i=0;i<STR__COUNT;i++){ free(g_override[i]); g_override[i]=NULL; }
}
static StrId lang_id_from_name(const char *name){
    for (int i=0;i<STR__COUNT;i++) if (strcmp(TABLE_NAME[i],name)==0) return (StrId)i;
    return (StrId)-1;
}

void lang_set(Lang l){ if (l>=0 && l<LANG_COUNT) g_lang=l; }
Lang lang_get(void){ return g_lang; }
const char *lang_name(Lang l){
    return (l==LANG_EN) ? "English" : "Français";
}

const char *tr(StrId id){
    if (id<0 || id>=STR__COUNT) return "?";
    if (g_override[id]) return g_override[id];                 /* le fichier l'emporte */
    const char *s = (g_lang==LANG_EN) ? TABLE_EN[id] : TABLE_FR[id];
    return s ? s : "?";
}

const char *tr_band(StrId base, int idx, int count){
    if (idx<0 || idx>=count) return "?";
    return tr((StrId)(base+idx));
}

/* Substitution positionnelle {0}..{9}, bornée, sans allocation. On lit du
 * va_list exactement (max_index+1) arguments — le format fait le contrat,
 * dans TOUTES les langues (une clé EN "{1}…{0}" lit deux arguments aussi). */
void tr_fmt(char *out, size_t n, StrId id, ...){
    if (!out || n==0) return;
    const char *fmt = tr(id);
    const char *args[10]={0};
    int hi=-1;
    for (const char *p=fmt; *p; p++)
        if (p[0]=='{' && p[1]>='0' && p[1]<='9' && p[2]=='}'){
            int k=p[1]-'0'; if (k>hi) hi=k;
        }
    { va_list ap; va_start(ap, id);
      for (int k=0;k<=hi && k<10;k++){
          const char *a=va_arg(ap, const char*);
          args[k]=a?a:"";
      }
      va_end(ap); }
    size_t o=0;
    for (const char *p=fmt; *p && o+1<n; ){
        if (p[0]=='{' && p[1]>='0' && p[1]<='9' && p[2]=='}'){
            const char *a=args[p[1]-'0']; if (!a) a="";
            while (*a && o+1<n) out[o++]=*a++;
            p+=3;
        } else out[o++]=*p++;
    }
    out[o]='\0';
}

/* ===================================================================== */
/* SURCHARGE PAR FICHIER (brief « tout éditable », rupture assumée de zéro-asset) */
/* ===================================================================== */
/* Une ligne = « STR_ID <TAB ou espaces> texte… ». '#' ou ligne vide = ignoré.
 * Le texte va jusqu'à la fin de ligne (espaces/ponctuation/UTF-8 admis). Un ID
 * inconnu est ignoré (robuste aux versions). Renvoie le nb d'entrées chargées,
 * -1 si le fichier est absent (le binaire garde alors ses défauts compilés). */
int lang_load_file(const char *path){
    FILE *f=fopen(path,"rb");
    if (!f) return -1;
    char line[1024]; int n=0;
    while (fgets(line,sizeof line,f)){
        char *p=line;
        while (*p==' '||*p=='\t') p++;                 /* indentation libre */
        if (*p=='#' || *p=='\n' || *p=='\r' || *p=='\0') continue;
        char *id=p;                                    /* l'ID : jusqu'au 1er séparateur */
        while (*p && *p!=' ' && *p!='\t' && *p!='\n' && *p!='\r') p++;
        if (!*p || *p=='\n' || *p=='\r'){ continue; }  /* pas de texte → ignore */
        *p++='\0';
        while (*p==' '||*p=='\t') p++;                 /* saute le séparateur ID↔texte */
        char *txt=p;
        size_t L=strlen(txt);                          /* retire le saut de ligne final */
        while (L>0 && (txt[L-1]=='\n'||txt[L-1]=='\r')) txt[--L]='\0';
        StrId k=lang_id_from_name(id);
        if (k<0) continue;                             /* ID inconnu : on ignore */
        free(g_override[k]);
        g_override[k]=malloc(L+1);
        if (g_override[k]){ memcpy(g_override[k],txt,L+1); n++; }
    }
    fclose(f);
    return n;
}

/* Écrit la liste ÉDITABLE complète (ID <TAB> texte de la langue courante) — le
 * point de départ que le joueur/traducteur édite. Renvoie le nb de lignes, -1 si échec. */
int lang_dump_file(const char *path){
    FILE *f=fopen(path,"wb");
    if (!f) return -1;
    fputs("# scps_lang.txt — TOUT le texte joueur, éditable. Format : STR_ID<TAB>texte\n"
          "# Édite après la tabulation, relance le jeu (le binaire garde ses défauts si ce fichier manque).\n"
          "# Une langue = un fichier (copie, traduis, charge). '#' ou ligne vide = ignoré.\n\n", f);
    for (int i=0;i<STR__COUNT;i++)
        fprintf(f, "%s\t%s\n", TABLE_NAME[i], tr((StrId)i));
    fclose(f);
    return STR__COUNT;
}
