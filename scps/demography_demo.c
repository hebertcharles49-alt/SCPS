/*
 * demography_demo.c — la clé de voûte : la province contient des groupes
 *
 *   make demography_demo && ./demography_demo [graine]
 *
 * Prouve les huit points du cahier :
 *   1. D réel par province (mixte → D∞ haut ; homogène → 0).
 *   2. H jouable : réprimer baisse l'agitation MAIS ronge la L et monte la fragilité.
 *   3. Réversibilité (Kuran) : H tombe + L basse → le groupe revient frondeur.
 *   4. Assimilation vraie : dérive DURABLE, timer ∝ D∞ (Agraire ~20 ans, Clanique ~80).
 *   5. Migration : un groupe afflue, devient diaspora (garde sa heritage/culture), crée du D.
 *   6. Agrégation : country_D∞ alimente scps_order ; conquérir du lointain → fracture↑.
 *   7. UI : composition (heritage/culture/classe/loyauté/état) sans un nom SCPS.
 *   8. Non-régression : une province mono-groupe = les nombres d'aujourd'hui.
 */
#include "scps_demography.h"
#include "scps_core.h"   /* le verdict vérifié (banc d'essai seulement) */
#include "scps_econ.h"   /* ESCLAVAGE : ProvinceEconomy/WorldEconomy (demography_manumit_country) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static float absf(float v){ return v<0?-v:v; }
static float cdist(const PopCulture*a,const PopCulture*b){
    float dv=absf(a->valeurs-b->valeurs),ds=absf(a->subsistance-b->subsistance);
    float dp=absf(a->parente-b->parente),dr=absf(a->religion-b->religion);
    float m=dv;if(ds>m)m=ds;if(dp>m)m=dp;if(dr>m)m=dr;return m;
}
static float agitL(float L){ float a=(6.f-L)*15.f; return a<0?0:(a>100?100:a); }

static PopCulture cult(float v,float s,float p,float r,Ethos e){
    PopCulture c; memset(&c,0,sizeof c);
    c.langue=5; c.valeurs=v; c.subsistance=s; c.parente=p; c.religion=r;
    c.ethos=e; c.lifeway=LIFE_FARMER; c.structure=STRUCT_LIGNAGER; c.credo=CREDO_PLURALISTE;
    c.rel_branch=REL_ABRAHAMIQUE; c.martial=MART_MUR_BOUCLIERS; c.settled=true; return c;
}
static int g_id=1;
static PopGroup grp(Heritage heritage, Sphere sph, PopCulture o, SocialClass k,
                    long count, float L, float integ, bool diaspora){
    PopGroup g; memset(&g,0,sizeof g);
    g.heritage=heritage; g.origin_sphere=sph; g.origin=o; g.culture=o; g.klass=k; g.count=count;
    g.L=L; g.agit_base=agitL(L); g.integration=integ; g.diaspora=diaspora; g.drift_id=g_id++;
    g.home_reg=-1;   /* défaut : pas de foyer ailleurs (le test le surcharge si besoin) */
    return g;
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u; (void)seed;
    ModifierStack *drift=malloc(sizeof(ModifierStack)); memset(drift,0,sizeof(*drift));

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" DÉMOGRAPHIE — la province contient des GROUPES (clé de voûte)\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* La couronne (culture régnante du pays). */
    PopCulture crown = cult(2,6,3,2, ETHOS_ORDRE);
    PopCulture humc  = crown;                                   /* natifs = couronne */
    PopCulture halfc = cult(4,6,5,4, ETHOS_BUREAUCRATE);        /* proche : D∞≈2 */
    PopCulture orcc  = cult(9,0,9,10, ETHOS_DOMINATEUR);        /* gouffre : D∞≈8 */

    /* ═══ 1. D RÉEL PAR PROVINCE ════════════════════════════════════════ */
    printf("\n── 1. D interne PAR province (entre groupes) ──\n");
    ProvincePop mixed; memset(&mixed,0,sizeof mixed);
    mixed.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,humc,CLASS_LABORER,700,8.0f,1.0f,false);
    mixed.groups[1]=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS,orcc,CLASS_LABORER,300,1.5f,0.2f,false);
    mixed.n_groups=2;
    ProvincePop homo; memset(&homo,0,sizeof homo);
    homo.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,humc,CLASS_LABORER,1000,8.0f,1.0f,false);
    homo.n_groups=1;
    float dmix=province_Dinf(&mixed,drift), dhom=province_Dinf(&homo,drift);
    printf("   province 70%% Humains / 30%% Claniques : D∞=%.1f | province homogène : D∞=%.1f\n", dmix, dhom);
    ok("une province conquise mixte a un D∞ interne VRAI (élevé)", dmix>6.f);
    ok("une province homogène a un D∞ interne nul", dhom==0.f);

    /* ═══ 8. NON-RÉGRESSION (mono-groupe = aujourd'hui) ════════════════ */
    printf("\n── 8. Non-régression : une province mono-groupe = les nombres d'hier ──\n");
    ok("mono-groupe : D∞=0, L = la L du groupe, pop = l'effectif",
       province_Dinf(&homo,drift)==0.f && province_L(&homo)==8.0f
       && province_total_pop(&homo)==1000);

    /* ═══ 2. H JOUABLE — il SUPPRIME (et ronge L, monte la fragilité) ══ */
    printf("\n── 2. H jouable : on réprime la minorité restive d'UNE province ──\n");
    float agit0=province_agitation(&mixed,drift), orcL0=mixed.groups[1].L;
    CoercionEffect ce=province_apply_coercion(&mixed,drift,8.0f);
    float agit1=province_agitation(&mixed,drift), orcL1=mixed.groups[1].L;
    printf("   agitation province %.0f→%.0f | L du groupe clanique %.2f→%.2f | fragilité +%.1f\n",
           agit0,agit1, orcL0,orcL1, ce.fragility_rise);
    ok("la coercition BAISSE l'agitation immédiatement (réprime)", agit1 < agit0 - 5.f);
    ok("… mais RONGE la L du groupe coercé (on règne par la force)", orcL1 < orcL0);
    ok("… et MONTE la fragilité (l'ordre tenu par la botte)", ce.fragility_rise > 0.f);

    /* ═══ 3. RÉVERSIBILITÉ (Kuran) — la botte se lève ═════════════════ */
    printf("\n── 3. Réversibilité : H tombe + L basse → le groupe revient frondeur ──\n");
    float orc_supp=group_agitation_effective(&mixed.groups[1],drift);  /* agitation clanique réprimée */
    province_lift_coercion(&mixed,drift);                              /* la botte se lève */
    float orc_ret=group_agitation_effective(&mixed.groups[1],drift);   /* … elle resurgit */
    printf("   agitation du groupe clanique : réprimée=%.0f → la botte se lève → %.0f (préférence falsifiée)\n",
           orc_supp, orc_ret);
    ok("la suppression n'a RIEN métabolisé : l'agitation revient (Kuran)",
       orc_ret > orc_supp + 30.f && mixed.groups[1].L < 4.f);

    /* ═══ 4. ASSIMILATION VRAIE — dérive durable, timer ∝ D∞ ═══════════ */
    printf("\n── 4. Assimilation : dérive DURABLE vers la dominante, timer ∝ D∞ ──\n");
    printf("   estimation : Agraire (D∞≈2) ~%.0f ans ; Clanique (D∞≈8) ~%.0f ans\n",
           assimilation_years(2.f,5.f,5.f), assimilation_years(8.f,5.f,5.f));
    ok("le gouffre allonge le timer (Agraire ≪ Clanique)",
       assimilation_years(2.f,5.f,5.f) < assimilation_years(8.f,5.f,5.f)*0.5f);
    ProvincePop town; memset(&town,0,sizeof town);
    town.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES,   humc, CLASS_LABORER,1000,8.f,1.f,false);
    town.groups[1]=grp(HERITAGE_AGRAIRE,SPHERE_HOMMES,  halfc,CLASS_BOURGEOIS, 200,5.f,0.5f,false);
    town.groups[2]=grp(HERITAGE_CLANIQUE,   SPHERE_ETRANGERS,orcc, CLASS_LABORER, 200,2.f,0.2f,false);
    town.n_groups=3;
    int half_id=town.groups[1].drift_id, orc_id=town.groups[2].drift_id;
    float half_d0=cdist(&town.groups[1].origin,&crown), orc_d0=cdist(&town.groups[2].origin,&crown);
    for (int yr=0; yr<40; yr++) assimilation_tick(&town, drift, 5.f, 5.f, 1.f);
    /* le Agraire a-t-il fusionné (ou quasi) ? l'Clanique traîne-t-il encore ? */
    bool half_gone=true; float half_d=0, orc_d=0;
    for (int i=0;i<town.n_groups;i++){
        PopCulture eff=group_culture_effective(&town.groups[i],drift);
        if (town.groups[i].drift_id==half_id){ half_gone=false; half_d=cdist(&eff,&crown); }
        if (town.groups[i].drift_id==orc_id){ orc_d=cdist(&eff,&crown); }
    }
    printf("   après 40 ans : Agraire %s (départ D∞=%.0f) | Clanique D∞ %.0f→%.1f (gouffre, lent)\n",
           half_gone?"ASSIMILÉ (fusionné)":"en cours", half_d0, orc_d0, orc_d);
    ok("le proche s'assimile (fusion) bien avant le lointain (timer ∝ D∞)",
       (half_gone || half_d<1.f) && orc_d>4.f);
    /* Durabilité : la dérive métabolisée ne saute PAS quand la coercition se lève. */
    PopCulture orc_eff_before=group_culture_effective(&town.groups[town.n_groups-1],drift);
    province_apply_coercion(&town,drift,5.f); province_lift_coercion(&town,drift);
    PopCulture orc_eff_after=group_culture_effective(&town.groups[town.n_groups-1],drift);
    ok("l'assimilation est DURABLE (la dérive métabolisée ne saute pas à la levée de H)",
       cdist(&orc_eff_before,&orc_eff_after) < 0.01f);

    /* ═══ 4b. CONVERSION RELIGIEUSE — la FOI converge vers le TRÔNE ════ */
    printf("\n── 4b. Conversion : la branche sacrée bascule vers la couronne (si prosélyte) ──\n");
    /* Une minorité HÉRÉTIQUE : culture quasi identique au trône, mais d'une
     * AUTRE branche sacrée (Dharmique vs Abrahamique) → un mur de fracture. */
    PopCulture heretic = cult(2,6,3,5, ETHOS_ORDRE);    /* v/s/p = couronne ; foi=5 vs 2 */
    heretic.rel_branch = REL_DHARMIQUE;                 /* l'autre branche : le vrai clivage */
    /* (a) TOLÉRANCE : un trône pluraliste ne convertit personne. */
    PopCulture crown_plu = crown; crown_plu.credo = CREDO_PLURALISTE; crown_plu.religion = 2.f;
    ProvincePop plu; memset(&plu,0,sizeof plu);
    plu.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,crown_plu,CLASS_LABORER,800,8.f,1.f,false);
    plu.groups[1]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,heretic,  CLASS_LABORER,200,6.f,0.9f,false);
    plu.n_groups=2;
    for (int yr=0; yr<200; yr++) faith_convert_tick(&plu, &crown_plu, 300.f, 1.f);
    ok("un trône PLURALISTE ne convertit personne (tolérance : empire multi-confessionnel)",
       plu.groups[1].origin.rel_branch == REL_DHARMIQUE);
    /* (b) PURIFICATION : la branche hérétique BASCULE vers la couronne, puis fusionne. */
    PopCulture crown_pur = crown; crown_pur.credo = CREDO_PURIFICATEUR; crown_pur.religion = 2.f;
    ProvincePop pur; memset(&pur,0,sizeof pur);
    pur.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,crown_pur,CLASS_LABORER,800,8.f,1.f,false);
    pur.groups[1]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,heretic,  CLASS_LABORER,200,6.f,0.9f,false);
    pur.n_groups=2;
    int her_id = pur.groups[1].drift_id;
    bool flipped=false;
    for (int yr=0; yr<120 && !flipped; yr++){
        faith_convert_tick(&pur, &crown_pur, 300.f, 1.f);
        for (int i=0;i<pur.n_groups;i++)
            if (pur.groups[i].drift_id==her_id && pur.groups[i].origin.rel_branch==REL_ABRAHAMIQUE) flipped=true;
    }
    ok("un trône PURIFICATEUR fait BASCULER la branche hérétique vers la sienne", flipped);
    /* … et l'axe ayant convergé, la conversion ACHÈVE l'assimilation : fusion. */
    bool fused_after_conv=false;
    for (int yr=0; yr<120; yr++){
        faith_convert_tick(&pur, &crown_pur, 300.f, 1.f);
        assimilation_tick(&pur, drift, 5.f, 5.f, 1.f);
    }
    fused_after_conv = (pur.n_groups==1);  /* l'hérétique converti a fondu dans la dominante */
    ok("la conversion fait tomber le mur de branche → l'assimilation peut ACHEVER (fusion)",
       fused_after_conv);
    /* (c) GRADIENT de prosélytisme : à 30 ans de règne, le purificateur a déjà
     *     converti ; l'évangéliste, plus lent (deux générations), pas encore. */
    PopCulture crown_eva = crown; crown_eva.credo = CREDO_EVANGELISTE; crown_eva.religion = 2.f;
    ProvincePop eva; memset(&eva,0,sizeof eva);
    eva.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,crown_eva,CLASS_LABORER,800,8.f,1.f,false);
    eva.groups[1]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,heretic,  CLASS_LABORER,200,6.f,0.9f,false);
    eva.n_groups=2;
    ProvincePop pur30; memset(&pur30,0,sizeof pur30);
    pur30.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,crown_pur,CLASS_LABORER,800,8.f,1.f,false);
    pur30.groups[1]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,heretic, CLASS_LABORER,200,6.f,0.9f,false);
    pur30.n_groups=2;
    for (int yr=0; yr<30; yr++){
        faith_convert_tick(&eva,   &crown_eva, 30.f, 1.f);
        faith_convert_tick(&pur30, &crown_pur, 30.f, 1.f);
    }
    ok("le PURIFICATEUR convertit en une génération ; l'ÉVANGÉLISTE traîne (deux)",
       pur30.groups[1].origin.rel_branch==REL_ABRAHAMIQUE &&
       eva.groups[1].origin.rel_branch==REL_DHARMIQUE);

    /* ═══ 5. MIGRATION — emporte heritage + culture, crée du D ════════════ */
    printf("\n── 5. Migration : un groupe afflue vers la prospérité, devient diaspora ──\n");
    ProvincePop poor; memset(&poor,0,sizeof poor); poor.prosperity=3.f;
    poor.groups[0]=grp(HERITAGE_CLANIQUE,SPHERE_ETRANGERS,orcc,CLASS_LABORER,500,4.f,0.5f,false);
    poor.n_groups=1;
    ProvincePop rich; memset(&rich,0,sizeof rich); rich.prosperity=8.f;
    rich.groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,humc,CLASS_LABORER,800,8.f,1.f,false);
    rich.n_groups=1;
    float rich_d_before=province_Dinf(&rich,drift);
    bool moved = (rich.prosperity>poor.prosperity) && migration_move(&poor,&rich,0,200, g_id++, ARR_MIGRANT, 0 /*home_reg*/);
    float rich_d_after=province_Dinf(&rich,drift);
    bool diaspora = (rich.n_groups==2 && rich.groups[1].diaspora && rich.groups[1].heritage==HERITAGE_CLANIQUE);
    printf("   les Claniques affluent (prospérité 3→8) : province d'accueil D∞ %.0f→%.1f, diaspora=%d\n",
           rich_d_before, rich_d_after, diaspora);
    ok("le groupe migre vers la province plus prospère", moved);
    ok("il y devient minorité/diaspora en GARDANT sa heritage/culture → crée du D interne",
       diaspora && rich_d_after>4.f && rich_d_before==0.f);

    /* ═══ 6. AGRÉGATION PAYS → scps_order (verdict inchangé) ══════════ */
    printf("\n── 6. Le pays agrège ; conquérir du lointain monte D∞ → fracture ──\n");
    ProvincePop country[2]; memset(country,0,sizeof country);
    country[0]=homo;                                            /* province humaine homogène */
    country[1].groups[0]=grp(HERITAGE_ADAPTATIF,SPHERE_HOMMES,humc,CLASS_LABORER,600,8.f,1.f,false);
    country[1].n_groups=1;
    float cd0=country_Dbar(country,2,drift), cL0=country_L(country,2);
    ScpsState s0={0}; s0.D_bar=cd0; s0.C=5; s0.P=5; s0.K=5; s0.H=2; s0.F=5; s0.I=4; s0.L=cL0;
    float frac0=scps_order(&s0).fracture;
    /* CONQUÊTE : une province clanique lointaine entre dans le pays. */
    country[1].groups[1]=grp(HERITAGE_CLANIQUE,SPHERE_ETRANGERS,orcc,CLASS_LABORER,400,1.5f,0.1f,false);
    country[1].n_groups=2;
    float cd1=country_Dbar(country,2,drift), cdinf=country_Dinf(country,2,drift), cL1=country_L(country,2);
    ScpsState s1={0}; s1.D_bar=cd1; s1.C=5; s1.P=5; s1.K=5; s1.H=2; s1.F=5; s1.I=4; s1.L=cL1;
    float frac1=scps_order(&s1).fracture;
    printf("   pays : D̄ %.1f→%.1f (D∞ maillon faible %.1f), L %.1f→%.1f → fracture %.2f→%.2f\n",
           cd0,cd1, cdinf, cL0,cL1, frac0,frac1);
    ok("conquérir du lointain monte le D du pays ET fait chuter sa L", cd1>cd0 && cL1<cL0);
    ok("le verdict VÉRIFIÉ (scps_order, inchangé) en tire plus de fracture", frac1>frac0);

    /* ═══ 7. UI — la composition par la membrane (mots, pas de SCPS) ══ */
    printf("\n── 7. Le panneau de province : la COMPOSITION en mots ──\n");
    GroupReadout comp[DEMO_MAX_GROUPS];
    int nc=province_composition(&town,drift,&crown,5.f,5.f,comp,DEMO_MAX_GROUPS);
    int sumpct=0;
    for (int i=0;i<nc;i++){
        printf("   %3d%%  %-9s · %-11s · %-10s · %s\n",
               comp[i].percent, comp[i].heritage, comp[i].klass, label_humeur(comp[i].loyaute), comp[i].etat);
        sumpct+=comp[i].percent;
    }
    ok("la composition montre heritage / classe / loyauté(MOT) / état, sans un nom SCPS",
       nc>=2 && sumpct>=95 && sumpct<=100
       && comp[0].heritage && comp[0].klass && comp[0].etat);

    /* ═══ 8. BRASSAGE — le mode d'arrivée nomme l'état (soumis / déporté) ══ */
    printf("\n── 8. Mode d'arrivée : soumis (conquis) vs déporté (esclave) ──\n");
    {
        ProvincePop mp; memset(&mp,0,sizeof mp);
        mp.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES, humc, CLASS_BOURGEOIS, 6000,6.f,1.f,false); /* natifs */
        PopGroup soum=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_LABORER, 2000,3.f,0.30f,true);
        soum.arrival=ARR_SOUMIS;  mp.groups[1]=soum;      /* conquis allophone, 30 % digéré */
        PopGroup depo=grp(HERITAGE_ESOTERIQUE, SPHERE_ETRANGERS, cult(9,1,9,8,ETHOS_DOMINATEUR), CLASS_LABORER, 800,2.f,0.05f,true);
        depo.arrival=ARR_DEPORTE; mp.groups[2]=depo;      /* captif, 5 % digéré */
        mp.n_groups=3;
        GroupReadout c8[DEMO_MAX_GROUPS];
        int n8=province_composition(&mp,drift,&crown,5.f,5.f,c8,DEMO_MAX_GROUPS);
        const char *e_soum=NULL, *e_depo=NULL;
        for (int i=0;i<n8;i++){
            printf("   %3d%%  %-11s · %s\n", c8[i].percent, c8[i].heritage, c8[i].etat);
            if (c8[i].etat && strstr(c8[i].etat,"soumis"))  e_soum=c8[i].etat;
            if (c8[i].etat && strstr(c8[i].etat,"déporté")) e_depo=c8[i].etat;
        }
        ok("le conquis allophone est nommé « soumis » (voie coercitive ≠ diaspora libre)", e_soum!=NULL);
        ok("le captif est nommé « déporté » (l'esclave, diffusion faible)", e_depo!=NULL);
        ok("l'état SURFACE l'intégration en % (la métabolisation, nombre tangible)",
           e_soum && strstr(e_soum,"%")!=NULL);
    }

    /* ═══ 9. BRASSAGE — le RÉFUGIÉ + le FOYER (home_reg : aucune migration définitive) ══ */
    printf("\n── 9. Réfugié : membrane + FOYER préservé (la pop RESPIRE) ──\n");
    {
        /* membrane : un groupe ARR_REFUGIE se lit « réfugié · N% intégré » */
        ProvincePop rp2; memset(&rp2,0,sizeof rp2);
        rp2.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES, humc, CLASS_BOURGEOIS, 5000,6.f,1.f,false);
        PopGroup ref=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_LABORER, 1000,4.f,0.20f,true);
        ref.arrival=ARR_REFUGIE; ref.home_reg=5; rp2.groups[1]=ref; rp2.n_groups=2;
        GroupReadout c9[DEMO_MAX_GROUPS];
        int n9=province_composition(&rp2,drift,&crown,5.f,5.f,c9,DEMO_MAX_GROUPS);
        const char *e_ref=NULL;
        for (int i=0;i<n9;i++) if (c9[i].etat && strstr(c9[i].etat,"réfugié")) e_ref=c9[i].etat;
        ok("le fuyard de guerre est nommé « réfugié » (voie de brassage neuve)", e_ref!=NULL);

        /* home_reg : un NATIF déplacé prend le foyer PASSÉ ; un DÉPLACÉ re-chassé le GARDE.
         * (le piège : home_reg est memset à 0 = région VALIDE — un natif ne doit PAS être vu
         * comme « ayant un foyer 0 » ; migration_move teste le src RÉEL, pas home_reg<0.) */
        ProvincePop A,B,C; memset(&A,0,sizeof A); memset(&B,0,sizeof B); memset(&C,0,sizeof C);
        A.groups[0]=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_LABORER, 900,5.f,1.f,false);
        A.n_groups=1;
        migration_move(&A,&B,0, 300, 900, ARR_REFUGIE, 5 /*foyer = région 5*/);
        ok("un natif déplacé inscrit son FOYER = la région de départ (5)",
           B.n_groups==1 && B.groups[0].arrival==ARR_REFUGIE && B.groups[0].home_reg==5);
        migration_move(&B,&C,0, 100, 901, ARR_REFUGIE, 8 /*re-chassé vers un autre camp (8)*/);
        ok("un réfugié RE-chassé garde son VRAI foyer (5), pas le dernier camp (8)",
           C.n_groups==1 && C.groups[0].home_reg==5);
    }

    /* ═══ 10. AUTO-VÉRIF — un empire ULTRA-BÂTI + ULTRA-PROSPÈRE est un AIMANT migratoire ══ */
    printf("\n── 10. Attractivité : bâti + prospérité → migration TRÈS élevée (aimant) ──\n");
    {
        float a_poor  = migration_attractivity(1.f, 0.f);    /* pauvre, non bâti */
        float a_mild  = migration_attractivity(4.f, 1.5f);   /* moyen */
        float a_ultra = migration_attractivity(10.f, 6.f);   /* ULTRA-bâti + ULTRA-prospère */
        printf("   attractivité : pauvre %.1f · moyen %.1f · ULTRA %.1f\n", a_poor, a_mild, a_ultra);
        ok("l'attractivité MONTE avec la prospérité ET le bâti", a_ultra>a_mild && a_mild>a_poor);
        ok("le BÂTI (institutions) compte dans l'attraction — pas que la prospérité",
           migration_attractivity(5.f,6.f) > migration_attractivity(5.f,0.f)+3.f);
        /* le flux moteur ÉCHELONNE avec le gradient d'attractivité (pull ∝ gradient/MIG_GRADIENT,
         * plafonné MIG_PULL_MAX) : un ULTRA a un gradient BIEN plus fort → migration TRÈS élevée. */
        float grad_ultra=a_ultra-a_poor, grad_mild=a_mild-a_poor;
        printf("   gradient d'attraction (vs pauvre) : ULTRA %.1f vs tiède %.1f\n", grad_ultra, grad_mild);
        ok("l'empire ULTRA aimante BIEN plus fort qu'un tiède (le flux échelonne)",
           grad_ultra > grad_mild*2.f);
    }

    /* ═══ 11. ESCLAVAGE — CLASS_SLAVE : bras sans pression d'intégration, affranchissement ══ */
    printf("\n── 11. Esclavage : bras/friction/mobilité/affranchissement ──\n");
    {
        /* 11a — la strate esclave n'entre PAS dans la friction culturelle (le prix du GARDER). */
        ProvincePop sp; memset(&sp,0,sizeof sp);
        sp.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES, humc, CLASS_LABORER, 5000,6.f,1.f,false);
        PopGroup slv=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_SLAVE, 2000,2.f,0.05f,true);
        slv.arrival=ARR_DEPORTE; sp.groups[1]=slv; sp.n_groups=2;
        float off_slave=econ_off_culture_fraction(&sp);
        sp.groups[1].klass=CLASS_LABORER;   /* même groupe, mais LIBRE : la friction devient réelle */
        float off_free=econ_off_culture_fraction(&sp);
        printf("   off-culture : esclave tenu %.4f vs le même groupe LIBRE %.4f\n", off_slave, off_free);
        ok("un groupe ESCLAVE ne compte PAS dans la friction culturelle (H décompresse)",
           off_slave==0.f && off_free>0.f);

        /* 11b — l'affranchissement bascule klass+arrival ET la strate économique. */
        World *w=calloc(1,sizeof(World));
        WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
        w->n_countries=1; w->n_provinces=1;
        w->country[0].capital_prov=0; w->province[0].region=0;
        e->n_prov=1; e->n_regions=1;
        ProvinceEconomy *pe=&e->prov[0];
        pe->owner=0; pe->active=true; pe->colonized=true; pe->region=0;
        pe->strata[CLASS_LABORER].pop=5000.f;
        pe->strata[CLASS_SLAVE].pop=2000.f;
        pe->pop.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES, humc, CLASS_LABORER, 5000,6.f,1.f,false);
        PopGroup slv2=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_SLAVE, 2000,2.f,0.05f,true);
        slv2.arrival=ARR_DEPORTE; pe->pop.groups[1]=slv2; pe->pop.n_groups=2;
        long freed=demography_manumit_country(e,0);
        ok("l'affranchissement libère TOUTES les âmes esclaves du pays", freed==2000);
        ok("la strate économique bascule CLASS_SLAVE→CLASS_LABORER (Σ constante)",
           pe->strata[CLASS_SLAVE].pop==0.f && pe->strata[CLASS_LABORER].pop==7000.f);
        ok("le groupe affranchi n'est plus CLASS_SLAVE", pe->pop.groups[1].klass==CLASS_LABORER);
        ok("l'arrival bascule DÉPORTÉ→MIGRANT (diffusion faible→pleine)",
           pe->pop.groups[1].arrival==ARR_MIGRANT);
        ok("l'INTÉGRATION est GARDÉE (le prix : la friction devient réelle, pas remise à 1)",
           pe->pop.groups[1].integration==0.05f);
        free(w); free(e);
    }

    /* ═══ 12. LOT G — RÉINCORPORATION DE POP (CMD_POP_TRANSFER, granularité RÉGION) ═ */
    printf("\n── 12. Réincorporation de pop : les groupes suivent, le coût frappe la source ──\n");
    {
        WorldEconomy *e=calloc(1,sizeof(WorldEconomy));
        e->n_prov=2; e->n_regions=2;
        e->region_rep_prov[0]=0; e->region_rep_prov[1]=1;
        ProvinceEconomy *pa=&e->prov[0], *pb=&e->prov[1];
        pa->owner=0; pa->active=true; pa->colonized=true; pa->region=0;
        pb->owner=0; pb->active=true; pb->colonized=true; pb->region=1;
        pa->strata[CLASS_LABORER].pop=10000.f;
        pa->pop.groups[0]=grp(HERITAGE_ADAPTATIF, SPHERE_HOMMES, humc, CLASS_LABORER, 10000,6.f,0.8f,false);
        pa->pop.groups[0].arrival=ARR_SOUMIS; pa->pop.n_groups=1;
        pb->strata[CLASS_LABORER].pop=1000.f;
        pb->pop.groups[0]=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_LABORER, 1000,6.f,0.5f,false);
        pb->pop.n_groups=1;
        float coerc_a_before=pa->coercion;
        long moved=demography_pop_transfer(e, 0, 1, CLASS_LABORER, 3000);
        ok("le transfert déplace le compte demandé (sous le plancher anti-vidage)", moved==3000);
        ok("la strate ÉCONOMIQUE suit (Σ constante)",
           pa->strata[CLASS_LABORER].pop==7000.f && pb->strata[CLASS_LABORER].pop==4000.f);
        ok("le groupe déplacé GARDE son arrival (SOUMIS) — migration_move, pas une refonte",
           pb->pop.groups[pb->pop.n_groups-1].arrival==ARR_SOUMIS);
        ok("LE COÛT : la coercition MONTE à la SOURCE (strate LIBRE)",
           pa->coercion>coerc_a_before);

        /* plancher anti-vidage : jamais plus de 50% de la classe ciblée. */
        long moved2=demography_pop_transfer(e, 0, 1, CLASS_LABORER, 100000);
        ok("le plancher anti-vidage borne à 50% de la classe ciblée dans la source",
           moved2<=3500 && moved2>0);

        /* CLASS_SLAVE exempt de coercition (on déplace une propriété). */
        ProvinceEconomy *pc=&e->prov[0], *pd=&e->prov[1];
        pc->strata[CLASS_SLAVE].pop=4000.f;
        PopGroup slv=grp(HERITAGE_CLANIQUE, SPHERE_ETRANGERS, cult(8,2,8,7,ETHOS_HONNEUR), CLASS_SLAVE, 4000,2.f,0.05f,true);
        slv.arrival=ARR_DEPORTE;
        pc->pop.groups[pc->pop.n_groups++]=slv;
        float coerc_c_before=pc->coercion;
        long moved3=demography_pop_transfer(e, 0, 1, CLASS_SLAVE, 1000);
        ok("un transfert d'ESCLAVES déplace bien les âmes", moved3==1000);
        ok("CLASS_SLAVE est EXEMPT de coercition à la source (on déplace une propriété)",
           pc->coercion==coerc_c_before);
        (void)pd;

        /* A==B refusé (même région) */
        long same=demography_pop_transfer(e, 0, 0, CLASS_LABORER, 100);
        ok("A==B est refusé (aucune âme ne se déplace vers elle-même)", same==0);
        free(e);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(drift);
    return g_fail?1:0;
}
