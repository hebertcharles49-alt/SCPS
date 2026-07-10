/*
 * statecraft_demo.c — banc d'essai des MÉTRIQUES & du STATECRAFT
 *
 *   make statecraft_demo && ./statecraft_demo [graine]
 *
 * Prouve l'évolution de la membrane (mots → mots + NOMBRES 0-100 à effets) et
 * les trois systèmes ancrés :
 *   1. Métriques 0-100 + effets LUS (prod ×1.15 à 100, agitation −2 à 100) —
 *      jamais un flottant SCPS ni son nom.
 *   2. Influence : trahir la fait chuter, tenir ses accords la monte, elle
 *      plafonne le vivier de diplomates.
 *   3. Diplomates : « Intégrer une province » occupe un agent ∝ D∞ et accélère
 *      la Légitimité ; le vivier est limité (plafond d'Influence).
 *   4. Opinion (−100..100) : une mission la monte, la guerre la crève.
 *   5. Agitation/révolte : une province à agitation SOUTENUE bascule ; la
 *      garnison (H) et la stabilité l'apaisent.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_trade.h"
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"
#include "scps_diplo.h"
#include "scps_routes.h"
#include "scps_statecraft.h"
#include "scps_tune.h"      /* P1-1/P3 : COUNCIL_* (registre J) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass=0, g_fail=0;
static void ok(const char *what, bool cond){
    printf("   %s %s\n", cond?"✓":"✗", what);
    if (cond) g_pass++; else g_fail++;
}
static bool near_f(float a, float b, float eps){ float d=a-b; if(d<0)d=-d; return d<=eps; }

typedef struct {
    World *w; WorldEconomy *econ; TradeNetwork *net; TechState *ts;
    WorldProsperity *wp; WorldLegitimacy *wl; DiploState *dp; RouteNetwork *rn;
    Statecraft *sc;
} Sim;

static int cap_region(const World *w, int cid){
    int cp=(cid>=0&&cid<w->n_countries)?w->country[cid].capital_prov:-1;
    return (cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
}

int main(int argc, char **argv){
    uint32_t seed=(argc>1)?(uint32_t)strtoul(argv[1],NULL,10):42u;
    Sim s={0};
    s.w=malloc(sizeof(World)); s.econ=malloc(sizeof(WorldEconomy));
    s.net=malloc(sizeof(TradeNetwork)); s.ts=calloc(SCPS_MAX_COUNTRY,sizeof(TechState));
    s.wp=malloc(sizeof(WorldProsperity)); s.wl=malloc(sizeof(WorldLegitimacy));
    s.dp=malloc(sizeof(DiploState)); s.rn=malloc(sizeof(RouteNetwork)); s.sc=malloc(sizeof(Statecraft));
    if(!s.w||!s.econ||!s.net||!s.ts||!s.wp||!s.wl||!s.dp||!s.rn||!s.sc){ fprintf(stderr,"OOM\n"); return 1; }

    printf("══════════════════════════════════════════════════════════════\n");
    printf(" STATECRAFT — métriques 0-100, Influence, Diplomates, Révolte (graine %u)\n", seed);
    printf("══════════════════════════════════════════════════════════════\n");

    WorldParams p=worldparams_default(seed);
    world_generate(s.w,&p);
    econ_init(s.econ,s.w); gen_population(s.w,s.econ); worldgen_seed_peoples(s.w,s.econ,HERITAGE_ADAPTATIF);
    trade_network_build(s.net,s.w,s.econ);
    for (int c=0;c<s.w->n_countries;c++) tech_state_init(&s.ts[c],false);
    prosperity_init(s.wp,s.w); legitimacy_init(s.wl,s.w,s.econ);
    diplo_init(s.dp); routes_init(s.rn); statecraft_init(s.sc,s.w);
    for (int t=0;t<25;t++){                 /* échauffe la sim (25 ans) */
        econ_tick(s.econ, 1.f); econ_colonize_tick(s.econ,s.w,-1,NULL,NULL); world_tick(s.w,s.econ,1.f);
        legitimacy_tick(s.wl,s.w,s.econ,s.ts);
        prosperity_tick(s.wp,s.w,s.econ,s.net,s.ts,s.wl);
    }
    int player=0; for (int c=0;c<s.w->n_countries;c++) if (s.w->country[c].role==POLITY_PLAYER){player=c;break;}

    /* ═══ 1. MÉTRIQUES 0-100 + EFFETS LUS ═══════════════════════════════ */
    printf("\n── 1. La membrane sort un NOMBRE et un MOT (jamais un flottant SCPS) ──\n");
    CountryReadout r=country_readout(s.wp,s.ts,s.w,player);
    r.influence=statecraft_influence(s.sc,player);
    printf("   Stabilité %3d — %-12s   Prospérité %3d — %-10s   Légitimité %3d — %s\n",
           r.m_stabilite.value, r.m_stabilite.word, r.m_prosperite.value, r.m_prosperite.word,
           r.m_legitimite.value, r.m_legitimite.word);
    printf("   Cohésion  %3d — %-12s   Savoir     %3d — %-10s   Influence  %3d\n",
           r.m_cohesion.value, r.m_cohesion.word, r.m_savoir.value, r.m_savoir.word, r.influence);
    ok("toutes les métriques sont bornées 0-100",
       r.m_stabilite.value>=0 && r.m_stabilite.value<=100 && r.m_prosperite.value>=0 &&
       r.m_prosperite.value<=100 && r.m_cohesion.value>=0 && r.m_cohesion.value<=100);
    ok("chaque métrique porte un mot (la bande coexiste avec le nombre)",
       r.m_stabilite.word[0] && r.m_prosperite.word[0] && r.m_legitimite.word[0]);

    printf("\n── Les effets sont des COURBES LUES (l'effet existant des coordonnées) ──\n");
    ok("Prospérité 100 → production ×1.15 (l'effet de prospérité, surfacé)",
       near_f(prod_multiplier(100), 1.15f, 0.001f));
    ok("Prospérité 0 → production ×0.85",  near_f(prod_multiplier(0), 0.85f, 0.001f));
    ok("Prospérité 50 → production ×1.00 (neutre)", near_f(prod_multiplier(50), 1.00f, 0.001f));
    ok("Stabilité 100 → agitation −2",     near_f(agitation_modifier(100), -2.0f, 0.001f));
    ok("Stabilité sous le seuil → réforme gâtée", !can_enact_reform(STAB_REFORM_MIN-1) && can_enact_reform(STAB_REFORM_MIN));
    ok("la métrique croît avec la coordonnée (monotone)",
       metric_prosperity(8.f) > metric_prosperity(3.f) && metric_legitimacy(9.f) > metric_legitimacy(2.f));

    /* ═══ 2. INFLUENCE ══════════════════════════════════════════════════ */
    printf("\n── 2. Influence : la réputation suit l'acte ──\n");
    statecraft_init(s.sc,s.w);
    int inf0=statecraft_influence(s.sc,player);
    int cap0=statecraft_missions_cap(s.sc,player);
    for (int k=0;k<4;k++) statecraft_on_accord_kept(s.sc,player);
    int inf_acc=statecraft_influence(s.sc,player);
    int cap_acc=statecraft_missions_cap(s.sc,player);
    statecraft_on_betrayal(s.sc,player);
    int inf_bet=statecraft_influence(s.sc,player);
    printf("   Influence : départ %d → accords tenus %d → trahison %d   (plafond missions %d→%d)\n",
           inf0, inf_acc, inf_bet, cap0, cap_acc);
    ok("tenir ses accords monte l'Influence",  inf_acc > inf0);
    ok("trahir un allié fait chuter l'Influence", inf_bet < inf_acc);
    ok("une Influence plus haute élargit le vivier (plafond de missions)", cap_acc >= cap0);

    /* ═══ 3. DIPLOMATES ═════════════════════════════════════════════════ */
    printf("\n── 3. Diplomates : un vivier limité, des missions en JOURS ──\n");
    statecraft_init(s.sc,s.w);
    const PopCulture *rul=&s.econ->region[cap_region(s.w,player)].culture;
    /* Deux régions test : l'une à culture PROCHE du trône, l'autre LOINTAINE. */
    int rClose=-1, rFar=-1, home=cap_region(s.w,player);
    for (int rr=0;rr<s.econ->n_regions;rr++){
        if (rr==home || !s.econ->region[rr].culture.settled) continue;
        if (rClose<0) rClose=rr; else if (rFar<0){ rFar=rr; break; }
    }
    if (rClose>=0){ s.econ->region[rClose].culture=*rul; }               /* D∞ ≈ 0 */
    if (rFar>=0){ s.econ->region[rFar].culture=*rul;
        s.econ->region[rFar].culture.valeurs = (rul->valeurs<5.f)?rul->valeurs+9.f:rul->valeurs-9.f; }
    bool okClose=statecraft_send(s.sc,s.w,s.econ,player,DIP_INTEGRATE,rClose);
    bool okFar  =statecraft_send(s.sc,s.w,s.econ,player,DIP_INTEGRATE,rFar);
    int dClose=s.sc->staff[player].agents[0].days_left;
    int dFar  =s.sc->staff[player].agents[1].days_left;
    printf("   « Intégrer » : province proche %d j, province lointaine %d j (∝ D∞)\n", dClose, dFar);
    ok("les deux missions occupent un agent chacune", okClose && okFar && statecraft_missions_active(s.sc,player)==2);
    ok("intégrer du LOINTAIN prend bien plus de jours (∝ D∞)", dFar > dClose + 300);

    /* Vivier limité : on remplit jusqu'au plafond, le suivant échoue. */
    int cap=statecraft_missions_cap(s.sc,player);
    int fails=0;
    for (int k=0;k<cap+3;k++){
        int tgt=(player+1)%s.w->n_countries;
        if (!statecraft_send(s.sc,s.w,s.econ,player,DIP_RELATIONS,tgt)) fails++;
    }
    printf("   Vivier : plafond %d, missions actives %d, refus %d\n",
           cap, statecraft_missions_active(s.sc,player), fails);
    ok("le vivier sature au plafond (les missions en trop sont refusées)",
       statecraft_missions_active(s.sc,player)==cap && fails>0);

    /* L'intégration proche aboutit et fait BONDIR la Légitimité de la région. */
    statecraft_init(s.sc,s.w);
    float yh_before=s.wl->years_held[rClose], L_before=s.wl->L[rClose];
    statecraft_send(s.sc,s.w,s.econ,player,DIP_INTEGRATE,rClose);
    for (int d=0; d<800 && s.sc->staff[player].agents[0].mission!=DIP_IDLE; d+=10)
        statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
    printf("   Intégration achevée : région %d, ancienneté %.0f→%.0f, L %.1f→%.1f\n",
           rClose, yh_before, s.wl->years_held[rClose], L_before, s.wl->L[rClose]);
    ok("la mission d'intégration accélère la Légitimité (ancienneté + L montent)",
       s.wl->years_held[rClose] > yh_before && s.wl->L[rClose] >= L_before);

    /* Une route s'ouvre après ~90 jours d'occupation d'un agent. */
    statecraft_init(s.sc,s.w);
    int partner=-1;
    for (int rr=0;rr<s.econ->n_regions;rr++)
        if (rr!=home && s.econ->region[rr].culture.settled && s.econ->region[rr].owner!=player){ partner=rr; break; }
    if (partner<0) partner=rFar;
    int routes_before=routes_count_for_region(s.rn,home);
    statecraft_send(s.sc,s.w,s.econ,player,DIP_ROUTE,partner);
    /* L'agent ouvre la route (90 j) ; le module routes la fait MÛRIR (90 j de
     * plus) — les deux systèmes avancent ensemble, comme dans la boucle réelle. */
    for (int d=0; d<220; d+=10){
        statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
        routes_advance(s.rn,s.w,s.econ,10);
    }
    ok("« Établir une route » occupe un agent puis ouvre une route",
       routes_count_for_region(s.rn,home) > routes_before);

    /* ═══ 4. OPINION (−100..100) ════════════════════════════════════════ */
    printf("\n── 4. Opinion : une mission la monte, la guerre la crève ──\n");
    statecraft_init(s.sc,s.w);
    int other=-1;
    for (int c=0;c<s.w->n_countries;c++) if (c!=player && s.w->country[c].role!=POLITY_UNCLAIMED){ other=c; break; }
    if (other<0) other=(player+1)%s.w->n_countries;
    int op0=statecraft_opinion(s.sc,player,other);
    statecraft_send(s.sc,s.w,s.econ,player,DIP_RELATIONS,other);
    for (int d=0; d<200; d+=10) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
    int op_rel=statecraft_opinion(s.sc,player,other);
    diplo_declare_war(s.dp,player,other);
    for (int d=0; d<400; d+=10) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
    int op_war=statecraft_opinion(s.sc,player,other);
    printf("   Opinion envers le voisin : départ %d → après mission %d → après guerre %d\n",
           op0, op_rel, op_war);
    ok("améliorer les relations monte l'opinion", op_rel > op0);
    ok("la guerre crève l'opinion", op_war < op_rel);

    /* ═══ 5. AGITATION — un SIGNAL soutenu ; H + Stabilité l'apaisent ═══════
     * Dédup Option B (2026-07-04) : statecraft ne fait plus fire de révolte
     * lui-même — scps_revolt.c (la révolte incarnée, groupe par groupe) est
     * le SEUL acteur, et lit CE signal (statecraft_agitation) comme un grief
     * politique de plus. Ce banc prouve donc que le SIGNAL franchit/ne
     * franchit pas le seuil selon la province (le mécanisme marche), plus
     * qu'un allumage indépendant (statecraft_revolt_fired reste INERTE,
     * toujours faux — testé explicitement ci-dessous). */
    printf("\n── 5. Agitation soutenue (SIGNAL) ; H + Stabilité l'apaisent ──\n");
    /* Unité : la garnison et la stabilité abattent l'agitation. */
    int agit_nu  = metric_agitation(/*L*/1.f,/*coerc*/0.6f,/*div*/8.f,/*shock*/1.f,/*stab*/40,/*H*/0.f);
    int agit_gar = metric_agitation(1.f, 0.6f, 8.f, 1.f, /*stab*/90, /*H*/8.f);
    printf("   Province L basse, lignée étrangère : nue=%d  garnisonnée+stable=%d\n", agit_nu, agit_gar);
    ok("sans garnison ni stabilité, l'agitation franchit le seuil de révolte", agit_nu >= AGIT_REVOLT_SEUIL);
    ok("garnison (H) + stabilité ramènent l'agitation sous le seuil", agit_gar < AGIT_REVOLT_SEUIL);

    /* Intégration : une province à L basse, étrangère, sans garnison voit son
     * AGITATION monter et TENIR au-dessus du seuil (le signal que scps_revolt.c
     * viendra lire) — statecraft, lui, ne bascule plus jamais indépendamment. */
    statecraft_init(s.sc,s.w);
    int rRevolt=rFar>=0?rFar:rClose;
    s.econ->region[rRevolt].owner=(int16_t)player; s.econ->region[rRevolt].culture.settled=true;
    s.econ->region[rRevolt].culture.valeurs=(rul->valeurs<5.f)?rul->valeurs+9.f:rul->valeurs-9.f;
    s.econ->region[rRevolt].build.H_coerc=0.f; s.econ->region[rRevolt].coercion=1.0f;
    /* Conquête FRAÎCHE, zéro légitimité (déterministe) : la frondeuse franchit
     * le seuil quelle que soit la géographie — on teste le SIGNAL, pas le monde. */
    if (rRevolt<SCPS_MAX_REG){ s.wl->L[rRevolt]=0.0f; s.wl->years_held[rRevolt]=0.f; }
    bool crossed=false; int day_crossed=-1; bool fired_indep=false;
    for (int d=0; d<800; d+=10){
        statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
        if (statecraft_revolt_fired(s.sc,rRevolt)) fired_indep=true;      /* doit rester TOUJOURS faux */
        if (!crossed && statecraft_agitation(s.sc,rRevolt) >= AGIT_REVOLT_SEUIL){ crossed=true; day_crossed=d; }
    }
    printf("   Province frondeuse (L≈1, étrangère, sans garnison) : agitation %d → seuil franchi au jour %d\n",
           statecraft_agitation(s.sc,rRevolt), day_crossed);
    ok("une misère soutenue fait MONTER l'agitation au-dessus du seuil (le SIGNAL marche)", crossed);
    ok("statecraft NE FIRE JAMAIS de révolte indépendamment (dédup Option B)", !fired_indep);

    /* Une province garnisonnée et légitime voit son agitation rester SOUS le seuil. */
    statecraft_init(s.sc,s.w);
    int rCalm=rClose;
    s.econ->region[rCalm].owner=(int16_t)player; s.econ->region[rCalm].culture=*rul;
    s.econ->region[rCalm].culture.settled=true;
    s.econ->region[rCalm].build.H_coerc=6.f; s.econ->region[rCalm].coercion=0.f;
    if (rCalm<SCPS_MAX_REG){ s.wl->L[rCalm]=8.f; s.wl->years_held[rCalm]=60.f; }
    bool calm_crossed=false;
    for (int d=0; d<600; d+=10){
        statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,10);
        if (statecraft_agitation(s.sc,rCalm) >= AGIT_REVOLT_SEUIL) calm_crossed=true;
    }
    printf("   Province loyale (L=8, du même sang, garnisonnée) : agitation %d → seuil franchi=%s\n",
           statecraft_agitation(s.sc,rCalm), calm_crossed?"OUI":"non");
    ok("une province légitime et garnisonnée reste SOUS le seuil (aucun grief à lire)", !calm_crossed);

    /* ── Q1 — LE CONSEIL : nommer monte le multiplicateur ET coûte ; renvoyer rétablit ── */
    {
        statecraft_init(s.sc, s.w);
        int cid=0, seat=0;                              /* siège Savoir */
        ok("Conseil : siège vacant → multiplicateur NEUTRE (1.0)",
           statecraft_council_seat_mult(s.sc,seed,cid,seat)==1.f);
        int best=0, bt=0;                               /* nomme le candidat de plus haut tier (pool gen 0) */
        for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seat,sl,0); if(t>bt){bt=t;best=sl;} }
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        ok("Conseil : NOMMER monte le multiplicateur (>1) et pourvoit le siège",
           statecraft_council_seat_mult(s.sc,seed,cid,seat)>1.f && statecraft_council_seated(s.sc,cid,seat)==best);
        ok("Conseil : le conseiller a un COÛT mensuel (>0, ×IPM)",
           statecraft_council_cost(s.sc,seed,cid,1.f)>0.f);
        ok("Conseil : candidats DÉTERMINISTES (même seed → même tier)",
           statecraft_council_cand_tier(seed,cid,seat,best,0)==bt);
        /* ── L'ÂGE : dérivé (base 30-51 + années écoulées), il GRANDIT avec l'année ── */
        int a0 = statecraft_council_cand_age(seed,cid,seat,best,0,0);
        ok("Conseil : l'âge de départ est humain (30-51 ans)", a0>=30 && a0<=51);
        ok("Conseil : l'âge GRANDIT avec l'année (+7 ans à l'an 7)",
           statecraft_council_cand_age(seed,cid,seat,best,0,7)==a0+7);
        ok("Conseil : le ministre ASSIS vieillit aussi (lecture seated_age)",
           statecraft_council_seated_age(s.sc,seed,cid,seat,10)==a0+10);
        /* ── LA RETRAITE vide le siège (66-73 ans) — impossible < an 16 (golden intact) ── */
        statecraft_council_age_tick(s.sc, seed, 12);
        ok("Conseil : AUCUNE retraite dans la fenêtre golden (an 12)",
           statecraft_council_seated(s.sc,cid,seat)==best);
        statecraft_council_age_tick(s.sc, seed, 100);
        ok("Conseil : passé l'âge, la RETRAITE vide le siège (an 100)",
           statecraft_council_seated(s.sc,cid,seat)<0);
        /* ── LA POOL TOURNE par génération et reste TOUJOURS DISPO (3 candidats valides) ── */
        ok("Conseil : génération 1 à l'an 25 (rotation de pool)",
           statecraft_council_gen(25)==1 && statecraft_council_gen(12)==0);
        bool pool_ok=true;
        for (int g=0; g<5 && pool_ok; g++)
            for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
                int t=statecraft_council_cand_tier(seed,cid,seat,sl,g);
                int ag=statecraft_council_cand_age(seed,cid,seat,sl,g,g*SC_COUNCIL_GEN_YEARS);
                if (t<1||t>3||ag<30||ag>51) pool_ok=false;
            }
        ok("Conseil : la pool est DISPO à toute génération (tiers 1-3, âges 30-51 au début de gen)", pool_ok);
        statecraft_council_dismiss(s.sc, seed, cid, seat);
        ok("Conseil : RENVOYER rétablit le neutre (1.0) sans coût",
           statecraft_council_seat_mult(s.sc,seed,cid,seat)==1.f
           && statecraft_council_seated(s.sc,cid,seat)<0 && statecraft_council_cost(s.sc,seed,cid,1.f)==0.f);
    }

    /* ── V2a — LE CONSEIL VIVANT : faction, loyauté, paie ──────────────────── */
    printf("\n── V2a : le Conseil vivant (faction, loyauté, paie) ──\n");
    {
        /* (1) P0-1 — SIX FACTIONS SUR TROIS SIÈGES : plus de spectre par siège — un
         * mélange DÉTERMINISTE des 6 factions par (siège, génération) ; les 3
         * candidats d'UN siège sont TOUJOURS 3 factions DISTINCTES (préfixe d'une
         * permutation) ; re-tirage à chaque génération. */
        int cid=0;
        bool det_ok=true, distinct_ok=true;
        for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
            EthosFaction seen[SC_COUNCIL_CANDS];
            for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
                EthosFaction f1=statecraft_council_faction(seed,cid,seat,sl,0);
                EthosFaction f2=statecraft_council_faction(seed,cid,seat,sl,0);
                if (f1!=f2) det_ok=false;
                seen[sl]=f1;
            }
            for (int a=0;a<SC_COUNCIL_CANDS;a++) for (int bb=a+1;bb<SC_COUNCIL_CANDS;bb++)
                if (seen[a]==seen[bb]) distinct_ok=false;
        }
        ok("Conseil vivant : attribution DÉTERMINISTE (même seed → même faction)", det_ok);
        ok("Conseil vivant : les 3 candidats d'un siège sont 3 factions DISTINCTES (P0-1, plus de spectre)", distinct_ok);
        /* Preuve que la restriction a VRAIMENT disparu : sur un échantillon de pays/
         * générations, chaque siège voit AU MOINS 4 factions différentes apparaître
         * (l'ancien code en plafonnait 2 par siège). */
        { int seen_mask[SC_COUNCIL_SEATS]={0,0,0};
          for (int seat=0; seat<SC_COUNCIL_SEATS; seat++)
              for (int c2=0;c2<12;c2++) for (int g2=0;g2<6;g2++) for (int sl=0; sl<SC_COUNCIL_CANDS; sl++)
                  seen_mask[seat] |= 1<<(int)statecraft_council_faction(seed,c2,seat,sl,g2);
          int popcount[SC_COUNCIL_SEATS];
          for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
              int n=0; for (int f=0;f<FAC_COUNT;f++) if (seen_mask[seat]&(1<<f)) n++;
              popcount[seat]=n;
          }
          printf("   Factions vues par siège (échantillon) : Savoir %d, Royaume %d, Ouvrages %d (sur 6)\n",
                 popcount[0], popcount[1], popcount[2]);
          ok("P0-1 : chaque siège accède à BIEN PLUS que son ancien spectre (≥4/6 factions vues)",
             popcount[0]>=4 && popcount[1]>=4 && popcount[2]>=4);
        }

        /* (2) CONVERGENCE SANS SAUT — la loyauté se déplace PROGRESSIVEMENT vers sa
         * cible, jamais un bond en un seul mois. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        int seat=0, best=0, bt=0;
        for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seat,sl,0); if(t>bt){bt=t;best=sl;} }
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        /* Une politique FAVORISE l'opposé de la faction du ministre (l'aigrit fort). */
        EthosFaction minf = statecraft_council_faction(seed,cid,seat,best,0);
        EthosFaction opp_f = FAC_CONQUERANT; float opp_v=-1.f;
        for (int f=0; f<FAC_COUNT; f++){ float o=faction_opposition(minf,(EthosFaction)f); if (o>opp_v){opp_v=o; opp_f=(EthosFaction)f;} }
        bool no_jump=true; int prev=statecraft_council_loyalty(s.sc,cid,seat);
        for (int m=0;m<36;m++){
            faction_lever_apply(cid, opp_f, 0.02f);     /* aigrit le ministre mois après mois */
            statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
            int cur=statecraft_council_loyalty(s.sc,cid,seat);
            if (prev-cur > 15) no_jump=false;           /* jamais un saut brutal en un mois */
            prev=cur;
        }
        int loy_after_grief=statecraft_council_loyalty(s.sc,cid,seat);
        printf("   Loyauté après 36 mois de grief opposé : %d (jamais de saut)\n", loy_after_grief);
        ok("Conseil vivant : la loyauté CONVERGE progressivement (jamais un saut brutal)", no_jump);
        ok("Conseil vivant : un grief soutenu envers SA faction FAIT BAISSER la loyauté",
           loy_after_grief < 60);

        /* (3) L'ASYMÉTRIE DU ROT — le rot (capture d'État) ACCÉLÈRE la chute,
         * jamais la remontée (motif COERCION_DECAY). Grief SATURÉ (≥1.0) pour que
         * la cible tombe SOUS la loyauté de départ (45-65) — une vraie CHUTE. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        faction_lever_apply(cid, opp_f, 1.5f);           /* un grief fixe, SATURÉ (cap 1.0) */
        for (int m=0;m<3;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
        int loy_baseline=statecraft_council_loyalty(s.sc,cid,seat);
        /* Même scénario, mais avec du ROT (capture d'État) accumulé au pays. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        faction_lever_apply(cid, opp_f, 1.5f);
        for (int k=0;k<20;k++) faction_concede(cid, minf);      /* gorge la faction du ministre : ROT */
        float rot = faction_capture_total(cid);
        for (int m=0;m<3;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
        int loy_rot=statecraft_council_loyalty(s.sc,cid,seat);
        printf("   Chute sur 3 mois : sans rot %d, avec rot(%.2f) %d\n", loy_baseline, rot, loy_rot);
        ok("Conseil vivant : le ROT accélère la CHUTE de loyauté (rot>0)", rot>0.f);
        ok("Conseil vivant : à grief égal, PLUS de rot ⇒ chute PLUS rapide", loy_rot < loy_baseline);
        /* Remontée : le rot ne doit RIEN accélérer côté hausse. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        statecraft_council_set_pay(s.sc, cid, seat, 2.f);         /* paie double : cible haute, ministre REMONTE */
        for (int m=0;m<3;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
        int rise_no_rot=statecraft_council_loyalty(s.sc,cid,seat);
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        statecraft_council_set_pay(s.sc, cid, seat, 2.f);
        for (int k=0;k<20;k++) faction_concede(cid, minf);
        for (int m=0;m<3;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
        int rise_with_rot=statecraft_council_loyalty(s.sc,cid,seat);
        printf("   Remontée sur 3 mois (paie ×2) : sans rot %d, avec rot %d\n", rise_no_rot, rise_with_rot);
        ok("Conseil vivant : le rot n'ACCÉLÈRE PAS la remontée (le rot n'aide jamais à se refaire une vertu)",
           rise_with_rot <= rise_no_rot);

        /* (4) BETRAYAL_READY — un ministre au bord (loyauté ≤ seuil) est signalé ; un
         * ministre choyé ne l'est jamais. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        statecraft_council_set_pay(s.sc, cid, seat, 0.f);         /* on ne PAIE plus : la cible plonge */
        faction_lever_apply(cid, opp_f, 1.5f);                    /* grief SATURÉ d'entrée (cap 1.0) */
        for (int m=0;m<30;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);   /* 30 mois : converge bien sous le seuil */
        bool bad_ready = statecraft_council_betrayal_ready(s.sc,cid,seat);
        printf("   Ministre non payé + grief soutenu : loyauté %d, au bord=%s\n",
               statecraft_council_loyalty(s.sc,cid,seat), bad_ready?"OUI":"non");
        ok("Conseil vivant : un ministre à loyauté basse est BETRAYAL_READY", bad_ready);
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        statecraft_council_set_pay(s.sc, cid, seat, 1.5f);
        for (int m=0;m<24;m++) statecraft_council_loyalty_tick(s.sc, s.w, s.econ, seed, 1.f/12.f);
        bool good_ready = statecraft_council_betrayal_ready(s.sc,cid,seat);
        ok("Conseil vivant : un ministre choyé (bien payé, aucun grief) N'EST JAMAIS betrayal_ready", !good_ready);
        ok("Conseil vivant : un siège VACANT n'est jamais betrayal_ready",
           !statecraft_council_betrayal_ready(s.sc,cid,1));

        /* (5) PAIR_STATE — 3 états lisibles : NEUTRE par défaut, RIVALITÉ (factions
         * opposées, tous deux en poste longtemps), CONSPIRATION (les deux aliénés). */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        int seatA=0, seatB=1;                            /* Savoir vs Société : spectres disjoints, peuvent s'opposer */
        int bestA=0, btA=0, bestB=0, btB=0;
        for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatA,sl,0); if(t>btA){btA=t;bestA=sl;} }
        for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatB,sl,0); if(t>btB){btB=t;bestB=sl;} }
        statecraft_council_hire(s.sc, seed, cid, seatA, bestA, 0);
        statecraft_council_hire(s.sc, seed, cid, seatB, bestB, 0);
        CouncilPairState st_fresh = statecraft_council_pair_state(s.sc, s.w, s.econ, seed, cid, seatA, seatB, 0);
        ok("Conseil vivant : deux ministres FRAIS (pas d'ancienneté) → NEUTRE",
           st_fresh==COUNCIL_PAIR_NEUTRE || st_fresh==COUNCIL_PAIR_ALLIANCE);
        /* En poste depuis longtemps (an 15 > 10 ans de tenure) : si leurs factions
         * s'opposent fort, la paire bascule en RIVALITÉ. */
        EthosFaction fA=statecraft_council_faction(seed,cid,seatA,bestA,0);
        EthosFaction fB=statecraft_council_faction(seed,cid,seatB,bestB,0);
        float oppAB = faction_opposition(fA,fB);
        CouncilPairState st_old = statecraft_council_pair_state(s.sc, s.w, s.econ, seed, cid, seatA, seatB, 15);
        printf("   Paire (%d,%d) opposition=%.2f, ancienne (an15) → état %d\n", seatA, seatB, oppAB, (int)st_old);
        if (oppAB>=0.6f)
            ok("Conseil vivant : factions opposées + ancienneté → RIVALITÉ", st_old==COUNCIL_PAIR_RIVALITE);
        else
            ok("Conseil vivant : factions proches + grief bas → ALLIANCE possible",
               st_old==COUNCIL_PAIR_ALLIANCE || st_old==COUNCIL_PAIR_NEUTRE);
        /* CONSPIRATION : les deux factions ALIÉNÉES (grief>0.6 chacune) — on
         * favorise une TROISIÈME faction dont l'opposition somme le plus haut
         * contre fA ET fB, pour aigrir les deux à la fois. */
        EthosFaction other=FAC_CONQUERANT; float bv=-1.f;
        for (int f=0; f<FAC_COUNT; f++) if (f!=(int)fA && f!=(int)fB){
            float o=faction_opposition(fA,(EthosFaction)f)+faction_opposition(fB,(EthosFaction)f);
            if (o>bv){ bv=o; other=(EthosFaction)f; }
        }
        for (int k=0;k<60;k++) faction_lever_apply(cid, other, 0.02f);
        CouncilPairState st_consp = statecraft_council_pair_state(s.sc, s.w, s.econ, seed, cid, seatA, seatB, 15);
        printf("   Grief fA=%.2f fB=%.2f → état %d\n", faction_grievance(cid,fA), faction_grievance(cid,fB), (int)st_consp);
        if (faction_grievance(cid,fA)>0.6f && faction_grievance(cid,fB)>0.6f)
            ok("Conseil vivant : les DEUX factions aliénées → CONSPIRATION", st_consp==COUNCIL_PAIR_CONSPIRATION);
        else
            ok("Conseil vivant : pair_state reste un lecteur cohérent (0-3 valeurs bornées)",
               st_consp>=COUNCIL_PAIR_NEUTRE && st_consp<=COUNCIL_PAIR_CONSPIRATION);

        /* (6) LA PAIE COÛTE — le curseur multiplie le coût mensuel. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        statecraft_council_hire(s.sc, seed, cid, seat, best, 0);
        float cost_1x = statecraft_council_cost(s.sc, seed, cid, 1.f);
        statecraft_council_set_pay(s.sc, cid, seat, 2.f);
        float cost_2x = statecraft_council_cost(s.sc, seed, cid, 1.f);
        statecraft_council_set_pay(s.sc, cid, seat, 0.5f);
        float cost_half = statecraft_council_cost(s.sc, seed, cid, 1.f);
        printf("   Coût mensuel : ×0.5 %.1f, ×1 %.1f, ×2 %.1f\n", cost_half, cost_1x, cost_2x);
        ok("Conseil vivant : payer DOUBLE coûte le double", near_f(cost_2x, cost_1x*2.f, 0.01f));
        ok("Conseil vivant : payer MOINS coûte moins", near_f(cost_half, cost_1x*0.5f, 0.01f));
        ok("Conseil vivant : le curseur de paie est BORNÉ [0,2]",
           statecraft_council_pay(s.sc,cid,seat)==0.5f &&
           (statecraft_council_set_pay(s.sc,cid,seat,9.f), statecraft_council_pay(s.sc,cid,seat)==2.f));

        /* (7) P0-4 — PERSONNE + MAISON : tirages déterministes et VIVANTS (varient
         * avec le candidat) — la maison ne « suit » pas le prénom (tables séparées,
         * salts distincts). */
        { const char *fn1 = statecraft_council_cand_firstname(seed,cid,0,0,0);
          const char *hs1 = statecraft_council_cand_house(seed,cid,0,0,0);
          ok("Conseil : prénom/maison DÉTERMINISTES (même clés → même valeur)",
             strcmp(fn1, statecraft_council_cand_firstname(seed,cid,0,0,0))==0 &&
             strcmp(hs1, statecraft_council_cand_house(seed,cid,0,0,0))==0);
          ok("Conseil : prénom et maison ne sont jamais vides", fn1[0]!=0 && hs1[0]!=0);
          bool firstname_varies=false, house_varies=false;
          for (int sl=1; sl<SC_COUNCIL_CANDS; sl++){
              if (strcmp(statecraft_council_cand_firstname(seed,cid,0,sl,0), fn1)!=0) firstname_varies=true;
              if (strcmp(statecraft_council_cand_house(seed,cid,0,sl,0), hs1)!=0)     house_varies=true;
          }
          ok("Conseil : le prénom varie avec le candidat (tirage vivant)", firstname_varies);
          ok("Conseil : la maison varie avec le candidat (tirage vivant, indépendant du prénom)", house_varies);
          printf("   Personne + maison, exemple : « %s %s » (siège Savoir, slot 0, gen 0)\n", fn1, hs1);
        }

        /* (8) P1-1 — EFFICACITÉ POLITIQUE : clamp(BASE + K_PER·K + LOY_W·loyauté/100
         * − CORRUPTION_PER_POINT·Corruption, MIN, MAX) — la formule VERBATIM de la
         * spec, et son rôle : bonus final du siège = bonus de RANG × efficacité (voir
         * statecraft_council_apply — seat_mult, LUI, reste le rang SEUL). */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        { int seatE=0, bestE=0, btE=0;
          for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatE,sl,0); if(t>btE){btE=t;bestE=sl;} }
          statecraft_council_hire(s.sc, seed, cid, seatE, bestE, 0);
          s.sc->loyalty[cid][seatE] = 70.f;                    /* accès struct direct : fixe une loyauté EXACTE pour le test */
          float Ksave = s.wp->country[cid].K;
          s.wp->country[cid].K = 6.f;
          for (int k=0;k<5;k++) faction_concede(cid, FAC_MARCHAND);   /* de la Corruption (peu importe la valeur exacte) */
          int corr = faction_corruption_0_100(cid);
          float eff_k6 = statecraft_council_efficiency(s.sc, s.wp, cid, seatE);
          float expect = tune_f("COUNCIL_EFF_BASE",0.70f) + tune_f("COUNCIL_EFF_K_PER",0.03f)*6.f
                       + tune_f("COUNCIL_EFF_LOY_W",0.15f)*0.70f - tune_f("COUNCIL_EFF_CORRUPTION_PER_POINT",0.0035f)*(float)corr;
          if (expect<tune_f("COUNCIL_EFF_MIN",0.50f)) expect=tune_f("COUNCIL_EFF_MIN",0.50f);
          if (expect>tune_f("COUNCIL_EFF_MAX",1.15f)) expect=tune_f("COUNCIL_EFF_MAX",1.15f);
          printf("   Efficacité (K=6, loy=70, Corr=%d) = %.3f (attendu %.3f, formule verbatim spec)\n", corr, eff_k6, expect);
          ok("P1-1 : l'efficacité SUIT exactement clamp(BASE+K_PER*K+LOY_W*loy/100-CORR*Corruption, MIN, MAX)",
             near_f(eff_k6, expect, 0.005f));
          s.wp->country[cid].K = 10.f;
          float eff_k10 = statecraft_council_efficiency(s.sc, s.wp, cid, seatE);
          ok("P1-1 : l'efficacité CROÎT avec K (monotone)", eff_k10 > eff_k6);
          s.wp->country[cid].K = 1000.f;
          ok("P1-1 : l'efficacité est BORNÉE au plafond COUNCIL_EFF_MAX",
             near_f(statecraft_council_efficiency(s.sc,s.wp,cid,seatE), tune_f("COUNCIL_EFF_MAX",1.15f), 0.001f));
          s.wp->country[cid].K = 0.f;
          s.sc->loyalty[cid][seatE] = 0.f;   /* ⚠ la Corruption PLAFONNE à 85 (spec) : à loy 70
                                              * l'eff vaut 0.5075 > plancher — il faut AUSSI
                                              * une loyauté nulle pour passer sous 0.50 */
          for (int k=0;k<200;k++) faction_concede(cid, FAC_MARCHAND);
          ok("P1-1 : l'efficacité est BORNÉE au plancher COUNCIL_EFF_MIN",
             near_f(statecraft_council_efficiency(s.sc,s.wp,cid,seatE), tune_f("COUNCIL_EFF_MIN",0.50f), 0.001f));
          statecraft_council_dismiss(s.sc, seed, cid, seatE);
          ok("P1-1 : un siège VACANT a une efficacité neutre (1.0, rien à multiplier)",
             statecraft_council_efficiency(s.sc,s.wp,cid,seatE)==1.f);
          s.wp->country[cid].K = Ksave;
        }

        /* (9) P1-3 — RENVOYER aigrit DIRECTEMENT la faction CONGÉDIÉE (plus de push
         * artificiel sur la faction la plus opposée à elle). */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        { int seatD=0, bestD=0, btD=0;
          for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatD,sl,0); if(t>btD){btD=t;bestD=sl;} }
          statecraft_council_hire(s.sc, seed, cid, seatD, bestD, 0);
          EthosFaction facD = statecraft_council_faction(seed,cid,seatD,bestD,0);
          float griefD_before = faction_grievance(cid, facD);
          statecraft_council_dismiss(s.sc, seed, cid, seatD);
          float griefD_after = faction_grievance(cid, facD);
          printf("   Renvoi : rancœur de SA propre faction %.3f → %.3f (+%.2f attendu)\n",
                 griefD_before, griefD_after, tune_f("COUNCIL_DISMISS_GRIEF",0.10f));
          ok("P1-3 : RENVOYER aigrit DIRECTEMENT la faction congédiée (pas un push vers l'opposée)",
             near_f(griefD_after, griefD_before + tune_f("COUNCIL_DISMISS_GRIEF",0.10f), 0.01f));
        }

        /* (10) P1-4 — une NOMINATION n'écrase JAMAIS un titulaire sans renvoi
         * explicite : le siège doit être vacant. */
        statecraft_init(s.sc, s.w); faction_levers_reset();
        { int seatG=0, s0=0, t0=0;
          for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){ int t=statecraft_council_cand_tier(seed,cid,seatG,sl,0); if(t>t0){t0=t;s0=sl;} }
          statecraft_council_hire(s.sc, seed, cid, seatG, s0, 0);
          int seated_before = statecraft_council_seated(s.sc,cid,seatG);
          int other = (s0+1)%SC_COUNCIL_CANDS;
          statecraft_council_hire(s.sc, seed, cid, seatG, other, 0);   /* tente de nommer SANS renvoyer d'abord */
          int seated_after = statecraft_council_seated(s.sc,cid,seatG);
          ok("P1-4 : une nomination sur un siège déjà POURVU est un NO-OP (le titulaire tient)",
             seated_after==seated_before);
          statecraft_council_dismiss(s.sc, seed, cid, seatG);
          statecraft_council_hire(s.sc, seed, cid, seatG, other, 0);
          ok("P1-4 : après un RENVOI explicite, la nomination réussit",
             statecraft_council_seated(s.sc,cid,seatG)==other);
        }
    }

    /* ── #26 — L'OPINION À MÉMOIRE : modificateurs TEMPORAIRES + mémoire DURABLE, tout TEND VERS 0 ──
     * (a) un STATUT actif (guerre) creuse l'opinion ; à la RUPTURE (paix) le modificateur disparaît →
     *     l'opinion REMONTE vers 0 (« au lieu de tendre vers −60, ça tend vers 0 »). (b) la TRAHISON
     *     laisse une marque DURABLE (opinion_mem) qui SURVIT au statut et s'estompe lentement vers 0. */
    printf("\n── #26 : opinion à mémoire (statuts temporaires + mémoire durable, tend vers 0) ──\n");
    {
        int A=0, B=1;
        /* (a) la GUERRE — un modificateur de STATUT, temporaire */
        statecraft_init(s.sc,s.w); diplo_init(s.dp);
        diplo_declare_war(s.dp, A, B);
        for (int m=0;m<24;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);   /* 2 ans de guerre */
        int op_war = statecraft_opinion(s.sc, A, B);
        ok("la GUERRE (statut actif) creuse l'opinion (< -20)", op_war < -20);
        diplo_make_peace(s.dp, A, B);
        for (int m=0;m<48;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);   /* 4 ans de paix */
        int op_peace = statecraft_opinion(s.sc, A, B);
        ok("à la PAIX le modificateur DISPARAÎT → l'opinion REMONTE vers 0", op_peace > op_war + 15 && op_peace > -10);
        /* (b) la TRAHISON — une mémoire DURABLE qui survit au statut */
        statecraft_init(s.sc,s.w); diplo_init(s.dp);
        statecraft_on_betrayal(s.sc, A);                 /* A trahit : les AUTRES le retiennent */
        for (int m=0;m<6;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);
        int op_bet = statecraft_opinion(s.sc, B, A);     /* ce que B pense du traître A (hors tout statut) */
        ok("la TRAHISON marque DURABLEMENT (B→A < -10, aucun statut actif)", op_bet < -10);
        for (int m=0;m<360;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);  /* 30 ans */
        int op_faded = statecraft_opinion(s.sc, B, A);
        ok("la mémoire de la trahison S'ESTOMPE vers 0 (decay naturelle)", op_faded > op_bet + 5 && op_faded > -6);
        /* (c) la SÉCESSION (#26bis) — le pays né d'une guerre civile AIME MOINS l'empire père
         * (Flandre vs France) : mémoire DURABLE dans UN SEUL SENS (le fils porte la plaie). */
        statecraft_init(s.sc,s.w); diplo_init(s.dp);
        statecraft_on_secession(s.sc, B, A);             /* B fait sécession de A */
        for (int m=0;m<6;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);
        int op_sec = statecraft_opinion(s.sc, B, A);     /* ce que le FILS pense du PÈRE */
        int op_rev = statecraft_opinion(s.sc, A, B);     /* le père : AUCUNE mémoire ajoutée */
        ok("la SÉCESSION marque le fils DURABLEMENT (fils→père < -12)", op_sec < -12);
        ok("un seul sens : le père ne gagne pas de mémoire (père→fils ~ 0)", op_rev > -6 && op_rev < 6);
        for (int m=0;m<360;m++) statecraft_tick(s.sc,s.w,s.econ,s.wp,s.wl,s.dp,s.rn,30);  /* 30 ans */
        int op_secf = statecraft_opinion(s.sc, B, A);
        ok("la rancune de sécession S'ESTOMPE (decay naturelle)", op_secf > op_sec + 5 && op_secf > -8);
        /* (d) le RÉSUMÉ (statecraft_opinion_parts) — les composantes = la CIBLE du lissage */
        statecraft_init(s.sc,s.w); diplo_init(s.dp);
        diplo_declare_war(s.dp, A, B);
        statecraft_on_secession(s.sc, A, B);             /* A garde AUSSI une mémoire contre B */
        OpinionParts pp; statecraft_opinion_parts(s.sc, s.dp, A, B, &pp);
        ok("résumé : la GUERRE paraît en composante (war < 0)", pp.war < -1.f);
        ok("résumé : la MÉMOIRE paraît en composante (mem < 0)", pp.mem < -1.f);
        ok("résumé : les statuts inactifs restent à 0", pp.ally==0.f && pp.pact==0.f && pp.vassal==0.f);
    }

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf(" BILAN : %d réussis, %d échoués\n", g_pass, g_fail);
    printf("══════════════════════════════════════════════════════════════\n");
    free(s.w);free(s.econ);free(s.net);free(s.ts);free(s.wp);free(s.wl);free(s.dp);free(s.rn);free(s.sc);
    return g_fail?1:0;
}
