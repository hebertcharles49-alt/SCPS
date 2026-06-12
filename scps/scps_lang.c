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
/* Renvoie l'index, ou -1 si inconnu. SIGNÉ exprès : StrId est un enum à valeurs
 * toutes ≥0, donc le compilateur le rend NON SIGNÉ — un « StrId k=...; if(k<0) »
 * serait TOUJOURS faux et déréférencerait g_override[(unsigned)-1] (hors borne,
 * plantage au démarrage dès qu'un ID périmé traîne dans scps_lang.txt). */
static int lang_id_from_name(const char *name){
    for (int i=0;i<STR__COUNT;i++) if (strcmp(TABLE_NAME[i],name)==0) return i;
    return -1;
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
        char sep=*p; *p++='\0';                        /* termine l'ID sur le séparateur */
        /* Le dump pose UNE tabulation : le texte commence alors JUSTE après — les
         * espaces de tête sont SIGNIFICATIFS et préservés (p.ex. " — …"). Si l'ID a
         * été clos par un espace (fichier édité à la main), on saute le blanc résiduel. */
        if (sep!='\t') while (*p==' '||*p=='\t') p++;
        char *txt=p;
        size_t L=strlen(txt);                          /* retire le saut de ligne final */
        while (L>0 && (txt[L-1]=='\n'||txt[L-1]=='\r')) txt[--L]='\0';
        /* dé-échappe \n \r \t \\ EN PLACE : le dump pose ces séquences pour qu'une
         * valeur (même multi-ligne, p.ex. une page de tutoriel) tienne sur UNE
         * ligne physique « ID<TAB>valeur » et se relise sans ambiguïté. */
        { char *w=txt;
          for (const char *r=txt; *r; r++){
              if (*r=='\\' && r[1]){
                  r++;
                  switch (*r){ case 'n': *w++='\n'; break; case 'r': *w++='\r'; break;
                               case 't': *w++='\t'; break; case '\\': *w++='\\'; break;
                               default:  *w++='\\'; *w++=*r; break; }  /* inconnue : littérale */
              } else *w++=*r;
          }
          *w='\0'; L=(size_t)(w-txt); }
        int k=lang_id_from_name(id);
        if (k<0) continue;                             /* ID inconnu : on ignore (robuste aux versions) */
        free(g_override[k]);
        g_override[k]=malloc(L+1);
        if (g_override[k]){ memcpy(g_override[k],txt,L+1); n++; }
    }
    fclose(f);
    return n;
}

/* Écrit s en ÉCHAPPANT \ \n \r \t — pour que toute valeur (même multi-ligne)
 * tienne sur UNE ligne physique « ID<TAB>valeur » (le chargeur dé-échappe). */
static void lang_fputs_escaped(FILE *f, const char *s){
    for (; *s; s++){
        switch (*s){
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n",  f); break;
            case '\r': fputs("\\r",  f); break;
            case '\t': fputs("\\t",  f); break;
            default:   fputc((unsigned char)*s, f);
        }
    }
}

/* Écrit la liste ÉDITABLE complète (ID <TAB> texte de la langue courante) — le
 * point de départ que le joueur/traducteur édite. Renvoie le nb de lignes, -1 si échec. */
int lang_dump_file(const char *path){
    FILE *f=fopen(path,"wb");
    if (!f) return -1;
    fputs("# scps_lang.txt — TOUT le texte joueur, éditable. Format : STR_ID<TAB>texte\n"
          "# Édite après la tabulation, relance le jeu (le binaire garde ses défauts si ce fichier manque).\n"
          "# Une valeur tient sur UNE ligne : \\n = saut de ligne, \\t = tabulation, \\\\ = antislash.\n"
          "# Une langue = un fichier (copie, traduis, charge). '#' ou ligne vide = ignoré.\n\n", f);
    for (int i=0;i<STR__COUNT;i++){
        fputs(TABLE_NAME[i], f); fputc('\t', f);
        lang_fputs_escaped(f, tr((StrId)i));           /* échappe \n \t \\ → UNE ligne physique */
        fputc('\n', f);
    }
    fclose(f);
    return STR__COUNT;
}
