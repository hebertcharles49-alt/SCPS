/*
 * factions_demo.c — LES FACTIONS PAR ÉTHOS (passe 1) : le spectre + l'enracinement
 *
 *   make factions_demo && ./factions_demo
 *
 * Prouve : (1) six factions = les axes IA + le Communautaire ; (2) le penchant
 * d'un groupe vient de son éthos / sa heritage / son credo ; (3) la distribution d'un
 * pays = Σ groupes, et CONQUÉRIR un peuple clanique MONTE les Conquérants ; (4) la
 * classe pèse — l'élite qui gouverne amplifie son éthos.
 */
#include "scps_factions.h"
#include "scps_readout.h"   /* K2 : faction_name vit au readout */
#include <stdio.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, int cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}

static PopCulture cult(Ethos e, Heritage heritage, Credo credo){
    PopCulture c; memset(&c,0,sizeof c);
    c.langue=5; c.valeurs=5; c.subsistance=5; c.parente=5; c.religion=5;
    c.ethos=e; c.heritage=heritage; c.credo=credo; c.rel_branch=REL_ABRAHAMIQUE; c.settled=true;
    return c;
}
static PopGroup grp(PopCulture cu, SocialClass k, long n){
    PopGroup g; memset(&g,0,sizeof g);
    g.heritage=cu.heritage; g.origin=cu; g.culture=cu; g.klass=k; g.count=n; g.integration=1.f;
    return g;
}
static int argmax(const float w[FAC_COUNT]){
    int b=0; for (int f=1;f<FAC_COUNT;f++) if (w[f]>w[b]) b=f; return b;
}

int main(void){
    printf("══════════════════════════════════════════════════════════════\n");
    printf(" FACTIONS PAR ÉTHOS (passe 1) — le spectre + l'enracinement\n");
    printf("══════════════════════════════════════════════════════════════\n");

    /* ═══ 1. LE SPECTRE — six factions = les axes IA (+ Communautaire) ═══ */
    printf("\n── 1. Le spectre : six factions-éthos (les axes des poids IA) ──\n");
    ok("six factions exactement", FAC_COUNT==6);
    printf("   ");
    for (int f=0; f<FAC_COUNT; f++) printf("%s%s", faction_name(f), f<FAC_COUNT-1?" · ":"\n");
    ok("chaque faction a un mot diégétique",
       faction_name(FAC_CONQUERANT)[0] && faction_name(FAC_COMMUNAUTAIRE)[0]
       && faction_name(FAC_TRANSGRESSEUR)[0]);

    /* ═══ 2. LE PENCHANT D'UN GROUPE — éthos / heritage / credo ═══════════════ */
    printf("\n── 2. Le penchant d'un groupe vient de sa culture ──\n");
    {
        float w[FAC_COUNT];
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_DOMINATEUR,.heritage=HERITAGE_CLANIQUE,.credo=CREDO_PLURALISTE}, w);
        ok("clanique dominateur → Conquérant dominant, Transgresseur fort",
           argmax(w)==FAC_CONQUERANT && w[FAC_TRANSGRESSEUR]>w[FAC_MARCHAND] && w[FAC_TRANSGRESSEUR]>0.2f);
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_MERCANTILE,.heritage=HERITAGE_AGRAIRE,.credo=CREDO_PLURALISTE}, w);
        ok("agraire mercantile → Marchand dominant, Communautaire présent",
           argmax(w)==FAC_MARCHAND && w[FAC_COMMUNAUTAIRE]>0.1f);
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_BUREAUCRATE,.heritage=HERITAGE_ADAPTATIF,.credo=CREDO_PLURALISTE}, w);
        ok("adaptatif bureaucrate → Légiste dominant", argmax(w)==FAC_LEGISTE);
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_PACIFISTE,.heritage=HERITAGE_MECANISTE,.credo=CREDO_PLURALISTE}, w);
        ok("mécaniste pacifiste → Communautaire dominant", argmax(w)==FAC_COMMUNAUTAIRE);
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_ORDRE,.heritage=HERITAGE_METALLURGISTE,.credo=CREDO_PLURALISTE}, w);
        /* ÉQUILIBRAGE 2026-07-10 : signature Métallurgiste normalisée (Transgresseur
         * 0.4→0.3) recale le résultat exactement à 0.15 normalisé — seuil abaissé à
         * 0.12 pour rester robuste à l'arrondi flottant, intention inchangée
         * (« Transgresseur présent, non dominant »). */
        ok("métallurgiste d'ordre → Transgresseur présent (forge à runes)", w[FAC_TRANSGRESSEUR]>0.12f);
        /* CREDO : la ferveur nourrit les Gardiens. */
        float wp[FAC_COUNT], wz[FAC_COUNT];
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_ORDRE,.heritage=HERITAGE_ADAPTATIF,.credo=CREDO_PLURALISTE}, wp);
        group_ethos_lean(&(PopCulture){.ethos=ETHOS_ORDRE,.heritage=HERITAGE_ADAPTATIF,.credo=CREDO_PURIFICATEUR}, wz);
        ok("le credo purificateur RENFORCE les Gardiens (vs pluraliste)",
           wz[FAC_GARDIEN] > wp[FAC_GARDIEN] + 0.1f);
        /* somme normalisée. */
        float s=0; for (int f=0;f<FAC_COUNT;f++) s+=wz[f];
        ok("le penchant est un profil normalisé (Σ≈1)", s>0.98f && s<1.02f);
    }

    /* ═══ 3. ENRACINEMENT — la distribution d'un pays = Σ groupes ═════════ */
    printf("\n── 3. Enracinement : conquérir un peuple clanique MONTE les Conquérants ──\n");
    {
        /* Un pays marchand (agraires). */
        ProvincePop prov; memset(&prov,0,sizeof prov);
        prov.groups[0]=grp(cult(ETHOS_MERCANTILE,HERITAGE_AGRAIRE,CREDO_PLURALISTE),CLASS_LABORER,1000);
        prov.n_groups=1;
        float before[FAC_COUNT]; EthosFaction dom0=faction_weights_of(&prov,1,before);
        /* CONQUÊTE : une province clanique entre dans le pays. */
        ProvincePop conq; memset(&conq,0,sizeof conq);
        conq.groups[0]=grp(cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE),CLASS_LABORER,600);
        conq.n_groups=1;
        ProvincePop country[2]={prov,conq};
        float after[FAC_COUNT]; EthosFaction dom1=faction_weights_of(country,2,after);
        printf("   avant : dominante %s (Conquérants %.0f%%) → après conquête clanique : %s (Conquérants %.0f%%)\n",
               faction_name(dom0), before[FAC_CONQUERANT]*100, faction_name(dom1), after[FAC_CONQUERANT]*100);
        ok("le pays marchand est d'abord dominé par les Marchands", dom0==FAC_MARCHAND);
        ok("avaler une province clanique MONTE les Conquérants (le spectre se reconfigure)",
           after[FAC_CONQUERANT] > before[FAC_CONQUERANT] + 0.1f);
        ok("la somme reste un profil (Σ≈1)",
           after[0]+after[1]+after[2]+after[3]+after[4]+after[5] > 0.98f);
    }

    /* ═══ 4. LA CLASSE PÈSE — l'élite qui gouverne amplifie son éthos ═════ */
    printf("\n── 4. Qui gouverne compte : l'élite pèse plus que la masse ──\n");
    {
        PopCulture agr=cult(ETHOS_MERCANTILE,HERITAGE_AGRAIRE,CREDO_PLURALISTE);
        PopCulture clan =cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE);
        /* Même masse clanique, une fois LABOUREURS, une fois ÉLITE régnante. */
        ProvincePop asLab; memset(&asLab,0,sizeof asLab);
        asLab.groups[0]=grp(agr,CLASS_LABORER,1000); asLab.groups[1]=grp(clan,CLASS_LABORER,200); asLab.n_groups=2;
        ProvincePop asElite; memset(&asElite,0,sizeof asElite);
        asElite.groups[0]=grp(agr,CLASS_LABORER,1000); asElite.groups[1]=grp(clan,CLASS_ELITE,200); asElite.n_groups=2;
        float wl[FAC_COUNT], we[FAC_COUNT];
        faction_weights_of(&asLab,1,wl); faction_weights_of(&asElite,1,we);
        printf("   clanique laboureur : Conquérants %.0f%% | clanique ÉLITE régnante : Conquérants %.0f%%\n",
               wl[FAC_CONQUERANT]*100, we[FAC_CONQUERANT]*100);
        ok("la même minorité clanique pèse PLUS comme élite que comme masse (qui gouverne compte)",
           we[FAC_CONQUERANT] > wl[FAC_CONQUERANT] + 0.05f);
    }

    /* ═══ 5. L'ÉTHOS EFFECTIF (§3) — la résultante que le moteur lit ══════ */
    printf("\n── 5. L'éthos effectif : la dominante fixe les poids w_* ──\n");
    {
        /* Un pays conquérant (claniques) vs un pays marchand (agraires). */
        float wc[FAC_COUNT], wm[FAC_COUNT];
        ProvincePop pc; memset(&pc,0,sizeof pc);
        pc.groups[0]=grp(cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE),CLASS_ELITE,300);
        pc.groups[1]=grp(cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE),CLASS_LABORER,800); pc.n_groups=2;
        ProvincePop pm; memset(&pm,0,sizeof pm);
        pm.groups[0]=grp(cult(ETHOS_MERCANTILE,HERITAGE_AGRAIRE,CREDO_PLURALISTE),CLASS_LABORER,1000); pm.n_groups=1;
        faction_weights_of(&pc,1,wc); faction_weights_of(&pm,1,wm);
        EthosWeights ec=faction_effective_weights(wc), em=faction_effective_weights(wm);
        printf("   conquérant : w_expand=%.2f w_trade=%.2f | marchand : w_expand=%.2f w_trade=%.2f\n",
               ec.w_expand, ec.w_trade, em.w_expand, em.w_trade);
        ok("le pays conquérant a un w_expand effectif PLUS fort que le marchand", ec.w_expand > em.w_expand);
        ok("le pays marchand a un w_trade effectif PLUS fort que le conquérant", em.w_trade > ec.w_trade);
    }

    /* ═══ 6. COHÉSION vs FRACTURE DE VALEURS (§6) — le frein interne ══════ */
    printf("\n── 6. Fracture de valeurs : un empire 45/45 est paralysé, un mono-éthos cohésif ──\n");
    {
        /* Mono-éthos : un seul peuple → cohésion (fracture basse). */
        ProvincePop mono; memset(&mono,0,sizeof mono);
        mono.groups[0]=grp(cult(ETHOS_BUREAUCRATE,HERITAGE_ADAPTATIF,CREDO_PLURALISTE),CLASS_LABORER,1000); mono.n_groups=1;
        float wmono[FAC_COUNT]; faction_weights_of(&mono,1,wmono);
        /* Deux blocs forts et opposés (Conquérants ~ Marchands) → fracture. */
        ProvincePop split; memset(&split,0,sizeof split);
        split.groups[0]=grp(cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE),CLASS_LABORER,1000);
        split.groups[1]=grp(cult(ETHOS_MERCANTILE,HERITAGE_AGRAIRE,CREDO_PLURALISTE),CLASS_LABORER,1000);
        split.n_groups=2;
        float wsplit[FAC_COUNT]; faction_weights_of(&split,1,wsplit);
        printf("   mono-éthos : fracture %.2f (cohésion %.2f) | bloc 50/50 opposé : fracture %.2f\n",
               faction_fracture(wmono), faction_cohesion(wmono), faction_fracture(wsplit));
        ok("un mono-éthos est COHÉSIF (fracture basse)", faction_fracture(wmono) < 0.40f);
        ok("deux factions fortes et opposées DÉCHIRENT (fracture nettement plus haute)",
           faction_fracture(wsplit) > 0.45f && faction_fracture(wsplit) > faction_fracture(wmono) + 0.15f);
        ok("fracture = 1 − cohésion", faction_fracture(wsplit) + faction_cohesion(wsplit) > 0.99f
                                   && faction_fracture(wsplit) + faction_cohesion(wsplit) < 1.01f);
    }

    /* ═══ 7. TENSION DE COUP (§5) — une faction forte aliénée vise le trône ══ */
    printf("\n── 7. Tension de coup : une faction forte OPPOSÉE à la direction couve un coup ──\n");
    {
        ok("l'opposition Gardiens↔Transgresseurs est maximale (l'épine dorsale faustienne)",
           faction_opposition(FAC_GARDIEN,FAC_TRANSGRESSEUR) > 0.95f);
        ok("une faction ne s'oppose pas à elle-même", faction_opposition(FAC_MARCHAND,FAC_MARCHAND)==0.f);
        /* Régime marchand (dominante) avec un bloc CONQUÉRANT fort et opposé. */
        ProvincePop pp; memset(&pp,0,sizeof pp);
        pp.groups[0]=grp(cult(ETHOS_MERCANTILE,HERITAGE_AGRAIRE,CREDO_PLURALISTE),CLASS_LABORER,1100);
        pp.groups[1]=grp(cult(ETHOS_DOMINATEUR,HERITAGE_CLANIQUE,CREDO_PLURALISTE),CLASS_ELITE,300);
        pp.n_groups=2;
        float w[FAC_COUNT]; faction_weights_of(&pp,1,w);
        EthosFaction alien; float tension=faction_coup_tension(w,&alien);
        printf("   régime marchand + élite clanique conquérante : tension de coup %.2f (faction aliénée : %s)\n",
               tension, faction_name(alien));
        ok("un bloc fort et opposé à la direction crée une TENSION de coup", tension > 0.15f);
        /* Mono-éthos : nulle opposition forte → quasi nulle tension. */
        ProvincePop mono; memset(&mono,0,sizeof mono);
        mono.groups[0]=grp(cult(ETHOS_BUREAUCRATE,HERITAGE_ADAPTATIF,CREDO_PLURALISTE),CLASS_LABORER,1000);
        mono.n_groups=1;
        float wm[FAC_COUNT]; faction_weights_of(&mono,1,wm);
        ok("un mono-éthos ne couve aucun coup (nulle faction forte opposée)",
           faction_coup_tension(wm,&alien) < tension - 0.10f);
    }

    /* ═══ 8. LES LEVIERS COMME DES VOTES (§4) ═══════════════════════════ */
    printf("\n── 8. Un levier AVANCE un éthos et AIGRIT les opposés (un vote) ──\n");
    {
        faction_levers_reset();
        ok("au départ, nulle rancœur", faction_grievance(0, FAC_MARCHAND)==0.f);
        faction_lever_apply(0, FAC_GARDIEN, 0.4f);   /* foi imposée → Gardiens */
        ok("imposer la foi (Gardiens) AIGRIT les Marchands (opposés)",
           faction_grievance(0,FAC_MARCHAND) > 0.2f);
        ok("… et n'aigrit PAS la faction avancée elle-même", faction_grievance(0,FAC_GARDIEN)==0.f);
        faction_lever_apply(1, FAC_TRANSGRESSEUR, 0.4f);  /* forge à runes → Transgresseurs */
        ok("la forge à runes (Transgresseurs) aigrit les Communautaires",
           faction_grievance(1,FAC_COMMUNAUTAIRE) > 0.2f);
        float g0=faction_grievance(0,FAC_MARCHAND);
        faction_levers_decay(0.5f);
        ok("une stance non entretenue S'EFFACE (la rancœur retombe)",
           faction_grievance(0,FAC_MARCHAND) < g0 - 0.05f);
        faction_levers_reset();
        ok("reset remet tout à zéro (début de sim)",
           faction_grievance(0,FAC_MARCHAND)==0.f && faction_grievance(1,FAC_COMMUNAUTAIRE)==0.f);
    }

    /* ⚠ SUPPRIMÉ (raccord 8, Âges sans ordre imposé, 2026-07-11) : « §9. ENGAGEMENT
     * D'ÂGE » testait age_patron()/faction_age_engage() (un vote de faction MONDIAL
     * + un bonus de satisfaction, tous deux supprimés — cf. scps_factions.c). Les
     * leviers d'âge SCOPÉS aux pays matériellement concernés vivent désormais dans
     * scps_events.c (age_lever_exchange/_discovery/_empires/_breach/_lumieres/
     * _soulevements/_tyrans) — testés dans events_demo.c/structural_demo.c, qui
     * ont accès au module events (ce banc-ci n'exerce QUE scps_factions.c). */

    /* ═══ I5 — L'AUDIT DES OFFICES : réprimer la capture (corr −20) ══════ */
    printf("\n── I5. L'audit des offices : l'État réprime la capture (corruption −20) ──\n");
    {
        faction_levers_reset();
        int cid=3;
        for (int k=0;k<13;k++) faction_concede(cid, FAC_MARCHAND);   /* les Marchands gorgent l'État */
        int corr0=faction_corruption_0_100(cid);
        ok("la capture répétée CORROMPT l'État (corruption > 50)", corr0>50);
        int ret=faction_audit(cid);
        int corr1=faction_corruption_0_100(cid);
        printf("   corruption %d → audit → %d (rendu AVANT = %d)\n", corr0, corr1, ret);
        ok("l'audit rend la corruption AVANT et la RÉPRIME (~−20 points)",
           ret==corr0 && corr1<=corr0-18 && corr1<corr0);
        ok("l'audit n'efface pas tout d'un coup (la capture résiduelle demeure)", corr1>0);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    return g_fail?1:0;
}
