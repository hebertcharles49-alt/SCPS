/*
 * religion_demo.c — banc du module fondation religion (P1) + lecteurs manquants
 * (mission « lecteurs », 2026-07-06) : religion_fracture_level/credo_drift/scholar_drift.
 * Appelle religion_selftest() : assert() abandonne (rc≠0) si un invariant casse —
 * axes (pole>>1==axe), un-pôle-par-axe, spawn/apply (somme pôles+crédo), schisme
 * (slot conservé + variante couleur proche). Module PUR : aucun moteur lié.
 */
#include "scps_religion.h"
#include "scps_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
#define CK(cond,msg) do{ if (cond) { g_pass++; printf("   \xe2\x9c\x93 %s\n",(msg)); } \
                          else { g_fail++; printf("   \xe2\x9c\x97 %s\n",(msg)); } }while(0)

int main(void){
    printf("== religion : module fondation (16 pôles · 3 crédos) ==\n");
    religion_selftest();
    printf("   \xe2\x9c\x93 selftest OK : axes · un-par-axe · spawn/apply · schisme · couleur\n");

    /* ---- Lecteurs manquants : fracture/credo_drift/scholar_drift --------- */
    printf("\n== lecteurs manquants (mission « lecteurs ») ==\n");
    religion_reset();
    World *w=malloc(sizeof(World)); WorldEconomy *e=malloc(sizeof(WorldEconomy));
    if(!w||!e){ fprintf(stderr,"OOM\n"); return 1; }
    WorldParams p=worldparams_default(9u);
    world_generate(w,&p); econ_init(e,w); gen_population(w,e);

    /* Trouve un pays avec ≥2 régions à lui pour fabriquer une fracture. */
    int cid=-1, r0=-1, r1=-1;
    for (int c=0;c<w->n_countries && cid<0;c++){
        int found[2]={-1,-1}, n=0;
        for (int r=0;r<w->n_regions && n<2;r++)
            if (w->region[r].country==c) found[n++]=r;
        if (n==2){ cid=c; r0=found[0]; r1=found[1]; }
    }
    if (cid<0){ fprintf(stderr,"aucun pays à 2 régions (graine 9) — banc inapplicable\n"); return 1; }

    /* Pop non-nulle sur les deux régions (pop-pondération du lecteur) + un groupe NATIF
     * explicite sur la province représentative (religion_set_region convertit les NATIFS
     * de souche — sans groupe attaché, rien à convertir : religion_of_region resterait -1). */
    e->region[r0].strata[0].pop = 1000.f;
    e->region[r1].strata[0].pop = 1000.f;
    for (int i=0;i<2;i++){
        int rg = i?r1:r0;
        int rp = econ_region_rep_province(e, rg);
        CK(rp>=0, i?"rep-province(r1) valide":"rep-province(r0) valide");
        if (rp<0) continue;
        ProvinceEconomy *pe=&e->prov[rp];
        pe->active=true; pe->colonized=true;
        memset(&pe->pop.groups[0],0,sizeof pe->pop.groups[0]);
        pe->pop.groups[0].count=1000; pe->pop.groups[0].diaspora=false;
        pe->pop.groups[0].integration=1.f; pe->pop.groups[0].faith=-1; pe->pop.groups[0].home_reg=-1;
        pe->pop.n_groups=1;
    }

    /* Le pays est ATHÉE : aucun signal à fracturer. */
    CK(religion_fracture_level(w,e,cid) == 0.f, "athée → fracture_level = 0");
    CK(religion_credo_drift(w,e,cid) == 0.f,    "athée → credo_drift (alias) = 0");
    CK(religion_scholar_drift(cid) == 0.f,       "aucun lettré actif → scholar_drift = 0");

    /* Fonde une foi, les DEUX régions professent la foi d'État → 0 (uniforme). */
    int trad[3]={RP_FECONDITE, RP_TRANSE, RP_ACCUEIL};
    int rid = religion_spawn(CREDO_PLURALISTE, trad, /*centre_cell*/0, cid, NULL);
    if (rid < 0){ fprintf(stderr,"religion_spawn a échoué\n"); return 1; }
    religion_set_country(cid, rid);
    religion_set_region(e, r0, rid);
    religion_set_region(e, r1, rid);
    religion_refresh_region(e, r0);
    religion_refresh_region(e, r1);
    CK(religion_fracture_level(w,e,cid) == 0.f, "les 2 régions professent la foi d'État → fracture_level = 0");
    CK(religion_credo_drift(w,e,cid) == 0.f,    "credo_drift (alias) = 0 aussi");

    /* Fonde une SECONDE foi (schisme réel) et bascule r1 dessus : r1 devient MINORITAIRE
     * (hors foi d'État) — la moitié de la pop (pop-pondérée) est off ⇒ fracture ≈ 0.5. */
    int trad2[3]={RP_SILENCE, RP_MUR, RP_COURONNE};
    int rid2 = religion_schism(rid, /*slot_a*/0, RP_SILENCE, /*slot_b*/1, RP_MUR,
                                CREDO_EVANGELISTE, /*declare_cell*/0, cid, /*randomize_color*/0, 1234u);
    if (rid2 < 0) rid2 = religion_spawn(CREDO_EVANGELISTE, trad2, 1, cid, NULL); /* repli si le schisme échoue */
    if (rid2 >= 0){
        religion_set_region(e, r1, rid2);
        religion_refresh_region(e, r0);
        religion_refresh_region(e, r1);
        float fl = religion_fracture_level(w,e,cid);
        printf("   r1 bascule sur une AUTRE foi → fracture_level=%.2f\n", fl);
        CK(fl > 0.4f && fl < 0.6f, "une région sur deux (pop égale) hors foi d'État → fracture_level ≈ 0.5");
        CK(religion_credo_drift(w,e,cid) == fl, "credo_drift (alias) == fracture_level, même transitoire");
        CK(fl >= 0.f && fl <= 1.f, "fracture_level borné [0..1]");
    }

    /* Recrute un lettré : sa face suit le crédo COURANT (pluraliste→Gourou) → alignée, drift=0. */
    int role = religion_scholar_recruit(cid, r0);
    CK(role == SCHOLAR_RESIST, "crédo pluraliste → face recrutée = Gourou (RESIST)");
    CK(religion_scholar_drift(cid) == 0.f, "face alignée sur le crédo d'État courant → scholar_drift = 0");

    /* Le pays SCHISME (change de foi d'État vers l'évangéliste) : la face du lettré,
     * recrutée sous l'ancien crédo, est désormais PÉRIMÉE → scholar_drift monte à 1. */
    if (rid2 >= 0){
        religion_set_country(cid, rid2);
        CK(religion_scholar_drift(cid) == 1.f,
           "foi d'État change de crédo → face du lettré périmée → scholar_drift = 1");
    }

    printf("\n== BILAN : religion OK (%d réussis, %d échoués) ==\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
