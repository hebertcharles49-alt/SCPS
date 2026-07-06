/*
 * culture_demo.c — banc d'essai des pools culturels (console)
 *
 *   make culture_demo && ./culture_demo
 *
 * Démontre :
 *   1. Deux cultures par biome (même corps, âme contraire — le pire rival).
 *   2. Le moteur de mutation : éthos hérité × mode de vie verrouillé →
 *      hybride émergent nommé (« le viking des steppes »).
 *   3. La lecture de distance : D_inf (contenu) vs horloge, et le vocabulaire
 *      de relation lisible.
 *   4. Le syncrétisme : substrat A + élite B → hybride aux traits mutés.
 */
#include "scps_culture.h"
#include <stdio.h>

/* ---- mini-cadre d'ASSERTION (banc auto-vérifiant : le harnais lit « X réussis, Y échoués ») ---- */
static int g_pass=0, g_fail=0;
#define CK(cond,msg) do{ if (cond) g_pass++; else { g_fail++; printf("  ✗ %s\n",(msg)); } }while(0)

static void print_culture(const Culture *c, const char *tag){
    printf("  %-10s « %s »%s\n", tag, c->name, c->is_hybrid?"  [hybride]":"");
    printf("     éthos=%-11s  mode=%-22s  struct=%-15s  credo=%s/%s\n",
           ethos_name(c->ethos), lifeway_name(c->lifeway),
           structure_name(c->structure), religion_branch_name(c->rel_branch),
           credo_name(c->credo));
    printf("     martial=%-22s  éco=%s\n",
           martial_name(c->martial), econ_name(c->econ));
    printf("     axes : val=%.1f subs=%.1f par=%.1f rel=%.1f langue=%.1f\n",
           c->valeurs, c->subsistance, c->parente, c->religion, c->langue);
}

static void print_relation(const Culture *a, const Culture *b){
    printf("  %-24s ↔ %-24s\n", a->name, b->name);
    printf("     D_inf(contenu)=%.1f  horloge=%.1f  →  %s\n",
           culture_content_distance(a,b), culture_clock_distance(a,b),
           relation_name(culture_relation(a,b)));
}

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" POOLS CULTURELS — un trait est une coordonnée d'axe qu'on lit\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* 1. Deux cultures par biome (§8) -------------------------------------- */
    printf("\n── 1. DEUX CULTURES PAR BIOME (même vie, âme contraire) ──\n");
    Biome biomes[]={BIO_STEPPE, BIO_MOUNTAINS, BIO_COAST, BIO_PLAINS, BIO_JUNGLE};
    for (unsigned i=0;i<sizeof(biomes)/sizeof(biomes[0]);i++){
        Culture a,b; culture_pair_for_biome(biomes[i], &a, &b, REL_ANIMISTE);
        printf("\n  [%s]\n", lifeway_name(lifeway_for_biome(biomes[i])));
        print_culture(&a,"A");
        print_culture(&b,"B");
        printf("     → pire rival : même corps (subs %.1f), âme contraire "
               "(val %.1f vs %.1f)\n", a.subsistance, a.valeurs, b.valeurs);
        /* INVARIANT §8 : même CORPS (mode de vie + subsistance identiques), âme CONTRAIRE (valeurs ≠). */
        CK(a.lifeway == b.lifeway,            "paire/biome : même mode de vie");
        CK(a.subsistance == b.subsistance,    "paire/biome : même corps (subsistance)");
        CK(a.valeurs != b.valeurs,            "paire/biome : âme contraire (valeurs)");
    }

    /* 2. Le viking des steppes (§4) --------------------------------------- */
    printf("\n── 2. MUTATION : éthos hérité × biome verrouillé ──\n");
    /* Honneur d'origine maritime (razzia) débarque sur la steppe : pas de mer. */
    Culture viking_orig = culture_make(BIO_COAST, ETHOS_HONNEUR, REL_ANIMISTE, CREDO_PLURALISTE);
    Culture viking_step = culture_make(BIO_STEPPE, ETHOS_HONNEUR, REL_ANIMISTE, CREDO_PLURALISTE);
    printf("  Honneur sur la CÔTE :\n"); print_culture(&viking_orig,"origine");
    printf("  Le même Honneur sur la STEPPE (pas de mer) :\n");
    print_culture(&viking_step,"muté");
    printf("  → la razzia maritime n'a plus d'océan : elle MUTE en %s.\n",
           martial_name(viking_step.martial));

    Culture merch_mount = culture_make(BIO_MOUNTAINS, ETHOS_MERCANTILE, REL_ABRAHAMIQUE, CREDO_PLURALISTE);
    printf("\n  Mercantile enclavé en MONTAGNE :\n");
    print_culture(&merch_mount,"muté");
    printf("  → commerce sans voies → %s ; garde → %s.\n",
           econ_name(merch_mount.econ), martial_name(merch_mount.martial));
    /* INVARIANT §4 : l'éthos hérité MUTE quand le biome verrouille (la razzia maritime sans mer). */
    CK(viking_step.martial != viking_orig.martial, "Honneur côte→steppe : la doctrine MUTE (plus de mer)");

    /* 3. Distance & relations (§9) ---------------------------------------- */
    printf("\n── 3. LECTURE DE DISTANCE (contenu vs horloge) ──\n");
    Culture grenier = culture_make(BIO_PLAINS, ETHOS_BUREAUCRATE, REL_SINIQUE, CREDO_PLURALISTE);
    /* jumeau transcontinental : même contenu, horloge éloignée */
    Culture step_far = viking_step; step_far.langue = 9.5f;  /* horloge divergée */
    print_relation(&viking_step, &grenier);     /* étrangers/cousins */
    print_relation(&viking_step, &step_far);    /* jumeaux convergents */
    /* schisme : même branche, prosélytisme haut des deux côtés */
    Culture zealA = culture_make(BIO_DESERT, ETHOS_HONNEUR, REL_ABRAHAMIQUE, CREDO_EVANGELISTE);
    Culture zealB = culture_make(BIO_DESERT, ETHOS_DOMINATEUR, REL_ABRAHAMIQUE, CREDO_PURIFICATEUR);
    print_relation(&zealA, &zealB);             /* ennemis-schismatiques */
    /* INVARIANT §9 : DEUX CANAUX distincts. Les jumeaux (step_far ne diverge QUE l'horloge/langue) sont
     * PROCHES en contenu mais LOIN sur l'horloge ; un étranger (grenier) est plus loin EN CONTENU. */
    float cd_twin = culture_content_distance(&viking_step, &step_far);
    float ck_twin = culture_clock_distance  (&viking_step, &step_far);
    CK(ck_twin > cd_twin,                                                  "jumeaux : l'horloge diverge plus que le contenu");
    CK(culture_content_distance(&viking_step,&grenier) > cd_twin,          "étranger : plus distant EN CONTENU que les jumeaux");

    /* 4. Syncrétisme : le GOUFFRE est un continuum, pas un mur (v3) -------- */
    printf("\n── 4. SYNCRÉTISME — tout s'assimile, à différentes échelles ──\n");
    Culture substrat = culture_make(BIO_PLAINS, ETHOS_HONNEUR, REL_DHARMIQUE, CREDO_PLURALISTE);
    Culture elite    = culture_make(BIO_PLAINS, ETHOS_BUREAUCRATE, REL_SINIQUE, CREDO_EVANGELISTE);
    /* paire LOINTAINE (le « gouffre ») : forêt pacifiste ↔ désert dominateur purificateur */
    Culture doux = culture_make(BIO_FOREST, ETHOS_PACIFISTE,  REL_ANIMISTE,    CREDO_PLURALISTE);
    Culture dur  = culture_make(BIO_DESERT, ETHOS_DOMINATEUR, REL_ABRAHAMIQUE, CREDO_PURIFICATEUR);

    struct { const char *tag; const Culture *a, *b; } pr[] = {
        { "proche  (demi-ésotériques)",  &substrat, &elite },
        { "lointain (le gouffre)", &doux,     &dur   },
    };
    struct { const char *who; float P, K; } st[] = {
        { "État moyen (P=5,K=5)",            5.f, 5.f },
        { "empire ouvert+capable (P=9,K=9)", 9.f, 9.f },
    };
    for (unsigned i=0;i<2;i++){
        printf("\n  %s — D∞=%.1f\n", pr[i].tag, culture_content_distance(pr[i].a, pr[i].b));
        for (unsigned j=0;j<2;j++){
            SyncFeasibility f = culture_can_syncretize(pr[i].a, pr[i].b, st[j].P, st[j].K);
            printf("     %-34s porte=%.2f  %-18s  temps≈%.0f ticks\n",
                   st[j].who, f.openness,
                   f.feasible ? "OUVERTE" : "fermée (monte P+K)", f.time_ticks);
        }
    }
    printf("\n  → jamais un « impossible » : le gouffre s'ouvre à forte P+K, lentement (temps ∝ D∞).\n");
    Culture fused;
    bool fused_ok = culture_syncretize(&doux, &dur, &fused);   /* la fusion produit toujours une fiche */
    if (fused_ok){
        printf("  fusion forêt↔désert (porte ouverte, après le temps requis) :\n");
        print_culture(&fused,"hybride");
    }
    /* INVARIANT v3 : le syncrétisme est un CONTINUUM. La porte s'OUVRE avec P+K ; le « gouffre »
     * (doux↔dur) est plus DUR et plus LENT que le proche (substrat↔élite) ; la fusion rend un hybride. */
    SyncFeasibility sp_lo = culture_can_syncretize(&substrat,&elite,5.f,5.f);
    SyncFeasibility sp_hi = culture_can_syncretize(&substrat,&elite,9.f,9.f);
    SyncFeasibility gf_hi = culture_can_syncretize(&doux,&dur,9.f,9.f);
    CK(sp_hi.openness > sp_lo.openness,     "la porte de syncrétisme s'OUVRE avec P+K");
    CK(sp_hi.feasible,                      "proche à P+K haut : OUVERTE");
    CK(gf_hi.openness < sp_hi.openness,     "le gouffre est plus DUR que le proche (à P+K égal)");
    CK(gf_hi.time_ticks > sp_hi.time_ticks, "temps de fusion ∝ distance (le gouffre plus lent)");
    CK(fused_ok && fused.is_hybrid,         "la fusion produit un HYBRIDE");

    /* 5. Lecteurs manquants (mission « lecteurs ») ------------------------- */
    printf("\n── 5. LECTEURS MANQUANTS — culture_relation_of + region_ethos_drift ──\n");
    /* culture_relation_of : mêmes fiches que §3, déballées en champs nus — DOIT
     * renvoyer EXACTEMENT le même verdict que culture_relation (source unique). */
    CultureRelation r_direct = culture_relation(&viking_step, &grenier);
    CultureRelation r_of = culture_relation_of(
        viking_step.langue, viking_step.valeurs, viking_step.subsistance,
        viking_step.parente, viking_step.religion, viking_step.credo, viking_step.rel_branch,
        grenier.langue, grenier.valeurs, grenier.subsistance,
        grenier.parente, grenier.religion, grenier.credo, grenier.rel_branch);
    CK(r_of == r_direct, "culture_relation_of ≡ culture_relation (mêmes champs, même verdict)");

    CultureRelation zeal_direct = culture_relation(&zealA, &zealB);
    CultureRelation zeal_of = culture_relation_of(
        zealA.langue, zealA.valeurs, zealA.subsistance, zealA.parente, zealA.religion,
        zealA.credo, zealA.rel_branch,
        zealB.langue, zealB.valeurs, zealB.subsistance, zealB.parente, zealB.religion,
        zealB.credo, zealB.rel_branch);
    CK(zeal_of == zeal_direct && zeal_of == REL_ENNEMIS_SCHISME,
       "culture_relation_of : cas schisme identique (ennemis-schismatiques)");

    /* region_ethos_drift : ~0 quand la région PARTAGE l'éthos régnant ; monte
     * quand une marche dominatrice vit sous une couronne pacifiste. */
    float drift_same = region_ethos_drift(ETHOS_BUREAUCRATE, ETHOS_BUREAUCRATE);
    float drift_far   = region_ethos_drift(ETHOS_DOMINATEUR, ETHOS_PACIFISTE);
    printf("  même éthos que la couronne : drift=%.2f ; marche dominatrice/couronne pacifiste : drift=%.2f\n",
           drift_same, drift_far);
    CK(drift_same == 0.f,                          "region_ethos_drift : même éthos que la couronne → 0");
    CK(drift_far > 0.9f,                            "region_ethos_drift : dominateur/pacifiste → proche du max [0..1]");
    CK(drift_far >= 0.f && drift_far <= 1.f,        "region_ethos_drift : borné [0..1]");

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" La friction lit D_inf sur le contenu ; le cousinage lit l'horloge.\n");
    printf(" Tout le drame vit dans l'écart entre les deux canaux.\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("\n BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
