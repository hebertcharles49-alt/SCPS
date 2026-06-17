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
#include <stdint.h>

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

/* §MINI-SPEC `{n|spec}` (brief loc) — un emplacement positionnel peut porter une
 * mini-spec d'AFFICHAGE après la barre. On reconnaît :  '{' DIGIT '}'  OU
 * '{' DIGIT '|' SPEC… '}'  où SPEC est un court run de lettres de format. La
 * fonction renvoie 1 (match), pose l'indice *idx et les drapeaux *flags, et
 * avance *after au caractère SUIVANT le '}'. Bornée : SPEC ne dépasse pas un
 * petit run (au-delà = pas un placeholder, le '{' s'imprime littéralement). */
enum { FMT_GROUP=1, FMT_SIGN=2, FMT_PCT=4 };
static int fmt_match(const char *p, int *idx, unsigned *flags, const char **after){
    if (p[0]!='{' || p[1]<'0' || p[1]>'9') return 0;
    *idx = p[1]-'0'; *flags = 0;
    if (p[2]=='}'){ *after = p+3; return 1; }       /* {n} pur — rétro-compatible */
    if (p[2]!='|') return 0;
    const char *q = p+3;
    unsigned f = 0;
    for (int k=0; k<8 && *q && *q!='}'; k++, q++){
        switch (*q){
            case 'n': f|=FMT_GROUP; break;
            case '+': f|=FMT_SIGN;  break;
            case '%': f|=FMT_PCT;   break;
            default:  return 0;     /* lettre inconnue → pas un placeholder valide */
        }
    }
    if (*q!='}') return 0;          /* non terminé (ou spec trop longue) */
    *flags = f; *after = q+1; return 1;
}

/* L'espace fine insécable U+202F — séparateur de milliers et avant le % français. */
#define FMT_THINSP "\xe2\x80\xaf"
static int fmt_isdigit(char c){ return c>='0' && c<='9'; }

/* Écrit l'argument `a` mis en FORME selon `flags` dans out[o..n[, renvoie le
 * nouvel o. Sans allocation, un seul passage. Le groupement par milliers n'opère
 * que sur la PARTIE ENTIÈRE de tête (chiffres consécutifs après un signe
 * optionnel) — un suffixe non chiffré (« /j », décimales, « k »…) est recopié
 * tel quel. Un argument non numérique passe inchangé (les drapeaux n'ont d'effet
 * que sur un nombre). */
static size_t fmt_emit(char *out, size_t n, size_t o, const char *a, unsigned flags){
    if (!a) a="";
    const char *p = a;
    int neg = (*p=='-'), had_sign = (*p=='-'||*p=='+');
    if (had_sign) p++;                               /* p pointe sur la 1re décimale */
    int ndig = 0; while (fmt_isdigit(p[ndig])) ndig++;
    int is_number = (ndig>0);

    if (!is_number){                                 /* pas un nombre : copie brute */
        while (*a && o+1<n) out[o++]=*a++;
        return o;
    }
    /* le signe explicite (+) : seulement pour un positif sans signe déjà présent */
    if ((flags&FMT_SIGN) && !had_sign && o+1<n) out[o++]='+';
    if (had_sign && o+1<n) out[o++] = neg ? '-' : '+';   /* signe d'origine conservé */

    if (flags&FMT_GROUP){
        int first = ndig % 3; if (first==0) first=3;
        for (int i=0;i<ndig;i++){
            if (i>0 && ((i-first)%3)==0){            /* frontière de groupe → espace fine */
                const char *sp=FMT_THINSP; while (*sp && o+1<n) out[o++]=*sp++;
            }
            if (o+1<n) out[o++]=p[i];
        }
        p += ndig;                                   /* le reste (suffixe) suit */
    }
    while (*p && o+1<n) out[o++]=*p++;                /* suffixe (ou tout le nombre si !group) */

    if (flags&FMT_PCT){
        if (lang_get()==LANG_FR){                     /* espace fine avant le % (typo FR) */
            const char *sp=FMT_THINSP; while (*sp && o+1<n) out[o++]=*sp++;
        }
        if (o+1<n) out[o++]='%';
    }
    return o;
}

/* Substitution positionnelle {0}..{9} (+ mini-spec `{n|spec}`), bornée, sans
 * allocation. On lit du va_list exactement (max_index+1) arguments — le format
 * fait le contrat, dans TOUTES les langues (une clé EN "{1}…{0}" lit deux
 * arguments aussi ; la spec d'affichage ne change pas QUELS args sont lus). */
void tr_fmt(char *out, size_t n, StrId id, ...){
    if (!out || n==0) return;
    const char *fmt = tr(id);
    const char *args[10]={0};
    int hi=-1;
    for (const char *p=fmt; *p; ){
        int k; unsigned fl; const char *nx;
        if (fmt_match(p,&k,&fl,&nx)){ if (k>hi) hi=k; p=nx; }
        else p++;
    }
    { va_list ap; va_start(ap, id);
      for (int k=0;k<=hi && k<10;k++){
          const char *a=va_arg(ap, const char*);
          args[k]=a?a:"";
      }
      va_end(ap); }
    size_t o=0;
    for (const char *p=fmt; *p && o+1<n; ){
        int k; unsigned fl; const char *nx;
        if (fmt_match(p,&k,&fl,&nx)){ o=fmt_emit(out,n,o,args[k],fl); p=nx; }
        else out[o++]=*p++;
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

/* ===================================================================== */
/* EMPREINTE PAR STR_* (FNV-1a 32 bits) — le SCEAU (brief loc §3)         */
/* ===================================================================== */
/* FNV-1a sur la chaîne RÉFÉRENCE (le FR compilé — la « vérité » d'une clé), pas
 * sur la surcharge : deux versions du binaire partagent l'empreinte ssi le texte
 * de référence d'un ID n'a pas bougé. Un override pointant un ID dont l'empreinte
 * a changé vise une ANCIENNE formulation → à relire. 32 bits suffisent (audit,
 * pas crypto). On RENVOIE un unsigned long (≥32 bits garanti) pour l'affichage. */
unsigned long lang_fnv_str(const char *s){
    uint32_t h = 2166136261u;                          /* offset basis FNV-1a/32 */
    if (s) for (const unsigned char *p=(const unsigned char*)s; *p; p++){
        h ^= *p; h *= 16777619u;                        /* prime FNV/32 */
    }
    return (unsigned long)h;
}
unsigned long lang_fnv(StrId id){
    if (id<0 || id>=STR__COUNT) return 0;
    return lang_fnv_str(TABLE_FR[id]);                  /* la RÉFÉRENCE, jamais la surcharge */
}

/* Dump « STR_ID<TAB>hash<TAB>texte(réf) » — le manifeste d'empreintes que l'on
 * commite/diffe pour repérer une clé dont le sens a changé. Renvoie le nb de
 * lignes, -1 si échec. (Outil ingénieur : en-têtes français, hors clôture.) */
int lang_dump_fingerprints(const char *path){
    FILE *f=fopen(path,"wb");
    if (!f) return -1;
    fputs("# scps_lang.fnv — empreinte FNV-1a/32 du texte de RÉFÉRENCE (FR) par ID.\n"
          "# Format : STR_ID<TAB>0xHASH<TAB>texte. Un hash qui change = clé au sens modifié.\n\n", f);
    for (int i=0;i<STR__COUNT;i++){
        fprintf(f, "%s\t0x%08lx\t", TABLE_NAME[i], lang_fnv((StrId)i));
        lang_fputs_escaped(f, TABLE_FR[i]);
        fputc('\n', f);
    }
    fclose(f);
    return STR__COUNT;
}

/* AUDIT d'un scps_lang.txt face au set COMPILÉ (brief loc §3, flag --lang-audit).
 * Lit le fichier en MIROIR du chargeur (même découpe ID/texte), mais SANS toucher
 * aux overlays : il ne fait que JUGER. Signale —
 *   · ID INCONNU  : présent dans le fichier, absent du binaire → clé périmée/
 *                   supprimée (l'override est mort) ;
 *   · ID MANQUANT : présent dans le binaire, jamais cité par le fichier → un
 *                   traducteur en oublie (couverture incomplète).
 * Renvoie le nombre d'anomalies (0 = sain), -1 si le fichier est absent. Le détail
 * va sur `rep` (FILE*), stderr si NULL. */
int lang_audit_file(const char *path, void *rep_FILE){
    FILE *rep = rep_FILE ? (FILE*)rep_FILE : stderr;
    FILE *f=fopen(path,"rb");
    if (!f){ fprintf(rep, "[lang-audit] absent : %s\n", path); return -1; }
    char seen[STR__COUNT];
    memset(seen, 0, sizeof seen);
    int unknown=0, dup=0, lineno=0;
    char line[1024];
    while (fgets(line,sizeof line,f)){
        lineno++;
        char *p=line;
        while (*p==' '||*p=='\t') p++;
        if (*p=='#' || *p=='\n' || *p=='\r' || *p=='\0') continue;
        char *id=p;
        while (*p && *p!=' ' && *p!='\t' && *p!='\n' && *p!='\r') p++;
        if (!*p || *p=='\n' || *p=='\r') continue;     /* pas de texte → ligne ignorée (comme le chargeur) */
        *p='\0';                                       /* termine l'ID */
        int k=lang_id_from_name(id);
        if (k<0){
            fprintf(rep, "[lang-audit] ligne %d — ID PÉRIMÉ/INCONNU : %s\n", lineno, id);
            unknown++;
        } else {
            if (seen[k]) dup++;
            seen[k]=1;
        }
    }
    fclose(f);
    int missing=0;
    for (int i=0;i<STR__COUNT;i++) if (!seen[i]){
        fprintf(rep, "[lang-audit] ID MANQUANT (non surchargé) : %s\n", TABLE_NAME[i]);
        missing++;
    }
    if (dup) fprintf(rep, "[lang-audit] %d ligne(s) en DOUBLE (dernière l'emporte au chargement).\n", dup);
    int anomalies = unknown + missing;
    fprintf(rep, "[lang-audit] %s : %d périmé(s), %d manquant(s)%s.\n",
            anomalies ? "ANOMALIES" : "SAIN", unknown, missing,
            dup ? " (+ doublons)" : "");
    return anomalies;
}

/* ===================================================================== */
/* GLOSSAIRE DES CONCEPTS (hover_*) — registre DÉFINITION+ALIAS+CATÉGORIE */
/* ===================================================================== */
/* Display-only : titre (STR_GLOSS_*) + définition (le STR_HOVER_* existant) +
 * catégorie + alias (clés de recherche brutes, hors-table À DESSEIN). Exposé à
 * l'outillage readout (le --dump-readout de l'agent 1 peut le parcourir). */
static const GlossEntry G_GLOSSARY[] = {
  { STR_GLOSS_STAB,      STR_HOVER_STAB,      GLOSS_CAT_ETAT,     "stabilité|ordre|tenue|vacillante" },
  { STR_GLOSS_LEGIT,     STR_HOVER_LEGIT,     GLOSS_CAT_ETAT,     "légitimité|trône|sacrée|usurpée" },
  { STR_GLOSS_CONCORDE,  STR_HOVER_CONCORDE,  GLOSS_CAT_ETAT,     "concorde|cohésion|unité|sécession" },
  { STR_GLOSS_ASSISE,    STR_HOVER_ASSISE,    GLOSS_CAT_ETAT,     "assise|obéissance|adhésion|coercition" },
  { STR_GLOSS_PROSP,     STR_HOVER_PROSP,     GLOSS_CAT_ECONOMIE, "prospérité|richesse|opulence|disette" },
  { STR_GLOSS_MARCHE,    STR_MARCHE_PRIX_HOV, GLOSS_CAT_ECONOMIE, "marché|prix|offre|demande" },
  { STR_GLOSS_AISANCE,   STR_HOVER_AISANCE,   GLOSS_CAT_ECONOMIE, "aisance|carrefour|flux|commerce" },
  { STR_GLOSS_HUMEUR,    STR_HOVER_HUMEUR,    GLOSS_CAT_PROVINCE, "humeur|loyauté|fronde|couronne" },
  { STR_GLOSS_LIGNEE,    STR_HOVER_LIGNEE,    GLOSS_CAT_PROVINCE, "lignée|culture|sang|assimilation" },
  { STR_GLOSS_AGITATION, STR_HOVER_AGITATION, GLOSS_CAT_PROVINCE, "agitation|colère|révolte|garnison" },
  { STR_GLOSS_SAVOIR,    STR_HOVER_SAVOIR,    GLOSS_CAT_SAVOIR,   "savoir|recherche|arts|arcane" },
  { STR_GLOSS_PRESAGE,   STR_HOVER_PRESAGE,   GLOSS_CAT_SAVOIR,   "présage|brèche|ombre|charge" },
};
enum { GLOSSARY_N = (int)(sizeof G_GLOSSARY / sizeof *G_GLOSSARY) };

int glossary_count(void){ return GLOSSARY_N; }
const GlossEntry *glossary_at(int i){ return (i>=0 && i<GLOSSARY_N) ? &G_GLOSSARY[i] : NULL; }

static const char *G_GLOSS_CATNAME[GLOSS_CAT_COUNT] = {
    "État", "Économie", "Province", "Savoir"
};
const char *glossary_cat_name(GlossCat c){
    return (c>=0 && c<GLOSS_CAT_COUNT) ? G_GLOSS_CATNAME[c] : "?";
}

/* Compare `key` à `cand` sans casse ASCII (les alias sont en minuscules ; le
 * terme affiché peut porter une majuscule). Renvoie 1 si égal sur toute la
 * longueur de `cand` (qui est une tranche entre deux '|'), 0 sinon. */
static int gloss_ieq(const char *key, const char *cand, size_t len){
    for (size_t i=0;i<len;i++){
        char a=key[i], b=cand[i];
        if (a>='A'&&a<='Z') a=(char)(a-'A'+'a');
        if (b>='A'&&b<='Z') b=(char)(b-'A'+'a');
        if (a!=b) return 0;
    }
    return key[len]=='\0';
}

const GlossEntry *glossary_find(const char *key){
    if (!key || !*key) return NULL;
    for (int i=0;i<GLOSSARY_N;i++){
        const GlossEntry *e=&G_GLOSSARY[i];
        if (gloss_ieq(key, tr(e->term), strlen(tr(e->term)))) return e;   /* par titre */
        if (!e->alias) continue;                                          /* par alias */
        const char *a=e->alias;
        while (*a){
            const char *bar=strchr(a,'|');
            size_t len = bar ? (size_t)(bar-a) : strlen(a);
            if (gloss_ieq(key, a, len)) return e;
            if (!bar) break;
            a = bar+1;
        }
    }
    return NULL;
}
