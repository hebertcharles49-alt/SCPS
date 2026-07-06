/*
 * revolt_demo.c — la révolte incarnée : qui se soulève, combien, ce qu'il advient
 *
 *   make revolt_demo && ./revolt_demo [graine]
 *
 * Une révolte n'est plus un fanion : c'est un GROUPE qui prend les armes. On
 * vérifie la chaîne entière :
 *   1. DÉFICIT — un groupe affamé/sur-taxé/étranger/non-intégré a un fort déficit ;
 *               un natif rassasié et loyal en a un faible.
 *   2. MOBILISATION — le déficit lève des combattants ; sans déficit, personne.
 *   3. NATURE — l'élite fait un COUP ; une nation conquise une SÉCESSION ; un
 *               natif mécontent une JACQUERIE.
 *   4. CHOC ÉCONOMIQUE — les mobilisés QUITTENT la main-d'œuvre (l'atelier perd ses bras).
 *   5. ÉCRASEMENT — une grosse garnison écrase les rebelles : morts, pas de pays neuf.
 *   6. SÉCESSION — garnison faible + nation étrangère → un PAYS NAÎT (la carte change).
 *   7. CONCESSION — une jacquerie victorieuse arrache une hausse de satisfaction.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_demography.h"
#include "scps_prosperity.h"
#include "scps_legitimacy.h"
#include "scps_revolt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

/* Une culture-test à axes connus (langue ignorée par la distance). */
static PopCulture cult(float v,float s,float p,float r,Heritage heritage){
    PopCulture c; memset(&c,0,sizeof c);
    c.valeurs=v; c.subsistance=s; c.parente=p; c.religion=r; c.heritage=heritage; c.settled=true;
    return c;
}
static PopGroup grp(Heritage heritage, SocialClass k, long n, float L, float integ, PopCulture cu, int id){
    PopGroup g; memset(&g,0,sizeof g);
    g.heritage=heritage; g.klass=k; g.count=n; g.L=L; g.integration=integ; g.culture=cu; g.origin=cu;
    g.agit_base=0.f; g.drift_id=id; g.origin_sphere=heritage_sphere(heritage);
    return g;
}

/* RE-KEY PROVINCE (charte PROVINCE_MODEL.md) : l'économie vit à la province, la
 * région n'est qu'un agrégat recalculé par econ_tick — scps_revolt.c route ses
 * mutations sur econ_region_rep_province. Le banc doit poser SA fixture et lire
 * SES vérifications au même grain. Repli : scan direct si le cache n'a rien
 * retenu (même idiome que social_demo.c/econ_tax_demo.c). */
static int rep_prov(WorldEconomy *e, int r){
    if (r>=0 && r<SCPS_MAX_REG && e->region_rep_prov[r]>=0) return e->region_rep_prov[r];
    for (int p=0;p<e->n_prov;p++) if (e->prov[p].region==r) return p;
    return -1;
}
/* Fige une région en banc d'essai : propriétaire, crown, satisfaction, garnison. */
static void rig(WorldEconomy *e, int r, int owner, float food, float soc, float coerc, float H){
    int pid=rep_prov(e,r); if (pid<0) return;
    ProvinceEconomy *re=&e->prov[pid];
    re->active=true; re->colonized=true; re->culture.settled=true;
    re->owner=(int16_t)owner;
    re->food_sat=food; re->society_sat=soc; re->coercion=coerc; re->satisfaction=0.5f;
    re->build.H_coerc=H;
    re->pop.n_groups=0;
    for (int k=0;k<CLASS_COUNT;k++){ re->strata[k].pop=0.f; re->strata[k].wealth=100.f; }
}
static void push(WorldEconomy *e, int r, PopGroup g){
    int pid=rep_prov(e,r); if (pid<0) return;
    ProvinceEconomy *re=&e->prov[pid];
    re->strata[g.klass].pop += (float)g.count;
    re->pop.groups[re->pop.n_groups++]=g;
    /* push() est TOUJOURS le dernier geste de fixture avant revolt_ignite/_scan/_tick
     * (qui lisent owner/pop/food_sat/… via l'agrégat region[], le grain public historique
     * de scps_revolt.c) — on rafraîchit region[] depuis prov[] ICI, une fois, PURE (aucun
     * effet de temps : ce n'est PAS un econ_tick). */
    econ_aggregate_regions(e);
}
/* Isole un soulèvement : SEULE la région `r` appartient à `owner` (sinon la
 * puissance militaire de la couronne — somme sur ses régions — fausserait la
 * garnison d'un test à l'autre). owner posé sur TOUTES les provinces (comme
 * econ_region_set_owner, la conquête) pour que l'agrégat region[].owner suive. */
static void solo_owner(WorldEconomy *e, int r, int owner){
    for (int p=0;p<e->n_prov;p++) e->prov[p].owner=-1;
    int pid=rep_prov(e,r); if (pid>=0) e->prov[pid].owner=(int16_t)owner;
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    World *w=malloc(sizeof(World));
    WorldEconomy *e=malloc(sizeof(WorldEconomy));
    WorldLegitimacy *wl=malloc(sizeof(WorldLegitimacy));
    WorldProsperity *wp=malloc(sizeof(WorldProsperity));
    ModifierStack *drift=malloc(sizeof(ModifierStack));
    if(!w||!e||!wl||!wp||!drift){ fprintf(stderr,"OOM\n"); return 1; }
    memset(wl,0,sizeof *wl); memset(wp,0,sizeof *wp); memset(drift,0,sizeof *drift);

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" LA RÉVOLTE INCARNÉE — qui se soulève, combien, ce qu'il advient — graine %u\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p); econ_init(e,w);
    if (e->n_regions<5){ fprintf(stderr,"monde trop petit\n"); return 1; }

    /* table rase : on maîtrise propriétaires et populations (grain PROVINCE : la
     * charte fait vivre owner/pop à la province, region[] n'est qu'un agrégat). */
    for (int p=0;p<e->n_prov;p++){ e->prov[p].owner=-1; e->prov[p].pop.n_groups=0; }
    for (int i=0;i<SCPS_MAX_REG;i++){ wl->L[i]=6.f; wl->years_held[i]=50.f; }

    /* la COURONNE = culture de la région-capitale du pays 5 (humaine, médiane).
     * On laisse de la place dans la table des pays pour qu'une sécession PUISSE
     * faire naître un nouveau pays (le worldgen en remplit parfois jusqu'au plafond). */
    w->n_countries=8;
    PopCulture crown=cult(5,5,5,5,HERITAGE_ADAPTATIF);
    int OWNER=5;
    w->country[OWNER].capital_prov=w->region[0].province_ids[0];
    w->country[OWNER].role=POLITY_ANTAGONIST;
    w->country[OWNER].n_regions=1; w->country[OWNER].region_ids[0]=0;
    { int p0=rep_prov(e,0); if (p0>=0) e->prov[p0].culture=crown; }   /* crown_of(5) lira ceci */
    int n_countries_0=w->n_countries;

    /* ═══ 1. DÉFICIT — le grief est ANCRÉ ═══════════════════════════════ */
    printf("\n── 1. Le déficit : la misère ANCRÉE sur un groupe ──\n");
    PopCulture foreign=cult(9,9,2,9,HERITAGE_CLANIQUE);   /* loin de la couronne (D≈4) */
    PopGroup misery = grp(HERITAGE_CLANIQUE, CLASS_LABORER, 8000, 2.f, 0.15f, foreign, 101);
    PopGroup content= grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 8000, 7.f, 1.00f, crown, 102);
    float d_mis = revolt_group_deficit(&misery,  drift, &crown, 0.05f, 0.20f, 0.4f, 0.4f);
    float d_con = revolt_group_deficit(&content, drift, &crown, 0.95f, 0.90f, 0.0f, 0.0f);
    printf("   déficit : affamé/étranger=%.2f  vs  rassasié/natif=%.2f\n", d_mis, d_con);
    ok("le groupe affamé/étranger/non-intégré a un FORT déficit", d_mis>0.55f);
    ok("le groupe rassasié, natif et loyal a un FAIBLE déficit",  d_con<0.15f);

    /* ═══ 2. MOBILISATION — le déficit lève des bras ════════════════════ */
    printf("\n── 2. La mobilisation : le déficit prend les armes ──\n");
    long m_mis=revolt_mobilized(&misery, d_mis);
    long m_con=revolt_mobilized(&content, d_con);
    printf("   mobilisés : misère=%ld  vs  contentement=%ld (sur 8000)\n", m_mis, m_con);
    ok("un fort déficit MOBILISE des combattants", m_mis>500);
    ok("un faible déficit ne mobilise personne",   m_con==0);
    ok("la mobilisation est PLAFONNÉE (jamais >40 %)", m_mis<=3200);

    /* ═══ 3. NATURE — qui se lève → ce qu'il veut ═══════════════════════ */
    printf("\n── 3. La nature : qui se lève détermine ce qu'il veut ──\n");
    PopGroup noble = grp(HERITAGE_ADAPTATIF, CLASS_ELITE, 400, 3.f, 1.0f, crown, 103);
    ok("l'ÉLITE vise le trône (coup d'État)",        revolt_classify(&noble, drift,&crown)==REBEL_COUP);
    ok("la nation étrangère mal liée fait SÉCESSION", revolt_classify(&misery,drift,&crown)==REBEL_SECESSION);
    ok("le natif mécontent fait une JACQUERIE",       revolt_classify(&content,drift,&crown)==REBEL_CLASS);

    /* ═══ 4. CHOC ÉCONOMIQUE — les bras quittent l'atelier ══════════════ */
    printf("\n── 4. Le choc économique : les mobilisés quittent la main-d'œuvre ──\n");
    RevoltState rs; revolt_init(&rs);
    rig(e, 1, OWNER, 0.05f, 0.20f, 0.4f, 0.f);
    push(e, 1, grp(HERITAGE_CLANIQUE, CLASS_LABORER, 8000, 2.f, 0.15f, foreign, 201));
    int p1=rep_prov(e,1);
    float labor_before=e->prov[p1].strata[CLASS_LABORER].pop;
    int idx=revolt_ignite(&rs, w, e, drift, NULL, NULL, 1, 0.4f);
    float labor_after=e->prov[p1].strata[CLASS_LABORER].pop;
    printf("   main-d'œuvre : %.0f → %.0f (partis au combat : %ld)\n",
           labor_before, labor_after, idx>=0?rs.list[idx].mobilized:0);
    ok("le soulèvement s'ALLUME sur le groupe au pire déficit", idx>=0);
    ok("les mobilisés QUITTENT la main-d'œuvre (choc productif)", labor_after < labor_before-100.f);

    /* ═══ 5. ÉCRASEMENT — la grosse garnison gagne ══════════════════════ */
    printf("\n── 5. L'écrasement : une forte garnison brise la jacquerie ──\n");
    revolt_init(&rs);
    solo_owner(e, 2, OWNER);
    rig(e, 2, OWNER, 0.05f, 0.20f, 0.3f, 20.f);   /* H=20 → garnison écrasante */
    push(e, 2, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 4000, 2.f, 1.0f, crown, 202)); /* natif → jacquerie */
    int before_countries=w->n_countries;
    long pop_lost_before=rs.pop_lost;
    int ix=revolt_ignite(&rs, w, e, drift, NULL, NULL, 2, 0.4f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    printf("   verdict : %s | morts=%ld | pays avant/après=%d/%d\n",
           ix>=0?revolt_outcome_word(rs.list[ix].outcome):"(rien)",
           rs.pop_lost-pop_lost_before, before_countries, w->n_countries);
    ok("la jacquerie est ÉCRASÉE par la garnison", ix>=0 && rs.list[ix].outcome==OUT_CRUSHED);
    ok("l'échec se paie en MORTS",                 rs.pop_lost>pop_lost_before);
    ok("aucun pays ne naît d'une révolte écrasée",  w->n_countries==before_countries);

    /* ═══ 6. SÉCESSION — un pays naît ═══════════════════════════════════ */
    printf("\n── 6. La sécession : garnison faible + nation étrangère → un PAYS NAÎT ──\n");
    revolt_init(&rs);
    solo_owner(e, 3, OWNER);
    rig(e, 3, OWNER, 0.02f, 0.10f, 0.5f, 0.f);    /* H=0 → garnison faible */
    push(e, 3, grp(HERITAGE_CLANIQUE, CLASS_LABORER, 9000, 2.f, 0.12f, foreign, 203));
    int sec_before=w->n_countries;
    int iy=revolt_ignite(&rs, w, e, drift, NULL, NULL, 3, 0.5f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    int born = (iy>=0)?rs.list[iy].spawned:-1;
    int p3=rep_prov(e,3);
    printf("   verdict : %s | pays né n°%d (table %d→%d) | propriétaire région 3 : %d→%d\n",
           iy>=0?revolt_outcome_word(rs.list[iy].outcome):"(rien)",
           born, sec_before, w->n_countries, OWNER, e->prov[p3].owner);
    ok("la nation étrangère fait SÉCESSION",        iy>=0 && rs.list[iy].outcome==OUT_SECEDED);
    ok("un PAYS NAÎT (slot neuf ou vacant réutilisé)", born>=0 && born!=OWNER);
    ok("la région passe à la NOUVELLE couronne",     e->prov[p3].owner==born && born>=0);

    /* ═══ 7. CONCESSION — la couronne cède ══════════════════════════════ */
    printf("\n── 7. La concession : une jacquerie victorieuse arrache un mieux ──\n");
    revolt_init(&rs);
    solo_owner(e, 4, OWNER);
    rig(e, 4, OWNER, 0.0f, 0.0f, 1.0f, 0.f);      /* misère totale, garnison faible */
    push(e, 4, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 9000, 2.f, 1.0f, crown, 204)); /* natif désespéré */
    int p4=rep_prov(e,4);
    float sat_before=e->prov[p4].satisfaction;
    int iz=revolt_ignite(&rs, w, e, drift, NULL, NULL, 4, 1.0f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    float sat_after=e->prov[p4].satisfaction;
    printf("   verdict : %s | satisfaction %.2f→%.2f\n",
           iz>=0?revolt_outcome_word(rs.list[iz].outcome):"(rien)", sat_before, sat_after);
    ok("la jacquerie victorieuse arrache une CONCESSION", iz>=0 && rs.list[iz].outcome==OUT_CONCESSION);
    ok("la satisfaction MONTE (la couronne a cédé)",       sat_after>sat_before);

    /* ═══ 8. SCAN — la misère SOUTENUE finit par lever la région ════════ */
    printf("\n── 8. Le scan : la misère soutenue (mois après mois) finit par soulever ──\n");
    revolt_init(&rs);
    solo_owner(e, 1, OWNER);
    rig(e, 1, OWNER, 0.02f, 0.05f, 0.5f, 0.f);
    push(e, 1, grp(HERITAGE_CLANIQUE, CLASS_LABORER, 9000, 2.f, 0.12f, foreign, 211));
    int months=0;
    for (; months<6 && revolt_active_count(&rs)==0; months++) revolt_scan(&rs, w, e, drift, NULL, NULL, NULL, 30);
    printf("   région désespérée : soulèvement au bout de %d mois de misère soutenue\n", months);
    ok("une région désespérée finit par se SOULEVER (misère soutenue)", revolt_active_count(&rs)>0);
    /* une région contente, scannée autant, ne se lève JAMAIS */
    revolt_init(&rs);
    solo_owner(e, 2, OWNER);
    rig(e, 2, OWNER, 0.95f, 0.90f, 0.0f, 0.f);
    push(e, 2, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 9000, 7.f, 1.0f, crown, 212));
    for (int mo=0; mo<6; mo++) revolt_scan(&rs, w, e, drift, NULL, NULL, NULL, 30);
    ok("une région contente ne se soulève jamais (aucun grief)", revolt_active_count(&rs)==0);

    /* ═══ 9. REVANCHISME — subir la conquête arme le séparatisme ════════ */
    printf("\n── 9. Le revanchisme : la province fraîchement conquise se libère « quoi qu'il arrive » ──\n");
    /* Garnison FORTE (citadelle H=10) : sans revanchisme, la sécession est écrasée… */
    revolt_init(&rs);
    solo_owner(e, 1, OWNER);
    rig(e, 1, OWNER, 0.4f, 0.4f, 0.3f, 10.f);
    push(e, 1, grp(HERITAGE_CLANIQUE, CLASS_LABORER, 6000, 2.f, 0.12f, foreign, 221));
    int j1=revolt_ignite(&rs, w, e, drift, NULL, NULL, 1, 0.3f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    int out_plain = (j1>=0)?rs.list[j1].outcome:-1;
    /* …MÊME garnison, mais la province vient d'être conquise (revanchisme actif) → libre. */
    revolt_init(&rs);
    solo_owner(e, 2, OWNER);
    rig(e, 2, OWNER, 0.4f, 0.4f, 0.3f, 10.f);
    push(e, 2, grp(HERITAGE_CLANIQUE, CLASS_LABORER, 6000, 2.f, 0.12f, foreign, 222));
    revolt_on_conquest(&rs, 2);                /* on vient de la soumettre */
    int j2=revolt_ignite(&rs, w, e, drift, NULL, NULL, 2, 0.3f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    int out_rev = (j2>=0)?rs.list[j2].outcome:-1;
    printf("   même citadelle (H=10) : sans revanchisme → %s ; juste conquise → %s\n",
           revolt_outcome_word(out_plain), revolt_outcome_word(out_rev));
    ok("sans revanchisme, la citadelle ÉCRASE la sécession", out_plain==OUT_CRUSHED);
    ok("le REVANCHISME post-conquête donne l'INDÉPENDANCE malgré la citadelle", out_rev==OUT_SECEDED);

    /* ═══ 10. CICATRICE — une révolte résolue stunt la province ════════ */
    printf("\n── 10. La cicatrice : une révolte résolue laisse une plaie (−50 %% dévelop.) ──\n");
    revolt_init(&rs);
    solo_owner(e, 4, OWNER);
    rig(e, 4, OWNER, 0.05f, 0.20f, 0.3f, 20.f);   /* H haut → écrasement */
    push(e, 4, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 4000, 2.f, 1.0f, crown, 241));
    { int p4b=rep_prov(e,4); if (p4b>=0) e->prov[p4b].revolt_scar=0.f; }
    revolt_ignite(&rs, w, e, drift, NULL, NULL, 4, 0.4f);
    revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
    int p4c=rep_prov(e,4);
    printf("   cicatrice après écrasement : %.2f\n", e->prov[p4c].revolt_scar);
    ok("une révolte résolue LAISSE une cicatrice de développement", e->prov[p4c].revolt_scar > 0.4f);

    /* ═══ 11. LOT H — LA RÉVOLTE SERVILE STRUCTURELLE (le contrepoids du mécanisme H) ══ */
    printf("\n── 11. Servilité : sous le seuil rien, au-dessus la pression MONTE avec la part ──\n");
    {
        /* région CONTENTE (aucun autre grief — TOUS les groupes loyaux/intégrés/de la
         * couronne, y compris la strate servile elle-même : on isole le terme
         * STRUCTUREL de part, pas le déficit ordinaire du groupe esclave lui-même). */
        revolt_init(&rs);
        solo_owner(e, 1, OWNER);
        rig(e, 1, OWNER, 0.95f, 0.90f, 0.0f, 0.f);
        push(e, 1, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 8000, 7.f, 1.0f, crown, 251));
        /* SOUS le seuil (0.20) : 1000/9000 ≈ 11 % d'esclaves — aucune pression ajoutée. */
        PopGroup low_slv = grp(HERITAGE_ADAPTATIF, CLASS_SLAVE, 1000, 7.f, 1.0f, crown, 252);
        push(e, 1, low_slv);
        int p1=rep_prov(e,1);
        e->prov[p1].strata[CLASS_SLAVE].pop=1000.f;
        for (int mo=0; mo<12; mo++) revolt_scan(&rs, w, e, drift, NULL, NULL, NULL, 30);
        ok("sous SLAVE_REVOLT_SHARE, une région autrement contente ne se soulève PAS",
           revolt_active_count(&rs)==0);

        /* AU-DESSUS du seuil : 6500/10000 = 65 % d'esclaves (Rome tient 30%, pas 60% —
         * même contentement par ailleurs, MÊME groupe loyal/intégré/couronne) — la
         * pression structurelle finit par soulever la région SEULE (le terme de PART,
         * pas le déficit ordinaire du groupe). */
        revolt_init(&rs);
        solo_owner(e, 2, OWNER);
        rig(e, 2, OWNER, 0.95f, 0.90f, 0.0f, 0.f);
        push(e, 2, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 3500, 7.f, 1.0f, crown, 253));
        PopGroup high_slv = grp(HERITAGE_ADAPTATIF, CLASS_SLAVE, 6500, 7.f, 1.0f, crown, 254);
        push(e, 2, high_slv);
        int p2=rep_prov(e,2);
        e->prov[p2].strata[CLASS_SLAVE].pop=6500.f;
        int mo2=0;
        for (; mo2<24 && revolt_active_count(&rs)==0; mo2++) revolt_scan(&rs, w, e, drift, NULL, NULL, NULL, 30);
        printf("   65%% d'esclaves (groupe autrement loyal/intégré) : soulèvement au bout de %d mois\n", mo2);
        ok("au-dessus de SLAVE_REVOLT_SHARE, la seule PART SERVILE finit par soulever",
           revolt_active_count(&rs)>0);
    }

    /* ═══ 12. LOT H — RÉVOLTE SERVILE VICTORIEUSE : affranchit DE FORCE ═══════════ */
    printf("\n── 12. Révolte servile victorieuse : les esclaves de la région s'affranchissent DE FORCE ──\n");
    {
        revolt_init(&rs);
        solo_owner(e, 3, OWNER);
        rig(e, 3, OWNER, 0.02f, 0.05f, 0.5f, 0.f);   /* H=0 → garnison faible : victoire rebelle */
        push(e, 3, grp(HERITAGE_ADAPTATIF, CLASS_LABORER, 1000, 6.f, 1.0f, crown, 260));
        PopGroup slave_grp = grp(HERITAGE_CLANIQUE, CLASS_SLAVE, 9000, 2.f, 0.05f, foreign, 261);
        slave_grp.diaspora=true; slave_grp.arrival=ARR_DEPORTE;
        push(e, 3, slave_grp);   /* push() rafraîchit l'agrégat region[] (revolt_ignite le lit) */
        int iw=revolt_ignite(&rs, w, e, drift, NULL, NULL, 3, 0.5f);
        ok("le soulèvement s'ALLUME sur le groupe SERVILE", iw>=0 && rs.list[iw].klass==CLASS_SLAVE);
        revolt_tick(&rs, w, e, drift, wl, wp, NULL, NULL, NULL, 120);
        int p3b=rep_prov(e,3);
        bool any_slave_left=false, any_free=false;
        for (int i=0;i<e->prov[p3b].pop.n_groups;i++){
            if (e->prov[p3b].pop.groups[i].klass==CLASS_SLAVE) any_slave_left=true;
            if (e->prov[p3b].pop.groups[i].klass!=CLASS_SLAVE && e->prov[p3b].pop.groups[i].heritage==HERITAGE_CLANIQUE)
                any_free=true;
        }
        printf("   verdict : %s | un groupe encore ESCLAVE : %s | un groupe clanique LIBRE : %s\n",
               iw>=0?revolt_outcome_word(rs.list[iw].outcome):"(rien)", any_slave_left?"oui":"non", any_free?"oui":"non");
        ok("une révolte SERVILE victorieuse affranchit TOUS les groupes esclaves de la région (klass→LABORER)",
           !any_slave_left && any_free);
    }

    (void)n_countries_0;
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(w); free(e); free(wl); free(wp); free(drift);
    return g_fail?1:0;
}
