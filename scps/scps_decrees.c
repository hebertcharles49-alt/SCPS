/*
 * scps_decrees.c — voir scps_decrees.h. Chaque orientation/décision DÉPLACE un levier
 * moteur EXISTANT ; aucun système neuf. Appliqué UNIQUEMENT au joueur humain, une fois
 * par mois (miroir de statecraft_council_apply) — la chronique ne l'appelle jamais ⇒
 * golden intact par construction (le bit d'un pays IA ne bouge JAMAIS : g_decree_mask
 * n'est écrit que par decree_toggle, appelé UNIQUEMENT depuis CMD_DECREE, qui n'existe
 * QUE dans le journal du joueur — cf. scps_sim.c sim_cmd_drain).
 */
#include "scps_decrees.h"
#include "scps_factions.h"   /* faction_audit/faction_corruption_0_100 (I5, API publique) */
#include "scps_math.h"       /* clampf partagé */
#include <string.h>

uint32_t g_decree_mask[SCPS_MAX_COUNTRY];
/* flag TRANSITOIRE (mois courant) : le décret a été FINANCÉ ce mois-ci — jamais
 * sérialisé (recalculé au premier decrees_tick suivant un chargement, comme documenté
 * dans le .h). 1 bit/décret, même forme que g_decree_mask. */
static uint32_t g_decree_funded[SCPS_MAX_COUNTRY];
/* cooldown de la DÉCISION Audit des offices (jours restants, décroît même à 0 actif —
 * ACCUMULATEUR INTER-TICKS : jurisprudence EMOB/COLC/TXYR/RVLT → SÉRIALISÉ ci-dessous,
 * borné au load comme rs->coup_grace/[-31, 40*365]). */
static float g_audit_cd[SCPS_MAX_COUNTRY];

/* ---- Conditions d'entrée (lisent l'état réel, jamais un flag gratuit) ---- */
static bool cond_levee(const World *w, const WorldEconomy *econ, const TechState *ts,
                       const WorldLegitimacy *wl, const Statecraft *sc,
                       const DiploState *dp, int cid){
    (void)w;(void)econ;(void)wl;(void)sc;(void)dp;
    return ts && cid>=0 && cid<SCPS_MAX_COUNTRY && ts[cid].unlocked[TECH_CONSCRIPTION];
}
/* AUDIT DES OFFICES — condition PARTAGÉE entre decree_legal (griser le bouton) et
 * decree_fire_decision (revalider au tir, jamais à l'aveugle) : Corruption ≥ seuil ET
 * cooldown écoulé. */
static bool audit_ready(int cid){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY) return false;
    if (g_audit_cd[cid] > 0.f) return false;
    return faction_corruption_0_100(cid) >= (int)tune_f("DECISION_AUDIT_CORRUPTION_MIN", 20.f);
}
static bool cond_audit(const World *w, const WorldEconomy *econ, const TechState *ts,
                       const WorldLegitimacy *wl, const Statecraft *sc,
                       const DiploState *dp, int cid){
    (void)w;(void)econ;(void)ts;(void)wl;(void)sc;(void)dp;
    return audit_ready(cid);
}

const DecreeDef DECREES[DECREE_COUNT] = {
    [DECREE_RATIONS] = {
        "Rations mesurées", "Chaque bouche recevra sa part. Le greffier précise seulement que la part sera plus petite.",
        "+ FOOD_NEED×0.95, POP_R_BASE×0.97 (moins de vivres nécessaires) / - la natalité en pâtit un peu "
        "(⊥ Primes aux foyers)",
        DCR_EDIT, NULL },
    [DECREE_FOYERS] = {
        "Primes aux foyers", "La couronne célèbre les berceaux. Les greniers, eux, comptent déjà les années.",
        "+ POP_R_BASE×1.05 (natalité) / - FOOD_NEED×1.04 (plus de bouches à nourrir) (⊥ Rations mesurées)",
        DCR_EDIT, NULL },
    [DECREE_ECOLES] = {
        "Écoles soutenues", "Le maître reçoit une bourse, l'élève une place et le trésorier une nouvelle colonne de dépenses.",
        "+ SAVOIR_W_{élite,bourgeois,laboureur}×1.05 (recherche) / - ponction mensuelle du trésor royal",
        DCR_EDIT, NULL },
    [DECREE_ATELIERS] = {
        "Ateliers soutenus", "La couronne paie les premières pierres. Les guildes conserveront volontiers les murs.",
        "+ MANUF_BUILD_COST×0.95 (construction moins chère) / - ponction mensuelle du trésor royal",
        DCR_EDIT, NULL },
    [DECREE_COMPTOIRS] = {
        "Comptoirs soutenus", "Un sceau royal sur une lettre de change ne la rend pas plus honnête, seulement plus facile à encaisser.",
        "+ COMMERCE_W_{bourgeois,élite}×1.05 (volume échangeable) / - ponction mensuelle du trésor royal",
        DCR_EDIT, NULL },
    [DECREE_CIRCULATION] = {
        "Circulation encouragée", "Les routes restent ouvertes...",
        "+ MIG_PACT_*×1.10 (entrants ET sortants) / - ponction mensuelle du trésor royal (⊥ Frontières fermées)",
        DCR_EDIT, NULL },
    [DECREE_FRONTIERES] = {
        "Frontières fermées", "Les portes se ferment aux familles...",
        "+ rien ne coûte (0 or) / - MIG_PACT_*×0 (plus de pacte migratoire) ET COMMERCE_W_*×0.95 "
        "(ne bloque PAS les réfugiés de guerre/sac) (⊥ Circulation encouragée)",
        DCR_EDIT, NULL },
    [DECREE_MECENAT] = {   /* FÊTES PUBLIQUES — RÉUTILISE le bit DECREE_MECENAT (spec : aucun enum/état/save neuf) */
        "Fêtes publiques", "La place reçoit des tables, des musiciens et assez de vin pour que les griefs parlent moins fort jusqu'au lendemain.",
        "+ W_AGITATION_UNREST×0.95 (agitation moins contagieuse) / - ponction mensuelle du trésor royal",
        DCR_EDIT, NULL },
    [DECREE_LEGATIONS] = {
        "Légations permanentes", "Une table bien servie ouvre parfois une frontière que trois armées n'auraient fait que fortifier.",
        "+ influence +0.25/mois (plafond 100) / - ponction mensuelle du trésor royal",
        DCR_EDIT, NULL },
    [DECREE_LEVEE_ENTRETENUE] = {
        "Levée entretenue", "Les mêmes hommes montent la garde chaque saison. Leurs champs apprennent peu à peu à se passer d'eux.",
        "+ la levée tenue au plancher DECREE_LEVEE_MIN_LEVEL (jamais réduite tant qu'actif) / "
        "- les bras qui manquent aux champs et aux ateliers (0 or, contrepartie = main-d'œuvre)",
        DCR_EDIT, cond_levee },
    [DECISION_AUDIT_OFFICES] = {
        "Audit des offices", "Les livres sont ouverts. Dans chaque marge, quelqu'un découvre que son nom possédait un prix.",
        "+ Corruption −20 pts, capitale L +0.3 si Corruption>50 sinon L −0.3 / - coût ponctuel 25% du "
        "revenu annuel×IPM, cooldown 5 ans, exige Corruption≥20",
        DCR_DECISION, cond_audit },
};

bool decree_active(int cid, DecreeId id){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || id<0 || id>=DECREE_COUNT) return false;
    return (g_decree_mask[cid] & (1u<<(unsigned)id)) != 0;
}
/* La paire mutuellement exclusive (radio-boutons) d'un décret — DECREE_COUNT si aucune. */
static DecreeId decree_exclusive_of(DecreeId id){
    switch (id){
        case DECREE_RATIONS:     return DECREE_FOYERS;
        case DECREE_FOYERS:      return DECREE_RATIONS;
        case DECREE_CIRCULATION: return DECREE_FRONTIERES;
        case DECREE_FRONTIERES:  return DECREE_CIRCULATION;
        default:                 return DECREE_COUNT;
    }
}
void decree_toggle(int cid, DecreeId id, bool on){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || id<0 || id>=DECREE_COUNT) return;
    if (DECREES[id].type==DCR_DECISION) return;   /* les décisions passent par decree_fire_decision */
    bool was = decree_active(cid, id);
    if (DECREES[id].type==DCR_REFORME && was && !on) return;   /* réforme : le retour arrière est REFUSÉ */
    if (on){
        DecreeId excl = decree_exclusive_of(id);
        if (excl!=DECREE_COUNT && decree_active(cid,excl))
            g_decree_mask[cid] &= ~(1u<<(unsigned)excl);       /* radio-bouton : active l'un, désactive l'autre */
        g_decree_mask[cid] |= (1u<<(unsigned)id);
    } else {
        g_decree_mask[cid] &= ~(1u<<(unsigned)id);
    }
}
bool decree_legal(const World *w, const WorldEconomy *econ, const TechState *ts,
                  const WorldLegitimacy *wl, const Statecraft *sc,
                  const DiploState *dp, int cid, DecreeId id){
    if (id<0 || id>=DECREE_COUNT) return false;
    /* une réforme déjà active reste "légale" (affichée verrouillée, pas grisée-refusée) */
    if (DECREES[id].type==DCR_REFORME && decree_active(cid,id)) return true;
    DecreeCond c = DECREES[id].cond;
    return c ? c(w,econ,ts,wl,sc,dp,cid) : true;
}

/* Le décret est-il actif ET financé (ce mois-ci) — cf. .h. */
static bool decree_effective(int cid, DecreeId id){
    if (!decree_active(cid, id)) return false;
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || id<0 || id>=DECREE_COUNT) return false;
    return (g_decree_funded[cid] & (1u<<(unsigned)id)) != 0;
}
float decree_mult(int cid, DecreeId id, float mult_actif){
    return decree_effective(cid, id) ? mult_actif : 1.0f;
}

/* ---- Composites par site de lecture (cf. .h) ----------------------------- */
float decree_food_need_mult(int cid){
    return decree_mult(cid, DECREE_RATIONS, tune_f("DECREE_RATIONS_FOOD_NEED_MULT", 0.95f))
         * decree_mult(cid, DECREE_FOYERS,  tune_f("DECREE_FOYERS_FOOD_NEED_MULT",  1.04f));
}
float decree_pop_r_base_mult(int cid){
    return decree_mult(cid, DECREE_RATIONS, tune_f("DECREE_RATIONS_POP_R_BASE_MULT", 0.97f))
         * decree_mult(cid, DECREE_FOYERS,  tune_f("DECREE_FOYERS_POP_R_BASE_MULT",  1.05f));
}
float decree_savoir_w_mult(int cid){
    return decree_mult(cid, DECREE_ECOLES, tune_f("DECREE_ECOLES_SAVOIR_W_MULT", 1.05f));
}
float decree_manuf_cost_mult(int cid){
    return decree_mult(cid, DECREE_ATELIERS, tune_f("DECREE_ATELIERS_MANUF_COST_MULT", 0.95f));
}
float decree_commerce_w_mult(int cid){
    return decree_mult(cid, DECREE_COMPTOIRS,  tune_f("DECREE_COMPTOIRS_COMMERCE_W_MULT",  1.05f))
         * decree_mult(cid, DECREE_FRONTIERES, tune_f("DECREE_FRONTIERES_COMMERCE_W_MULT", 0.95f));
}
float decree_mig_pact_mult(int cid){
    return decree_mult(cid, DECREE_CIRCULATION, tune_f("DECREE_CIRCULATION_MIG_PACT_MULT", 1.10f))
         * decree_mult(cid, DECREE_FRONTIERES,  tune_f("DECREE_FRONTIERES_MIG_PACT_MULT",  0.0f));
}
float decree_unrest_mult(int cid){
    return decree_mult(cid, DECREE_MECENAT, tune_f("DECREE_MECENAT_UNREST_MULT", 0.95f));
}

/* ---- Trésor de la capitale (province représentative — treasury est PROVINCE-OWNED,
 * miroir de statecraft_council_apply/econ_region_rep_province) --------------------- */
static int decree_capital_region(const World *w, int cid){
    if (!w || cid<0 || cid>=w->n_countries) return -1;
    int cap = w->country[cid].capital_prov;
    if (cap<0 || cap>=w->n_provinces) return -1;
    return w->province[cap].region;
}
/* DÉCISION (ponctuelle) : prend ce qui est disponible, jamais en négatif imposé — le
 * trésor plafonne la dépense (motif hérité de l'ancien decree_spend_capital). */
static float decree_spend_capital(const World *w, WorldEconomy *econ, int cid, float cost){
    if (cost<=0.f) return 0.f;
    int cr = decree_capital_region(w, cid);
    if (cr<0 || cr>=econ->n_regions) return 0.f;
    int crp = econ_region_rep_province(econ, cr);
    if (crp<0 || crp>=econ->n_prov) return 0.f;
    float have = econ->prov[crp].treasury;
    float take = (have<cost) ? have : cost;
    econ->prov[crp].treasury -= take;
    econ_flux_add(cid, FX_CONSEIL, -take);    /* même ligne que le Conseil : une dépense de cour */
    return take;
}
/* ORIENTATION (mensuelle) : TOUT ou RIEN — « trésor insuffisant CE mois ⇒ désactivée et
 * sans effet CE mois » (règle joueur). Contrairement à decree_spend_capital ci-dessus
 * (décisions ponctuelles), une orientation ne paie JAMAIS une fraction de son coût. */
static bool decree_afford_capital(const World *w, WorldEconomy *econ, int cid, float cost){
    if (cost<=0.f) return true;
    int cr = decree_capital_region(w, cid);
    if (cr<0 || cr>=econ->n_regions) return false;
    int crp = econ_region_rep_province(econ, cr);
    if (crp<0 || crp>=econ->n_prov) return false;
    if (econ->prov[crp].treasury < cost) return false;
    econ->prov[crp].treasury -= cost;
    econ_flux_add(cid, FX_CONSEIL, -cost);
    return true;
}

/* Taux de revenu annuel (× IPM, /12 mensuel) de chaque ÉDIT — clés au registre J. */
static float decree_revenue_rate(DecreeId id){
    switch (id){
        case DECREE_RATIONS:          return tune_f("DECREE_RATIONS_REVENUE_RATE",      0.005f);
        case DECREE_FOYERS:           return tune_f("DECREE_FOYERS_REVENUE_RATE",       0.015f);
        case DECREE_ECOLES:           return tune_f("DECREE_ECOLES_REVENUE_RATE",       0.02f);
        case DECREE_ATELIERS:         return tune_f("DECREE_ATELIERS_REVENUE_RATE",     0.02f);
        case DECREE_COMPTOIRS:        return tune_f("DECREE_COMPTOIRS_REVENUE_RATE",    0.015f);
        case DECREE_CIRCULATION:      return tune_f("DECREE_CIRCULATION_REVENUE_RATE",  0.0075f);
        case DECREE_FRONTIERES:       return tune_f("DECREE_FRONTIERES_REVENUE_RATE",   0.0f);
        case DECREE_MECENAT:          return tune_f("DECREE_MECENAT_REVENUE_RATE",      0.015f);
        case DECREE_LEGATIONS:        return tune_f("DECREE_LEGATIONS_REVENUE_RATE",    0.015f);
        case DECREE_LEVEE_ENTRETENUE: return tune_f("DECREE_LEVEE_REVENUE_RATE",        0.0f);
        default: return 0.f;
    }
}

void decrees_tick(World *w, WorldEconomy *econ, Statecraft *sc, WorldLegitimacy *wl,
                  WarHost *host, DiploState *dp, int cid, int days){
    (void)wl; (void)dp;
    if (!w || !econ || cid<0 || cid>=w->n_countries || cid>=SCPS_MAX_COUNTRY) return;
    float dt_year = (float)days/365.f;
    float ipm = econ_world_ipm(econ);
    float revenue = econ_country_tax_year(cid);

    if (g_audit_cd[cid] > 0.f) g_audit_cd[cid] -= (float)days;   /* cooldown de la décision Audit */

    /* COÛT MENSUEL de chaque ÉDIT actif — TOUT ou RIEN, pose le flag "financé" du mois
     * (recalculé intégralement : aucun décret désactivé ne garde un flag périmé). */
    g_decree_funded[cid] = 0;
    for (int id=0; id<DECREE_COUNT; id++){
        if (DECREES[id].type==DCR_DECISION) continue;
        if (!decree_active(cid, (DecreeId)id)) continue;
        /* ⚠ le TAUX est ANNUEL (spec : « coût annuel = tax_year × taux × IPM ») —
         * la part du tick = × dt_year SEUL. Le « ×12 » initial (copié du motif
         * Légations ligne ~249, où la valeur est PAR-MOIS) faisait payer 12× trop :
         * une orientation à 2 % prélevait 24 %/an. Pris par l'agent P4, vérifié ici. */
        float cost = revenue * decree_revenue_rate((DecreeId)id) * ipm * dt_year;
        if (decree_afford_capital(w, econ, cid, cost))
            g_decree_funded[cid] |= (1u<<(unsigned)id);
    }

    /* LÉGATIONS PERMANENTES (ex-DECREE_AMBASSADES) — +influence/mois, SEULEMENT si financé. */
    if (decree_effective(cid, DECREE_LEGATIONS) && sc && cid<sc->n_countries){
        sc->influence[cid] += tune_f("DECREE_LEGATIONS_INFLUENCE_PER_MONTH", 0.25f) * dt_year * 12.f;
        if (sc->influence[cid] > 100.f) sc->influence[cid] = 100.f;
    }
    /* LEVÉE ENTRETENUE (ex-DECREE_LEVEE_PERMANENTE) — verrouille la levée au plancher
     * (0 or : la contrepartie est la main-d'œuvre immobilisée, DÉJÀ portée par warhost_tick). */
    if (decree_effective(cid, DECREE_LEVEE_ENTRETENUE) && host){
        int min_lvl = (int)tune_f("DECREE_LEVEE_MIN_LEVEL", 2.0f);   /* = WH_LEVY_GUERRE */
        if (warhost_levy(host, cid) < min_lvl) warhost_set_levy(host, cid, min_lvl);
    }
    /* RATIONS/FOYERS (FOOD_NEED, POP_R_BASE), ÉCOLES (SAVOIR_W_*), ATELIERS (MANUF_BUILD_COST),
     * COMPTOIRS/FRONTIÈRES (COMMERCE_W_*), CIRCULATION/FRONTIÈRES (MIG_PACT_*), FÊTES
     * (W_AGITATION_UNREST) : PURS effets de LECTURE (decree_mult aux sites de scps_econ.c/
     * scps_demography.c/scps_revolt.c/scps_sim.c) — rien de plus à faire ICI que le coût
     * déjà prélevé + le flag "financé" posé ci-dessus. */
}

/* ---- DÉCISION PONCTUELLE — AUDIT DES OFFICES ------------------------------------ */
bool decree_fire_decision(World *w, WorldEconomy *econ, WorldLegitimacy *wl, int cid, DecreeId id){
    if (id!=DECISION_AUDIT_OFFICES) return false;
    if (!w || !econ || cid<0 || cid>=w->n_countries || cid>=SCPS_MAX_COUNTRY) return false;
    if (!audit_ready(cid)) return false;   /* condition + cooldown, REVALIDÉS au tir */

    float revenue = econ_country_tax_year(cid);
    float cost = revenue * tune_f("DECISION_AUDIT_REVENUE_RATE", 0.25f) * econ_world_ipm(econ);
    decree_spend_capital(w, econ, cid, cost);         /* ponctuel : prend ce qui est dispo */

    int before = faction_audit(cid);                  /* -20 pts (borné en interne) ; renvoie AVANT */
    float ldelta = tune_f("DECISION_AUDIT_L_DELTA", 0.3f);
    float dl = (before > 50) ? ldelta : -ldelta;       /* L +0.3 si Corruption(avant)>50, sinon L -0.3 */
    int cr = decree_capital_region(w, cid);
    if (wl && cr>=0 && cr<SCPS_MAX_REG)
        wl->L[cr] = clampf(wl->L[cr] + dl, 0.f, 10.f);

    g_audit_cd[cid] = tune_f("DECISION_AUDIT_COOLDOWN_YEARS", 5.f) * 365.f;
    return true;
}

/* ---- Sérialisation (section DCRE, motif WILD : sim_wild_save/load) ------ */
void decrees_save(FILE *f){
    fwrite(g_decree_mask,   sizeof g_decree_mask,   1, f);
    fwrite(g_audit_cd,      sizeof g_audit_cd,      1, f);
    fwrite(g_decree_funded, sizeof g_decree_funded, 1, f);   /* AUDIT P4 : le flag « financé ce mois » */
}
bool decrees_load(FILE *f){
    bool ok = fread(g_decree_mask,   sizeof g_decree_mask,   1, f) == 1
           && fread(g_audit_cd,      sizeof g_audit_cd,      1, f) == 1
           && fread(g_decree_funded, sizeof g_decree_funded, 1, f) == 1;
    if (!ok) return false;
    /* toute valeur désérialisée qui borne une décision SE REVALIDE (jurisprudence RVLT :
     * [-31, 40*365] jours — une save forgée avec un cooldown hors-borne est REFUSÉE net). */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)
        if (g_audit_cd[c] < -31.f || g_audit_cd[c] > 40.f*365.f) return false;
    /* AUDIT P4 : le flag « financé » pilote les EFFETS réels ; auparavant NON sérialisé,
     * il valait 0 (process frais) ou gardait la valeur d'une autre partie jusqu'au prochain
     * tick mensuel — un ordre émis dans cette fenêtre perdait la remise du décret. Restauré ;
     * BORNÉ au masque CHOISI (on ne peut financer qu'un décret adopté : jamais un effet fantôme). */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)
        g_decree_funded[c] &= g_decree_mask[c];
    return true;
}
void decrees_reset(void){
    memset(g_decree_mask,   0, sizeof g_decree_mask);
    memset(g_decree_funded, 0, sizeof g_decree_funded);   /* flag transitoire : RAZ, pas sérialisé */
    memset(g_audit_cd,      0, sizeof g_audit_cd);
}
