/*
 * influence_demo.c — L'INFLUENCE POLITIQUE (docs/DESIGN_MISSIONS_DOCTRINES.md §3)
 *
 *   make influence_demo && ./influence_demo [graine]
 *
 * Prouve : génération mensuelle = INFLUENCE_PER_NOBLE × élites(prov[]) × mult_conseil ;
 * plancher INFLUENCE_COUNCIL_FLOOR quand aucun siège n'est pourvu ; le rang du Conseil
 * relève le gain ; dépense/refus (jamais négatif) ; persistance (round-trip binaire,
 * motif section INFL) ; et — au niveau de la FAÇADE (le drain réel, scps_sim.c) — le
 * remplacement du cooldown de l'émissaire par le coût en influence + le plancher COURT
 * DIPLO_ENVOY_FLOOR_DAYS (30 j, contre 60 j avant §3), et CMD_DECLARE_WAR qui reste
 * libre de tout ça (« la guerre n'attend pas la cour »).
 */
#include "scps_influence.h"
#include "scps_culture.h"
#include "scps_heritage.h"
#include "scps_religion.h"  /* religion_set_country/of_country : le terme des FIDELES de Divin */
#include "scps_tune.h"
#include "scps_api.h"
#include "scps_fog.h"      /* fog_debug_meet_all : découverte forcée, banc seul (motif scps_api_demo) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, int cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static bool near_f(double a, double b, double eps){ double d=a-b; if(d<0)d=-d; return d<=eps; }

/* Pose les SIEGES (pop_by_class) d'UN groupe unique dans la province — la
 * réalité de classe que lit l'influence (jamais les strata[], une AUTRE
 * réalité de classe du moteur, cf. scps_influence.c). n_groups=1 ⇒ les autres
 * groupes semés par worldgen_seed_peoples n'entrent plus dans aucune somme. */
static void set_seats(ProvinceEconomy *pe, long elite, long bourgeois, long laborer){
    ProvincePop *pp = &pe->pop;
    pp->n_groups = 1;
    PopGroup *g = &pp->groups[0];
    g->pop_by_class[CLASS_ELITE]     = elite;
    g->pop_by_class[CLASS_BOURGEOIS] = bourgeois;
    g->pop_by_class[CLASS_LABORER]   = laborer;
    g->pop_by_class[CLASS_SLAVE]     = 0;
    g->count = elite + bourgeois + laborer;
}
static void zero_country_seats(WorldEconomy *econ, int cid){
    for (int i=0;i<econ->n_prov;i++)
        if (econ->prov[i].owner==cid) set_seats(&econ->prov[i], 0,0,0);
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;

    printf("==================================================================\n");
    printf(" INFLUENCE POLITIQUE (§3) -- generation, plancher, depense, persistance\n");
    printf("==================================================================\n");

    /* ═══ 1. MODULE — génération, plancher du Conseil, dépense/refus ═══ */
    World *w=malloc(sizeof(World)); WorldEconomy *econ=malloc(sizeof(WorldEconomy));
    Statecraft *sc=malloc(sizeof(Statecraft));
    InfluenceState *is=malloc(sizeof(InfluenceState));
    if(!w||!econ||!sc||!is){ fprintf(stderr,"OOM\n"); return 1; }

    WorldParams p=worldparams_default(seed);
    world_generate(w,&p);
    econ_init(econ,w); gen_population(w,econ); worldgen_seed_peoples(w,econ,HERITAGE_ADAPTATIF);
    statecraft_init(sc,w);
    influence_init(is);

    int cid=-1;
    for (int c=0;c<w->n_countries;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].role==POLITY_WILD) continue;
        if (w->country[c].capital_prov<0) continue;
        cid=c; break;
    }
    if (cid<0){ fprintf(stderr,"monde trop vide -- autre graine\n"); return 1; }

    /* ── 1a. Les SIÈGES CONNUS (pop_by_class, prov[]) — JAMAIS les strata[] :
     * la double réalité de classe du moteur, cf. scps_influence.c ── */
    printf("\n-- 1. Generation mensuelle : PER_NOBLE x elites(sieges, pop_by_class) x mult_conseil --\n");
    zero_country_seats(econ, cid);
    int fixed_prov=-1;
    for (int i=0;i<econ->n_prov && fixed_prov<0;i++) if (econ->prov[i].owner==cid) fixed_prov=i;
    ok("le pays choisi possede au moins une province", fixed_prov>=0);
    if (fixed_prov<0) return 1;
    set_seats(&econ->prov[fixed_prov], 1000, 0, 0);   /* élites(cid) = 1000 sièges, EXACT */
    double elites_now = influence_elites(econ, cid);
    ok("influence_elites somme les SIEGES (pop_by_class) au pays (prov[], jamais region[].pop ni strata[])",
       near_f(elites_now, 1000.0, 0.5));

    tune_set("INFLUENCE_PER_NOBLE", 0.002f);
    tune_set("INFLUENCE_PER_NOBLE_ARISTO", 0.0025f);
    tune_set("INFLUENCE_PER_BOURGEOIS_BASE", 0.0011f);
    tune_set("INFLUENCE_PER_BOURGEOIS", 0.0022f);
    tune_set("INFLUENCE_PER_LABORER_BASE", 0.00011f);
    tune_set("INFLUENCE_PER_LABORER", 0.00022f);
    tune_set("INFLUENCE_PER_BELIEVER", 0.00016667f);
    tune_set("INFLUENCE_COUNCIL_FLOOR", 1.0f);
    tune_set("INFLUENCE_CAP", 0.0f);

    /* ── 1a-bis. L'ASSIETTE — les TROIS classes toujours, 60/20/20, les
     * courants (jamais un malus), Divin (les FIDÈLES) ── */
    printf("\n-- 1bis. L'assiette : 3 classes (60/20/20), chaque courant >= defaut, Divin --\n");
    set_seats(&econ->prov[fixed_prov], 130, 80, 780);   /* 13/8/78 % — la repartition du chronicle (SIEGES) */
    double e3=0.0,b3=0.0,l3=0.0;
    influence_seats(econ, cid, &e3, &b3, &l3);
    ok("influence_seats lit les TROIS classes exactement (130/80/780)",
       near_f(e3,130.0,0.5) && near_f(b3,80.0,0.5) && near_f(l3,780.0,0.5));

    double g_def = influence_base_gain(econ, cid, INFL_BASE_DEFAUT);
    double expect_def = 130.0*0.002 + 80.0*0.0011 + 780.0*0.00011;
    double share_e = (130.0*0.002)/g_def, share_b=(80.0*0.0011)/g_def, share_l=(780.0*0.00011)/g_def;
    printf("   assiette defaut (13/8/78%%) = %.5f/mois -- parts e=%.1f%% b=%.1f%% l=%.1f%%\n",
           g_def, share_e*100.0, share_b*100.0, share_l*100.0);
    ok("l'assiette par defaut SOMME les trois classes (jamais un seul terme)",
       near_f(g_def, expect_def, 0.0001));
    ok("la repartition 13/8/78 pose ~60% de part aux elites", fabs(share_e-0.60)<0.02);
    ok("la repartition 13/8/78 pose ~20% de part aux bourgeois", fabs(share_b-0.20)<0.02);
    ok("la repartition 13/8/78 pose ~20% de part aux journaliers", fabs(share_l-0.20)<0.02);

    double g_aristo = influence_base_gain(econ, cid, INFL_BASE_ARISTO);
    double g_bourg  = influence_base_gain(econ, cid, INFL_BASE_BOURGEOIS);
    double g_labor  = influence_base_gain(econ, cid, INFL_BASE_LABORER);
    ok("Aristocratie releve SEULEMENT les elites (strictement > defaut, jamais un malus)", g_aristo > g_def);
    ok("Bourgeoisie releve SEULEMENT les bourgeois (strictement > defaut, jamais un malus)", g_bourg > g_def);
    ok("Populaire releve SEULEMENT les journaliers (strictement > defaut, jamais un malus)", g_labor > g_def);

    /* Divin — le terme des FIDÈLES (grain GROUPE, jamais region[]) ajouté à
     * l'assiette par défaut (aucun taux de classe relevé). */
    econ->prov[fixed_prov].pop.groups[0].faith = -1;   /* athée : aucun fidèle */
    double g_divin_sans = influence_base_gain(econ, cid, INFL_BASE_FAITH);
    ok("sans religion fondee (religion_of_country<0), Divin == l'assiette par defaut (terme fideles NUL)",
       near_f(g_divin_sans, g_def, 0.0001));

    religion_set_country(cid, 0);                      /* fonde une foi d'État MINIMALE pour le banc */
    econ->prov[fixed_prov].pop.groups[0].faith = 0;     /* le groupe professe la foi d'État */
    double fideles = influence_base_pop(econ, cid, INFL_BASE_FAITH);
    ok("les fideles somment les AMES (count) du groupe qui professe la religion d'Etat",
       near_f(fideles, 130.0+80.0+780.0, 0.5));
    double g_divin = influence_base_gain(econ, cid, INFL_BASE_FAITH);
    double expect_divin = g_def + fideles * 0.00016667;
    printf("   Divin : defaut %.5f + %.0f fideles / 6000 = %.5f (attendu %.5f)\n",
           g_def, fideles, g_divin, expect_divin);
    ok("Divin = assiette par defaut + fideles x INFLUENCE_PER_BELIEVER",
       near_f(g_divin, expect_divin, 0.001));
    ok("Divin n'est jamais un malus (>= assiette par defaut)", g_divin >= g_def);
    religion_set_country(cid, -1);   /* efface le lien : le banc reste propre pour la suite */

    /* ── 1a-ter. é COHÉRENT — l'échelle d'assiette avec le modèle 3-classes
     * (mission §5) : empire de départ (~2750 hab, 13/8/78) ≈1.2/mois ⇒ é≈0.6 ;
     * empire mûr (~13000 hab) ≈5.7/mois ⇒ é≈2.8. Le plancher 0.25 reste
     * atteignable par un micro-pays (<~1150 hab dans cette répartition) —
     * PAS retiré : la mission ne demandait de le signaler que s'il devenait
     * inatteignable par un VRAI empire, ce qui n'est pas le cas ici. ── */
    printf("\n-- 1ter. e (l'echelle) reste coherent avec l'assiette 3-classes --\n");
    set_seats(&econ->prov[fixed_prov], (long)(2750*0.13), (long)(2750*0.08), (long)(2750*0.78));
    double g_start = influence_base_gain(econ, cid, INFL_BASE_DEFAUT);
    float  e_start = influence_scale(econ, cid, INFL_BASE_DEFAUT);
    printf("   empire de depart (~2750 hab, 13/8/78) : %.3f/mois -- e=%.3f\n", g_start, e_start);
    ok("empire de depart (~2750 hab) : ~1.2/mois", near_f(g_start, 1.2, 0.05));
    ok("empire de depart (~2750 hab) : e ~= 0.6", near_f((double)e_start, 0.6, 0.05));

    set_seats(&econ->prov[fixed_prov], (long)(13000*0.13), (long)(13000*0.08), (long)(13000*0.78));
    double g_mature = influence_base_gain(econ, cid, INFL_BASE_DEFAUT);
    float  e_mature = influence_scale(econ, cid, INFL_BASE_DEFAUT);
    printf("   empire mur (~13000 hab, 13/8/78) : %.3f/mois -- e=%.3f\n", g_mature, e_mature);
    ok("empire mur (~13000 hab) : ~5.7/mois", near_f(g_mature, 5.7, 0.1));
    ok("empire mur (~13000 hab) : e ~= 2.8", near_f((double)e_mature, 2.8, 0.1));

    set_seats(&econ->prov[fixed_prov], 15, 9, 90);   /* micro-hameau ~114 hab, 13/8/78 */
    float e_micro = influence_scale(econ, cid, INFL_BASE_DEFAUT);
    ok("un micro-pays retombe sur le plancher (e==0.25, toujours atteignable)",
       near_f((double)e_micro, 0.25, 0.001));

    /* remet le fixture SIMPLE (1000 elites, EXACT) pour les sections suivantes
     * (plancher / rang du Conseil / dépense) — leurs attentes numériques datent
     * d'avant l'ajout de ce sous-test. */
    set_seats(&econ->prov[fixed_prov], 1000, 0, 0);

    /* ── 1b. Aucun siège pourvu → PLANCHER (jamais un Conseil muet) ── */
    printf("\n-- 2. Aucun ministre en siege -> plancher INFLUENCE_COUNCIL_FLOOR --\n");
    int nseat=-1;
    float mult0 = influence_council_mult(sc, w->seed, cid, &nseat);
    ok("aucun siege pourvu => n_seated==0", nseat==0);
    ok("aucun siege pourvu => mult_conseil == INFLUENCE_COUNCIL_FLOOR (1.0, jamais un Conseil muet)",
       near_f(mult0, 1.0, 0.001));

    influence_tick(is, w, econ, sc, w->seed, cid, INFL_BASE_DEFAUT);
    double gain1 = influence_get(is, cid);
    double expect1 = 0.002 * 1000.0 * 1.0;
    printf("   gain mois 1 (plancher) : %.4f (attendu %.4f)\n", gain1, expect1);
    ok("gain/mois == INFLUENCE_PER_NOBLE x elites x plancher (aucun ministre)", near_f(gain1, expect1, 0.01));

    /* ── 1c. Un ministre pourvu → le RANG du Conseil relève le gain ── */
    printf("\n-- 3. Un ministre pourvu -> le rang du Conseil (I-III) relève le gain --\n");
    int best_slot=0, best_tier=0;
    for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
        int t = statecraft_council_cand_tier(w->seed, cid, 0, sl, 0);
        if (t>best_tier){ best_tier=t; best_slot=sl; }
    }
    statecraft_council_hire(sc, w->seed, cid, 0, best_slot, 0);
    int nseat2=-1;
    float mult1 = influence_council_mult(sc, w->seed, cid, &nseat2);
    ok("un siege pourvu => n_seated==1", nseat2==1);
    ok("mult_conseil == le rang (I..III) du seul ministre en siege", near_f(mult1, (double)best_tier, 0.001));
    influence_tick(is, w, econ, sc, w->seed, cid, INFL_BASE_DEFAUT);
    double gain2 = influence_get(is, cid) - gain1;
    double expect2 = 0.002 * 1000.0 * (double)best_tier;
    printf("   gain mois 2 (rang %d) : %.4f (attendu %.4f)\n", best_tier, gain2, expect2);
    ok("gain/mois croit avec le rang du Conseil (mult_conseil = rang, pas le plancher)",
       near_f(gain2, expect2, 0.01));
    if (best_tier>1)
        ok("un ministre de rang > I genere STRICTEMENT plus que le plancher", gain2 > expect1 + 0.001);

    /* ── 1d. Dépense / refus (jamais négatif) ── */
    printf("\n-- 4. Depense / refus -- l'accumulateur ne descend jamais sous 0 --\n");
    float stock_before = influence_get(is, cid);
    ok("influence_can_spend refuse un cout > stock", !influence_can_spend(is, cid, stock_before + 1000.f));
    ok("influence_can_spend accepte un cout <= stock", influence_can_spend(is, cid, stock_before));
    influence_spend(is, cid, stock_before + 1000.f);   /* dépense EXCESSIVE : clampe à 0, ne va jamais négatif */
    ok("une depense excessive clampe le stock a 0 (jamais negatif)", influence_get(is,cid)==0.f);
    influence_tick(is, w, econ, sc, w->seed, cid, INFL_BASE_DEFAUT);      /* re-génère un mois pour la suite */
    float restocked = influence_get(is, cid);
    influence_spend(is, cid, restocked * 0.5f);
    ok("une depense LEGALE debite exactement le cout", near_f(influence_get(is,cid), restocked*0.5, 0.01));

    /* ── 1e. Persistance : round-trip binaire (motif section INFL, save v104) ── */
    printf("\n-- 5. Persistance : round-trip binaire de l'accumulateur --\n");
    char tmp_path[64]; snprintf(tmp_path,sizeof tmp_path,"influence_demo_%u.tmp", (unsigned)seed);
    float stock_saved = influence_get(is, cid);
    FILE *tf = fopen(tmp_path, "wb");
    ok("le fichier temporaire de round-trip s'ouvre en ecriture", tf!=NULL);
    if (tf){ fwrite(is, sizeof *is, 1, tf); fclose(tf); }
    InfluenceState *is2 = malloc(sizeof(InfluenceState));
    influence_init(is2);
    tf = fopen(tmp_path, "rb");
    ok("le fichier temporaire de round-trip se relit", tf!=NULL);
    if (tf){ size_t n=fread(is2, sizeof *is2, 1, tf); fclose(tf); ok("round-trip : 1 bloc complet lu", n==1); }
    remove(tmp_path);
    ok("SAVE->RELOAD conserve le stock d'influence au bit pres (section INFL)",
       influence_get(is2,cid)==stock_saved);
    free(is2);

    free(w); free(econ); free(sc); free(is);

    /* ═══ 2. FAÇADE — le DRAIN réel (scps_sim.c) : coût REMPLACE le cooldown ═══ */
    printf("\n-- 6. Facade (drain reel) : le cout REMPLACE le cooldown de l'emissaire --\n");
    tune_set("INFLUENCE_COST_ENVOY", 12.f);
    tune_set("DIPLO_ENVOY_FLOOR_DAYS", 30.f);
    ScpsSim *s2 = scps_sim_new();
    scps_sim_generate(s2, seed);
    int pl = scps_player(s2);
    fog_debug_meet_all(pl);   /* motif scps_api_demo : le banc prouve la plomberie, pas l'exploration */
    int nc = scps_country_count(s2);
    int t1=-1, t2=-1;
    for (int c=0;c<nc;c++){
        if (c==pl) continue;
        ScpsDiploOptions o;
        if (!scps_diplo_options(s2,c,&o)) continue;
        if (t1<0 && o.can_offer_pact) t1=c;
        else if (t2<0 && o.can_embargo) t2=c;
        if (t1>=0 && t2>=0) break;
    }
    ok("deux cibles diplomatiques valides trouvees (pacte + embargo)", t1>=0 && t2>=0);
    if (t1>=0 && t2>=0){
        /* laisse le stock monter au-dessus de 2x le cout d'un envoi (le Conseil peut être
         * vide au départ : le PLANCHER suffit déjà à générer un gain non nul). */
        int budget_days=0;
        ScpsInfluence inf;
        do { scps_sim_advance_days(s2, 30); scps_influence_info(s2, pl, &inf); budget_days+=30; }
        while (inf.stock < 30 && budget_days < 3650);
        ok("le stock d'influence croit avec le temps (generation mensuelle, joueur seul)", inf.stock>0);

        int before_stock = inf.stock;
        int r1 = scps_player_offer_pact(s2, t1);
        ok("verbe d'ENVOI (pacte) ENFILE", r1==1);
        scps_sim_advance_days(s2, 1);   /* le drain applique : coût débité, plancher posé */
        scps_influence_info(s2, pl, &inf);
        printf("   stock avant/apres 1er envoi : %d -> %d (cout attendu %.0f)\n",
               before_stock, inf.stock, tune_f("INFLUENCE_COST_ENVOY",12.f));
        ok("le verbe d'ENVOI a COUTE de l'influence (le cout REMPLACE le cooldown)",
           inf.stock <= before_stock - (int)tune_f("INFLUENCE_COST_ENVOY",12.f) + 1);

        /* coup sur coup, SOUS le plancher (30 j) : refusé net, même si l'influence suffit. */
        ScpsDiploOptions o2; scps_diplo_options(s2, t2, &o2);
        int emb_on_before = o2.can_embargo;
        int r2 = scps_player_embargo(s2, t2, 1);
        ok("second verbe d'ENVOI enfile (l'enfilement n'est jamais refuse, seul le drain tranche)", r2==1);
        scps_sim_advance_days(s2, 1);
        int cd_mid = scps_diplo_cd(s2);
        ok("SOUS le plancher (< DIPLO_ENVOY_FLOOR_DAYS) : l'emissaire reste occupe (cd>0)", cd_mid>0);
        scps_diplo_options(s2, t2, &o2);
        ok("le second verbe (coup sur coup, sous le plancher) N'EST PAS applique (embargo inchange)",
           o2.can_embargo == emb_on_before);

        /* au-delà du NOUVEAU plancher (31 j, largement sous l'ANCIEN cooldown 60 j) : reprend,
         * si l'influence a été reconstituée entre-temps (générée mois après mois). */
        scps_sim_advance_days(s2, 31);
        do { scps_influence_info(s2, pl, &inf); if (inf.stock >= (int)tune_f("INFLUENCE_COST_ENVOY",12.f)) break;
             scps_sim_advance_days(s2, 30); } while (1);
        int r3 = scps_player_embargo(s2, t2, 1);
        scps_sim_advance_days(s2, 1);
        scps_diplo_options(s2, t2, &o2);
        printf("   embargo (au-dela du plancher raccourci %d j) : can_embargo avant=%d apres=%d\n",
               (int)tune_f("DIPLO_ENVOY_FLOOR_DAYS",30.f), emb_on_before, o2.can_embargo);
        ok("PASSE le plancher raccourci (30 j, << l'ancien cooldown 60 j) : le second envoi REPREND",
           r3==1 && o2.can_embargo != emb_on_before);
    }

    /* ── CMD_DECLARE_WAR reste libre : jamais grisé par l'émissaire (motif façade) ── */
    printf("\n-- 7. CMD_DECLARE_WAR reste GRATUIT et HORS EMISSAIRE --\n");
    /* re-scanne une cible VIVANTE maintenant (le monde a tourné plusieurs milliers de
     * jours depuis t1/t2 : un pays peut avoir été conquis/absorbé entre-temps, motif
     * scps_api_demo qui re-scanne à chaque usage plutôt que de réutiliser un vieil index). */
    int war_target=-1;
    { int nc3=scps_country_count(s2);
      for (int c=0;c<nc3;c++){ if (c==pl) continue; ScpsDiploOptions tmp;
          if (scps_diplo_options(s2,c,&tmp)){ war_target=c; break; } } }
    ScpsActionLegal war_legal;
    int gotw = (war_target>=0) && scps_diplo_action_legal(s2, war_target, SCPS_DIPLO_WAR, &war_legal);
    ok("la legalite de la GUERRE se lit meme juste apres un envoi diplo (facade repond)", gotw==1);
    ok("la GUERRE n'est JAMAIS refusee pour cause d'emissaire occupe (« la guerre n'attend pas la cour »)",
       strcmp(war_legal.reason_code, "emissary_busy")!=0);

    scps_sim_free(s2);

    printf("\n==================================================================\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("==================================================================\n");
    return g_fail?1:0;
}
