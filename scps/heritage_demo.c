/*
 * heritage_demo.c — banc d'essai du roster & des traits (console)
 *
 *   make heritage_demo && ./heritage_demo
 *
 * Valide le système et sort ≠ 0 si un contrôle échoue (CI) :
 *   1. Structure : 6 atouts + 6 défauts par catégorie ; antonymes réciproques,
 *      même catégorie, signes opposés.
 *   2. Roster : les 6 héritages par défaut sont VALIDES (un trait/catégorie,
 *      équilibrées à 0).
 *   3. Flexibilité Stellaris : clanique charismatique & ésotérique belliqueux légaux.
 *   4. Le validateur REJETTE les builds illégaux (déséquilibrés, hors catégorie).
 *
 * NB — les deux exemples de flexibilité du document de design avaient des
 * coquilles (le clanique mettait deux traits Sociaux ; l'ésotérique « corrigé » tombait
 * à +2). On montre ici les versions LÉGALES ; le validateur est la vérité.
 */
#include "scps_heritage.h"
#include <stdio.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

static void print_build(const char *who, const HeritageBuild *b){
    int pos=0,neg=0;
    for (int c=0;c<CAT_COUNT;c++){ const TraitDef *d=trait_def(b->trait[c]);
        if (d->pts>0) pos++; else if (d->pts<0) neg++; }
    printf("   %-22s %-14s %-14s %-16s  %d+/%d−  %s\n",
           who, trait_name(b->trait[CAT_PHYSIQUE]),
           trait_name(b->trait[CAT_SOCIAL]), trait_name(b->trait[CAT_INTELLECTUEL]),
           pos, neg, build_is_valid(b)?"✓ valide":"✗ INVALIDE");
}

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" ROSTER & TRADITIONS — une tradition par axe, FORCÉ 1 majeur + 1 mineur + 1 défaut\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* ---- 1. Structure des pools ---------------------------------------- */
    printf("\n── 1. Structure des 36 traits ──\n");
    int atouts[CAT_COUNT]={0}, defauts[CAT_COUNT]={0};
    bool antos_ok=true, signs_ok=true, cats_ok=true;
    for (int t=0;t<TRAIT_COUNT;t++){
        const TraitDef *d=trait_def((TraitId)t);
        if (d->pts>0) atouts[d->cat]++; else defauts[d->cat]++;
        const TraitDef *an=trait_def(d->antonym);
        if ((int)an->antonym != t)    antos_ok=false;   /* réciprocité */
        if (an->cat != d->cat)        cats_ok=false;    /* même catégorie */
        if ((an->pts>0)==(d->pts>0))  signs_ok=false;   /* signes opposés */
    }
    bool pools_ok=true;
    for (int c=0;c<CAT_COUNT;c++) if (atouts[c]!=6||defauts[c]!=6) pools_ok=false;
    ok("chaque catégorie a 6 atouts + 6 défauts", pools_ok);
    ok("antonymes réciproques", antos_ok);
    ok("antonymes de même catégorie", cats_ok);
    ok("antonymes de signes opposés (atout↔défaut)", signs_ok);

    /* ---- 2. Le roster ------------------------------------------------- */
    printf("\n── 2. Roster (builds par défaut) ──\n");
    bool roster_ok=true;
    for (int r=0;r<HERITAGE_COUNT;r++){
        HeritageBuild b=heritage_default_build((Heritage)r);
        char tag[48]; snprintf(tag,sizeof tag,"%s [%s]",
                               heritage_name((Heritage)r),
                               sphere_name(heritage_sphere((Heritage)r)));
        print_build(tag,&b);
        if (!build_is_valid(&b)) roster_ok=false;
    }
    ok("les 6 héritages par défaut sont valides (1 majeur + 1 mineur + 1 défaut)", roster_ok);

    /* ---- 3. On se compose une culture (versions légales) -------------- */
    printf("\n── 3. On se compose une culture (légalement) ──\n");
    /* Clanique charismatique : Robuste (majeur Phys) + Charismatique (mineur Social) + Indolent (défaut Intel). */
    HeritageBuild clan_charm = {{ T_ROBUSTE, T_CHARISMATIQUE, T_INDOLENT }};
    /* Ésotérique martial : Longévif (mineur Phys) + Belliqueux (majeur Social) + Sourd à l'arcane (défaut Intel). */
    HeritageBuild eso_war    = {{ T_LONGEVIF, T_BELLIQUEUX, T_SOURD_ARCANE }};
    print_build("Clanique charismatique", &clan_charm);
    print_build("Ésotérique martial",     &eso_war);
    ok("clanique charismatique légal (1 maj + 1 min + 1 déf)", build_is_valid(&clan_charm));
    ok("ésotérique martial légal (1 maj + 1 min + 1 déf)",     build_is_valid(&eso_war));

    /* ---- 4. Le validateur rejette l'illégal -------------------------- */
    printf("\n── 4. Le validateur tient la règle ──\n");
    /* Endurant, Charismatique, Frondeur — deux Sociaux, aucun Intellectuel → REJETÉ (axe doublé). */
    HeritageBuild illegal_cat = {{ T_ENDURANT, T_CHARISMATIQUE, T_FRONDEUR }};
    ok("rejette deux traditions du même axe (Frondeur en slot Intel)",
       !build_is_valid(&illegal_cat));
    /* trois atouts → pas de défaut : illégal. */
    HeritageBuild three_pos = {{ T_ROBUSTE, T_BELLIQUEUX, T_INVENTIF }};
    ok("rejette trois atouts (il faut exactement 1 défaut)", !build_is_valid(&three_pos));
    /* deux mineurs, pas de majeur → illégal (il faut 1 majeur +2). */
    HeritageBuild no_major = {{ T_PROLIFIQUE, T_CHARISMATIQUE, T_INDOLENT }};
    ok("rejette l'absence d'atout majeur (2 mineurs + 1 défaut)", !build_is_valid(&no_major));
    /* deux défauts → illégal (il faut exactement 2 atouts : 1 majeur + 1 mineur). */
    HeritageBuild two_neg = {{ T_FRELE, T_FACTIEUX, T_INVENTIF }};
    ok("rejette deux défauts (il faut 1 majeur + 1 mineur)", !build_is_valid(&two_neg));

    /* ---- Leviers composés (le code voit les chiffres ; le joueur des mots) */
    printf("\n── Leviers composés par héritage (échelle moteur) ──\n");
    for (int r=0;r<HERITAGE_COUNT;r++){
        HeritageBuild b=heritage_default_build((Heritage)r);
        HeritageLeviers L=build_leviers(&b);
        printf("   %-9s : démo×%.2f prod×%.2f | K%+.1f P%+.1f H%+.1f arcane%+.1f"
               " fract%+.1f dérive×%.2f infl%+.2f\n",
               heritage_name((Heritage)r),
               1.f+L.demographie, 1.f+L.rendement,
               L.capacite, L.permeabilite, L.coercition, L.arcane,
               L.fracture, 1.f+L.derive, L.influence);
    }

    /* ---- Matrice de sphères (continuum, pas mur) --------------------- */
    printf("\n── Distance de sphère (continuum : 5 demi-ésotériques, 7 demi-claniques) ──\n");
    printf("   %-11s","");
    for (int j=0;j<SPHERE_COUNT;j++) printf("%-11s", sphere_name((Sphere)j));
    printf("\n");
    for (int i=0;i<SPHERE_COUNT;i++){
        printf("   %-11s", sphere_name((Sphere)i));
        for (int j=0;j<SPHERE_COUNT;j++) printf("%-11.0f", sphere_distance((Sphere)i,(Sphere)j));
        printf("\n");
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    return g_fail?1:0;
}
