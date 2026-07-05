/*
 * scps_decrees.c — voir scps_decrees.h. Chaque décret DÉPLACE un levier moteur
 * EXISTANT ; aucun système neuf. Appliqué UNIQUEMENT au joueur humain, une fois
 * par mois (miroir de statecraft_council_apply) — la chronique ne l'appelle
 * jamais ⇒ golden intact par construction.
 */
#include "scps_decrees.h"
#include <string.h>

uint32_t g_decree_mask[SCPS_MAX_COUNTRY];

/* ---- Conditions d'entrée (lisent l'état réel, jamais un flag gratuit) ---- */
static bool cond_levee(const World *w, const WorldEconomy *econ, const TechState *ts,
                       const WorldLegitimacy *wl, const Statecraft *sc,
                       const DiploState *dp, int cid){
    (void)w;(void)econ;(void)wl;(void)sc;(void)dp;
    return ts && cid>=0 && cid<SCPS_MAX_COUNTRY && ts[cid].unlocked[TECH_CONSCRIPTION];
}
static bool cond_mecenat(const World *w, const WorldEconomy *econ, const TechState *ts,
                         const WorldLegitimacy *wl, const Statecraft *sc,
                         const DiploState *dp, int cid){
    (void)w;(void)ts;(void)wl;(void)sc;(void)dp;
    if (!econ || cid<0 || cid>=SCPS_MAX_COUNTRY) return false;
    return econ_country_tax_year(cid) > 0.f;   /* un trésor qui roule : le mécénat a de quoi puiser */
}
static bool cond_ambassades(const World *w, const WorldEconomy *econ, const TechState *ts,
                            const WorldLegitimacy *wl, const Statecraft *sc,
                            const DiploState *dp, int cid){
    (void)w;(void)econ;(void)ts;(void)wl;(void)dp;
    if (!sc || cid<0 || cid>=sc->n_countries) return false;
    return sc->influence[cid] >= 0.f;   /* toujours permis (posture d'ouverture, pas de seuil dur) */
}
static bool cond_tribut(const World *w, const WorldEconomy *econ, const TechState *ts,
                        const WorldLegitimacy *wl, const Statecraft *sc,
                        const DiploState *dp, int cid){
    (void)w;(void)econ;(void)ts;(void)wl;(void)sc;
    return dp && diplo_vassal_count(dp, cid) > 0;   /* la réforme n'a de sens qu'avec au moins un vassal */
}

const DecreeDef DECREES[DECREE_COUNT] = {
    [DECREE_LEVEE_PERMANENTE] = {
        "Levée permanente", "Le paysan laboure, le soldat marche : les deux à la fois, sur ordre du prince.",
        "+ la levée tenue en permanence à WH_LEVY_GUERRE (jamais réduite tant qu'actif) / "
        "- les bras qui manquent aux champs et aux ateliers (moins de main-d'œuvre civile)",
        DCR_EDIT, cond_levee },
    [DECREE_MECENAT] = {
        "Mécénat des arts", "Un prince qui ne bâtit rien de beau n'est qu'un percepteur en couronne.",
        "+ prestige mensuel (statecraft) / - ponction du trésor royal proportionnelle au revenu annuel",
        DCR_EDIT, cond_mecenat },
    [DECREE_AMBASSADES] = {
        "Réseau d'ambassades", "Un émissaire dans chaque cour coûte moins cher qu'une armée aux frontières.",
        "+ influence mensuelle (agrandit aussi le vivier de diplomates) / - ponction modérée du trésor royal",
        DCR_EDIT, cond_ambassades },
    [DECREE_TRIBUT] = {
        "Politique de tribut", "Le vassal qui paie double n'oublie jamais qui l'a taillé en deux.",
        "+ ×1.5 la contribution de TOUS les vassaux (or/vivres/levée) / - grief vassal accru en continu "
        "(la fronde couve plus vite) — IRRÉVERSIBLE",
        DCR_REFORME, cond_tribut },
};

bool decree_active(int cid, DecreeId id){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || id<0 || id>=DECREE_COUNT) return false;
    return (g_decree_mask[cid] & (1u<<(unsigned)id)) != 0;
}
/* re-synchronise le drapeau LU par scps_diplo.c (Politique de tribut) depuis le masque
 * persistant — appelé au toggle ET après un chargement (decrees_load) : sans ce second
 * appel, un monde rechargé garderait le drapeau à false malgré une réforme déjà active. */
static void decree_sync_tribute(int cid){
    diplo_set_tribute_decree(cid, decree_active(cid, DECREE_TRIBUT));
}
void decree_toggle(int cid, DecreeId id, bool on){
    if (cid<0 || cid>=SCPS_MAX_COUNTRY || id<0 || id>=DECREE_COUNT) return;
    bool was = decree_active(cid, id);
    if (DECREES[id].type==DCR_REFORME && was && !on) return;   /* réforme : le retour arrière est REFUSÉ */
    if (on) g_decree_mask[cid] |= (1u<<(unsigned)id);
    else    g_decree_mask[cid] &= ~(1u<<(unsigned)id);
    if (id==DECREE_TRIBUT) decree_sync_tribute(cid);
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

/* ---- Tarifs / calibrage (contrepartie mensuelle) ------------------------ */
#define DECREE_MECENAT_FRAC     0.006f   /* fraction du revenu ANNUEL ponctionnée /mois (gros coût, gros effet) */
#define DECREE_MECENAT_PRESTIGE 0.6f     /* +prestige/mois */
#define DECREE_AMBASSADE_FRAC   0.0025f  /* coût moindre (différencié du mécénat) */
#define DECREE_AMBASSADE_INFLUX 0.5f     /* +influence/mois (converge plus vite vers le standing) */

/* ponction sur le trésor de la CAPITALE (province représentative — treasury est
 * PROVINCE-OWNED, miroir de statecraft_council_apply/econ_region_rep_province). */
static float decree_spend_capital(const World *w, WorldEconomy *econ, int cid, float cost){
    if (cost<=0.f) return 0.f;
    int cap = w->country[cid].capital_prov;
    int cr  = (cap>=0 && cap<w->n_provinces) ? w->province[cap].region : -1;
    if (cr<0 || cr>=econ->n_regions) return 0.f;
    int crp = econ_region_rep_province(econ, cr);
    if (crp<0 || crp>=econ->n_prov) return 0.f;
    float have = econ->prov[crp].treasury;
    float take = (have<cost) ? have : cost;   /* jamais en négatif imposé : le trésor plafonne la dépense */
    econ->prov[crp].treasury -= take;
    econ_flux_add(cid, FX_CONSEIL, -take);    /* même ligne que le Conseil : une dépense de cour */
    return take;
}

void decrees_tick(World *w, WorldEconomy *econ, Statecraft *sc, WorldLegitimacy *wl,
                  WarHost *host, DiploState *dp, int cid, int days){
    (void)wl; (void)dp;
    if (!w || !econ || cid<0 || cid>=w->n_countries || cid>=SCPS_MAX_COUNTRY) return;
    float dt_year = (float)days/365.f;
    float ipm = econ_world_ipm(econ);

    /* LEVÉE PERMANENTE — verrouille la jauge haute (contrepartie : DÉJÀ portée par
     * warhost_tick — une levée haute immobilise des bras/prod, c'est l'entrée existante). */
    if (decree_active(cid, DECREE_LEVEE_PERMANENTE) && host)
        if (warhost_levy(host, cid) < WH_LEVY_GUERRE)
            warhost_set_levy(host, cid, WH_LEVY_GUERRE);

    /* MÉCÉNAT DES ARTS — +prestige/mois, ponction ∝ revenu annuel × IPM. */
    if (decree_active(cid, DECREE_MECENAT) && sc && cid<sc->n_countries){
        float revenue = econ_country_tax_year(cid);
        float cost = revenue * DECREE_MECENAT_FRAC * ipm * dt_year * 12.f;
        decree_spend_capital(w, econ, cid, cost);
        sc->prestige[cid] += DECREE_MECENAT_PRESTIGE * dt_year * 12.f;
        if (sc->prestige[cid] > 100.f) sc->prestige[cid] = 100.f;
    }

    /* RÉSEAU D'AMBASSADES — +influence/mois (agrandit le vivier de diplomates via
     * staff_cap, LECTEUR existant), ponction moindre que le mécénat. */
    if (decree_active(cid, DECREE_AMBASSADES) && sc && cid<sc->n_countries){
        float revenue = econ_country_tax_year(cid);
        float cost = revenue * DECREE_AMBASSADE_FRAC * ipm * dt_year * 12.f;
        decree_spend_capital(w, econ, cid, cost);
        sc->influence[cid] += DECREE_AMBASSADE_INFLUX * dt_year * 12.f;
        if (sc->influence[cid] > 100.f) sc->influence[cid] = 100.f;
    }
    /* POLITIQUE DE TRIBUT : la réforme ne fait rien ICI — son levier (×1.5 la
     * contribution vassale + grief accru) vit DANS diplo_suzerainty_tick, la seule
     * fonction qui possède déjà la boucle vassal→maître (cf. scps_diplo.c). */
}

/* ---- Sérialisation (section DCRE, motif WILD : sim_wild_save/load) ------ */
void decrees_save(FILE *f){
    fwrite(g_decree_mask, sizeof g_decree_mask, 1, f);
}
bool decrees_load(FILE *f){
    bool ok = fread(g_decree_mask, sizeof g_decree_mask, 1, f) == 1;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) decree_sync_tribute(c);   /* re-synchronise le drapeau diplo */
    return ok;
}
void decrees_reset(void){
    memset(g_decree_mask, 0, sizeof g_decree_mask);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) diplo_set_tribute_decree(c, false);
}
