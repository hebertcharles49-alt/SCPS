/*
 * scps_events.c — évènements, chocs géo & âges (voir scps_events.h)
 *
 * Rien d'aléatoire hors-sol : les chocs RELISENT la géo (relief tectonique,
 * rivières, pluie, routes), les évènements lisent la FICHE, les âges lisent
 * l'ÉTAT du monde. Les effets déplacent des coordonnées/métriques ; les textes
 * sont diégétiques (aucun nom SCPS).
 */
#include "scps_events.h"
#include <string.h>
#include <math.h>
#include <stdio.h>    /* SCPS_AGEDIAG (diag gated, motif SCPS_CAPDIAG) : fprintf(stderr,...) */
#include <stdlib.h>   /* SCPS_AGEDIAG : getenv */
#include "scps_tune.h"
#include "scps_math.h"      /* clampf/absf/xs32/frand partagés */
#include "scps_provlog.h"   /* journal provincial : on POUSSE les évènements EV_PROVINCE (display) */
#include "scps_factions.h"  /* MEMBRANE DE DÉCISION : faction_lever_apply/faction_concede (hooks de choix) */
#include "scps_religion.h"  /* CONTENU W2 (lot 2) §C : religion_schism_eligible (C1) */
#include "scps_agency.h"    /* CONTENU W2 (lot 2) §C : Edifice (EDI_SANCTUAIRE/TEMPLE/CATHEDRALE, C6) */
#include "scps_demography.h" /* ESCLAVAGE (A1) : demography_manumit_country (choix « Abolir ») */
#include "scps_lang.h"      /* lot M : tr() — le NOM du ministre (maison V2a) dans les titres de trahison */
#include "scps_fog.h"       /* raccord 6 : country_known_pair_share/country_knows (Découvertes) */
/* V2b LOT 2 relit statecraft_council_* — déjà visible via scps_statecraft.h (inclus
 * par scps_events.h) ; LOT 1 relit endgame_* — déjà visible via scps_endgame.h (idem). */

/* MEMBRANE DE DÉCISION — TÉLÉMÉTRIE (« la télémétrie est la preuve d'équilibre ») : combien
 * de fois la crise phare (et sa suite conséquente) ont tiré sur l'ensemble d'un run. Statics
 * de MODULE, RAZ à events_init (par partie/sim, comme g_wild_spawned), PAS sérialisés — la
 * chronique les lit pour son bilan (résolu par IA OU joueur, les deux passent par resolve_choice). */
static long g_marbrive_fired=0, g_pont_effondre_fired=0;
long events_marbrive_fired(void){ return g_marbrive_fired; }
long events_pont_effondre_fired(void){ return g_pont_effondre_fired; }

/* LOT F (2026-07-08) — CATASTROPHES DU MONDE CALME : combien de chocs géo (quake/
 * flood/drought/fire/plague, EVENTS[] existant) ont tiré alors que le monde était
 * jugé CALME (aucune fin en vue, cf. world_events_tick) — la preuve que la pression
 * mord sur les mondes qui, sinon, sommeilleraient un siècle sans réaction IA. */
static long g_calm_shocks_fired=0;
long events_calm_shocks_fired(void){ return g_calm_shocks_fired; }

/* CONTENU W1 — même motif (statics de module, RAZ à events_init, PAS sérialisés). */
static long g_cloches_fired=0, g_entrepots_fermes_fired=0, g_deux_cartes_fired=0,
            g_eau_noire_fired=0, g_derniere_decision_fired=0, g_salve_runique_fired=0;
long events_cloches_fired(void){ return g_cloches_fired; }
long events_entrepots_fermes_fired(void){ return g_entrepots_fermes_fired; }
long events_deux_cartes_fired(void){ return g_deux_cartes_fired; }
long events_eau_noire_fired(void){ return g_eau_noire_fired; }
long events_derniere_decision_fired(void){ return g_derniere_decision_fired; }
long events_salve_runique_fired(void){ return g_salve_runique_fired; }

/* CONTENU W2 (lot 2) — même motif (statics de module, RAZ à events_init, PAS sérialisés). */
static long g_chaines_rapportent_fired=0, g_oeuvre_noire_fired=0, g_savoir_interdit_fired=0,
            g_culte_imperial_fired=0, g_eveil_fired=0, g_foreuse_saigne_fired=0,
            g_droit_integration_fired=0, g_diaspora_comptoirs_fired=0,
            g_foi_fendre_fired=0, g_prophete_breche_fired=0, g_relique_douteuse_fired=0,
            g_remede_morts_fired=0, g_cellule_faubourgs_fired=0, g_fusils_reviennent_fired=0,
            g_savants_ennemi_fired=0, g_tarif_appris_fired=0;
long events_chaines_rapportent_fired(void){ return g_chaines_rapportent_fired; }
long events_oeuvre_noire_fired(void){ return g_oeuvre_noire_fired; }
long events_savoir_interdit_fired(void){ return g_savoir_interdit_fired; }
long events_culte_imperial_fired(void){ return g_culte_imperial_fired; }
long events_eveil_fired(void){ return g_eveil_fired; }
long events_foreuse_saigne_fired(void){ return g_foreuse_saigne_fired; }
long events_droit_integration_fired(void){ return g_droit_integration_fired; }
long events_diaspora_comptoirs_fired(void){ return g_diaspora_comptoirs_fired; }
long events_foi_fendre_fired(void){ return g_foi_fendre_fired; }
long events_prophete_breche_fired(void){ return g_prophete_breche_fired; }
long events_relique_douteuse_fired(void){ return g_relique_douteuse_fired; }
long events_remede_morts_fired(void){ return g_remede_morts_fired; }
long events_cellule_faubourgs_fired(void){ return g_cellule_faubourgs_fired; }
long events_fusils_reviennent_fired(void){ return g_fusils_reviennent_fired; }
long events_savants_ennemi_fired(void){ return g_savants_ennemi_fired; }
long events_tarif_appris_fired(void){ return g_tarif_appris_fired; }

/* V2b — même motif (statics de module, RAZ à events_init, PAS sérialisés). */
static long g_merv_fondation_fired=0, g_merv_sacrifice_fired=0, g_merv_ascension_fired=0,
            g_trahison_savoir_fired=0, g_trahison_societe_fired=0, g_trahison_industrie_fired=0,
            g_conseil_succession_fired=0, g_conseil_r1_fired=0, g_conseil_r2_fired=0,
            g_conseil_r3_fired=0, g_conseil_a1_fired=0, g_conseil_a2_fired=0, g_conseil_c1_fired=0,
            g_rivaux_voisins_fired=0, g_parente_lointaine_fired=0, g_marche_ethos_fired=0,
            g_tolerance_credo_fired=0, g_lettre_perime_fired=0, g_pratique_derive_fired=0;
long events_merv_fondation_fired(void){ return g_merv_fondation_fired; }
long events_merv_sacrifice_fired(void){ return g_merv_sacrifice_fired; }
long events_merv_ascension_fired(void){ return g_merv_ascension_fired; }
long events_trahison_savoir_fired(void){ return g_trahison_savoir_fired; }
long events_trahison_societe_fired(void){ return g_trahison_societe_fired; }
long events_trahison_industrie_fired(void){ return g_trahison_industrie_fired; }
long events_conseil_succession_fired(void){ return g_conseil_succession_fired; }
long events_conseil_r1_fired(void){ return g_conseil_r1_fired; }
long events_conseil_r2_fired(void){ return g_conseil_r2_fired; }
long events_conseil_r3_fired(void){ return g_conseil_r3_fired; }
long events_conseil_a1_fired(void){ return g_conseil_a1_fired; }
long events_conseil_a2_fired(void){ return g_conseil_a2_fired; }
long events_conseil_c1_fired(void){ return g_conseil_c1_fired; }
long events_rivaux_voisins_fired(void){ return g_rivaux_voisins_fired; }
long events_parente_lointaine_fired(void){ return g_parente_lointaine_fired; }
long events_marche_ethos_fired(void){ return g_marche_ethos_fired; }
long events_tolerance_credo_fired(void){ return g_tolerance_credo_fired; }
long events_lettre_perime_fired(void){ return g_lettre_perime_fired; }
long events_pratique_derive_fired(void){ return g_pratique_derive_fired; }

/* signe d'un effet pour le journal : +1 fléau · -1 faveur · 0 neutre */
static int ev_sign(const EvEffect *e){
    if (e->d_agitation>0.1f || e->d_L<-0.1f || e->pop_mult<0.999f) return +1;
    if (e->d_agitation<-0.1f || e->d_L>0.1f) return -1;
    return 0;
}
/* directions d'effet pour le HOVER du journal (2 bits/stat : 1 hausse · 2 baisse) */
static unsigned ev_effdir(const EvEffect *e){
    unsigned d=0;
    if (e->pop_mult   > 1.001f) d|=1u<<(2*JEFF_POP);    else if (e->pop_mult   < 0.999f) d|=2u<<(2*JEFF_POP);
    if (e->d_food_cap >  0.01f) d|=1u<<(2*JEFF_PROD);   else if (e->d_food_cap < -0.01f) d|=2u<<(2*JEFF_PROD);
    if (e->d_agitation>  0.1f)  d|=1u<<(2*JEFF_AGIT);   else if (e->d_agitation< -0.1f)  d|=2u<<(2*JEFF_AGIT);
    if (e->d_L        >  0.01f) d|=1u<<(2*JEFF_LEGIT);  else if (e->d_L        < -0.01f) d|=2u<<(2*JEFF_LEGIT);
    if (e->d_treasury >  0.1f)  d|=1u<<(2*JEFF_TRESOR); else if (e->d_treasury < -0.1f)  d|=2u<<(2*JEFF_TRESOR);
    return d;
}

/* ===================================================================== */
/* Le bundle de pointeurs systèmes passé aux triggers/effets             */
/* ===================================================================== */
struct EventCtx {
    EventsState     *ev;
    World           *w;
    WorldEconomy    *econ;
    WorldLegitimacy *wl;
    WorldProsperity *wp;
    Statecraft      *sc;
    RouteNetwork    *rn;
    const TechState *ts;
    DiploState      *dp;   /* §F : guerres (T) + rancune (Amnistie) ; peut être NULL */
    EndgameState    *eg;   /* V2b LOT 1 : la Merveille (merv/merv_country/metab_count) ; peut être NULL */
    int              human_player;   /* MEMBRANE DE DÉCISION : -1 = chronique (jamais enfilé) */
    /* raccord 7 (Âge des Héros) — MissionsState, pour poser le bonus « la prochaine
     * mission du siège » depuis resolve_choice (EVID_HERO_*). NULL pour TOUT autre
     * évènement (les ~15 initialisateurs positionnels existants omettent ce champ
     * en fin de liste ⇒ zéro-initialisé par construction — aucune retouche). */
    MissionsState   *ms;
};

/* §G2 — fwd : un fait NOTABLE inscrit une MÉMOIRE (l'âge en est un, défini plus haut
 * que le bloc directeur ⇒ on annonce le ressort de mémoire ici). */
static void dir_remember(Director *D, int day, DirMemKind kind, int subject, float weight);

/* ===================================================================== */
/* events_init — RELIRE la géo du worldgen en agrégats par région        */
/* ===================================================================== */
static float relief_weight(Biome b){
    switch(b){
        case BIO_VOLCANO:   return 1.4f;   /* faille active : le plus tremblant */
        case BIO_PEAK:      return 1.2f;
        case BIO_MOUNTAINS: return 1.0f;
        case BIO_HIGHLANDS: return 0.6f;
        case BIO_HILLS:     return 0.35f;
        default:            return 0.f;
    }
}
static bool is_forest(Biome b){ return b==BIO_FOREST||b==BIO_WOODS||b==BIO_JUNGLE||b==BIO_MANGROVE; }
static bool is_arid (Biome b){ return b==BIO_DESERT||b==BIO_DRYLANDS||b==BIO_SAVANNA||b==BIO_STEPPE||b==BIO_COASTAL_DESERT; }
static bool is_lowland(Biome b){ return b==BIO_PLAINS||b==BIO_FARMLAND||b==BIO_GRASSLAND||b==BIO_MARSH||b==BIO_MANGROVE||b==BIO_BOG; }

static void events_fire_caps_seed(EventsState *ev, uint32_t seed);  /* défini avec fire_event */
void events_init(EventsState *ev, const World *w, uint32_t seed){
    memset(ev,0,sizeof(*ev));
    ev->rng = seed ? seed : 0xA17F23C5u;
    events_fire_caps_seed(ev, ev->rng);   /* PLAFOND DE TIRS À VIE : 3-5 par évènement plafonné */
    /* ÂGES SANS ORDRE IMPOSÉ : AUCUNE chronologie fixe, AUCUN minimum d'Aube — chaque
     * âge devient éligible dès que son déclencheur matériel est vrai (dès l'an 0, en
     * théorie). year_eligible[a]=-1 = pas encore éligible ; last_dawn_year très bas
     * (jamais un vrai an de partie) pour ne pas bloquer le TOUT premier avènement. */
    for (int a=0;a<AGE_COUNT;a++) ev->ages.year_eligible[a] = -1;
    ev->ages.last_dawned = -1;
    ev->ages.last_dawn_year = -1000000;
    ev->last_id = -1; ev->last_name = NULL;
    g_marbrive_fired=0; g_pont_effondre_fired=0;   /* MEMBRANE DE DÉCISION : télémétrie RAZ par sim */
    g_calm_shocks_fired=0;   /* LOT F : catastrophes du monde calme, RAZ par sim */
    g_cloches_fired=0; g_entrepots_fermes_fired=0; g_deux_cartes_fired=0;
    g_eau_noire_fired=0; g_derniere_decision_fired=0; g_salve_runique_fired=0;   /* CONTENU W1 */
    g_chaines_rapportent_fired=0; g_oeuvre_noire_fired=0; g_savoir_interdit_fired=0;
    g_culte_imperial_fired=0; g_eveil_fired=0; g_foreuse_saigne_fired=0;
    g_droit_integration_fired=0; g_diaspora_comptoirs_fired=0;
    g_foi_fendre_fired=0; g_prophete_breche_fired=0; g_relique_douteuse_fired=0;
    g_remede_morts_fired=0; g_cellule_faubourgs_fired=0; g_fusils_reviennent_fired=0;
    g_savants_ennemi_fired=0; g_tarif_appris_fired=0;   /* CONTENU W2 (lot 2) */
    g_merv_fondation_fired=0; g_merv_sacrifice_fired=0; g_merv_ascension_fired=0;
    g_trahison_savoir_fired=0; g_trahison_societe_fired=0; g_trahison_industrie_fired=0;
    g_conseil_succession_fired=0; g_conseil_r1_fired=0; g_conseil_r2_fired=0; g_conseil_r3_fired=0;
    g_conseil_a1_fired=0; g_conseil_a2_fired=0; g_conseil_c1_fired=0;
    g_rivaux_voisins_fired=0; g_parente_lointaine_fired=0; g_marche_ethos_fired=0;
    g_tolerance_credo_fired=0; g_lettre_perime_fired=0; g_pratique_derive_fired=0;   /* V2b */

    /* accumulateurs par région */
    double sr_rain[SCPS_MAX_REG]={0}, sr_temp[SCPS_MAX_REG]={0}, sr_relief[SCPS_MAX_REG]={0};
    int    rmax_river[SCPS_MAX_REG]={0}, nforest[SCPS_MAX_REG]={0}, narid[SCPS_MAX_REG]={0}, nlow[SCPS_MAX_REG]={0};

    for (int i=0;i<SCPS_N;i++){
        const Cell *c=&w->cell[i];
        int r=c->region;
        if (r<0||r>=SCPS_MAX_REG) continue;
        RegionGeo *g=&ev->geo[r];
        g->cells++;
        sr_rain[r]   += c->rainfall;
        sr_temp[r]   += c->temperature;
        sr_relief[r] += relief_weight(c->biome);
        if (c->river > rmax_river[r]) rmax_river[r]=c->river;
        if (is_forest(c->biome)) nforest[r]++;
        if (is_arid(c->biome))   narid[r]++;
        if (is_lowland(c->biome))nlow[r]++;
    }
    for (int r=0;r<SCPS_MAX_REG;r++){
        RegionGeo *g=&ev->geo[r];
        if (g->cells<=0) continue;
        float n=(float)g->cells;
        g->rainfall = (float)(sr_rain[r]/n);
        g->temp     = (float)(sr_temp[r]/n);
        g->relief   = clampf((float)(sr_relief[r]/n), 0.f, 1.f);
        g->river01  = (float)rmax_river[r]/255.f;
        g->forest   = (float)nforest[r]/n;
        g->arid     = (float)narid[r]/n;
        g->lowland  = (float)nlow[r]/n;
    }
}

/* ===================================================================== */
/* LECTEURS DE RISQUE — relisent la géo ; 0 = la géographie l'interdit    */
/* ===================================================================== */
float events_quake_risk(const EventsState *ev, int region){
    if (region<0||region>=SCPS_MAX_REG) return 0.f;
    return clampf(ev->geo[region].relief, 0.f, 1.f);          /* la tectonique = le relief */
}
float events_flood_risk(const EventsState *ev, int region){
    if (region<0||region>=SCPS_MAX_REG) return 0.f;
    const RegionGeo *g=&ev->geo[region];
    return clampf(g->river01 * (0.35f + 0.65f*g->rainfall) * (0.3f + 0.7f*g->lowland), 0.f, 1.f);
}
float events_drought_risk(const EventsState *ev, int region){
    if (region<0||region>=SCPS_MAX_REG) return 0.f;
    const RegionGeo *g=&ev->geo[region];
    float dry = (1.f - g->rainfall) * (0.5f + 0.5f*g->temp);
    return clampf(dry * (0.35f + 0.65f*g->arid), 0.f, 1.f);
}
float events_fire_risk(const EventsState *ev, int region){
    if (region<0||region>=SCPS_MAX_REG) return 0.f;
    const RegionGeo *g=&ev->geo[region];
    if (g->forest < 0.01f) return 0.f;   /* K4a : un feu de FORÊT exige de la forêt — STRICTEMENT nul
                                          * sous une couverture négligeable (une trace d'arbres ne brûle pas
                                          * en incendie de forêt ; un feu de steppe/urbain serait un AUTRE événement). */
    float dry = (1.f - g->rainfall) * (0.5f + 0.5f*g->temp);
    return clampf(g->forest * dry, 0.f, 1.f);
}
static float shock_risk(const EventsState *ev, int region, int shock){
    switch(shock){
        case EVID_QUAKE:   return events_quake_risk(ev,region);
        case EVID_FLOOD:   return events_flood_risk(ev,region);
        case EVID_DROUGHT: return events_drought_risk(ev,region);
        case EVID_FIRE:    return events_fire_risk(ev,region);
        default:           return 0.f;
    }
}

/* ===================================================================== */
/* TRIGGERS — chocs (géo) & politiques (fiche)                           */
/* ===================================================================== */
static bool trig_quake  (const EventCtx *cx,int r){ return events_quake_risk  (cx->ev,r)>0.08f; }
static bool trig_flood  (const EventCtx *cx,int r){ return events_flood_risk  (cx->ev,r)>0.08f; }
static bool trig_drought(const EventCtx *cx,int r){ return events_drought_risk(cx->ev,r)>0.10f; }
static bool trig_fire   (const EventCtx *cx,int r){ return events_fire_risk   (cx->ev,r)>0.10f; }

/* Base commune de l'évènement d'intégration : une région avalée, lointaine,
 * fraîchement tenue, dont l'agitation monte. */
static bool integ_base(const EventCtx *cx, int r){
    if (r<0||r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    const PopCulture *rul=econ_ruling_culture(cx->w,cx->econ,re->owner);
    if (!rul) return false;
    float dinf = econ_content_dist(&re->culture, rul);
    float yh   = (r<SCPS_MAX_REG)?cx->wl->years_held[r]:50.f;
    float agit = (cx->sc && r<SCPS_MAX_REG)?cx->sc->agitation[r]:0.f;
    return dinf > 6.f && yh < 30.f && agit > 40.f;
}
static Ethos  owner_ethos (const EventCtx *cx,int r){ const PopCulture*p=econ_ruling_culture(cx->w,cx->econ,cx->econ->region[r].owner); return p?p->ethos:ETHOS_ORDRE; }
static Sphere owner_sphere(const EventCtx *cx,int r){ const PopCulture*p=econ_ruling_culture(cx->w,cx->econ,cx->econ->region[r].owner); return p?heritage_sphere(p->heritage):SPHERE_HOMMES; }

static bool trig_integ_dom(const EventCtx *cx,int r){
    if (!integ_base(cx,r)) return false;
    Ethos e=owner_ethos(cx,r);
    return e==ETHOS_DOMINATEUR||e==ETHOS_HONNEUR;
}
static bool trig_integ_merc(const EventCtx *cx,int r){
    return integ_base(cx,r) && owner_ethos(cx,r)==ETHOS_MERCANTILE;
}
static bool trig_integ_bur(const EventCtx *cx,int r){
    if (!integ_base(cx,r)) return false;
    Ethos e=owner_ethos(cx,r);
    return e==ETHOS_BUREAUCRATE||e==ETHOS_ORDRE;
}
static bool trig_integ_anc(const EventCtx *cx,int r){
    return integ_base(cx,r) && owner_sphere(cx,r)==SPHERE_ANCIENS;
}

/* Succession (pays) : une couronne mal assise. */
static bool trig_succession(const EventCtx *cx,int c){
    if(c<0||c>=cx->w->n_countries) return false;
    if(cx->wp && c<cx->wp->n_countries) return cx->wp->country[c].L < 5.0f;
    return false;
}
/* Schisme (pays) : un credo zélé sur une foi qui diverge. */
static bool trig_schism(const EventCtx *cx,int c){
    const PopCulture *rul=econ_ruling_culture(cx->w,cx->econ,c);
    if(!rul) return false;
    if(rul->credo==CREDO_PLURALISTE) return false;
    /* divergence religieuse interne : une région du pays s'écarte de la foi du trône */
    for(int r=0;r<cx->econ->n_regions;r++) if(cx->econ->region[r].owner==c && cx->econ->region[r].culture.settled){
        if(absf(cx->econ->region[r].culture.religion - rul->religion) > 4.f) return true;
    }
    return false;
}
/* Floraison COSMOPOLITE (province) : plusieurs cultures CONVERGENT (forte minorité installée)
 * sous un éthos ACCUEILLANT (Bureaucrate « tient la diversité » · Mercantile « carrefours » ·
 * Pacifiste « ne fracture jamais »), et la province est APAISÉE (la diversité s'intègre au lieu
 * de fracturer) → le creuset porte ses fruits. Le récit suit la FICHE (l'éthos), comme tout le
 * module ; les éthos xénophobes (Dominateur/Honneur, mauvais intégrateurs) n'y ont pas droit. */
static bool trig_xenophile(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    Ethos e=owner_ethos(cx,r);
    if (e!=ETHOS_BUREAUCRATE && e!=ETHOS_MERCANTILE && e!=ETHOS_PACIFISTE) return false;
    if (econ_off_culture_fraction(&re->pop) < 0.20f) return false;   /* convergence RÉELLE, pas un monolithe */
    float agit=(cx->sc && r<SCPS_MAX_REG)?cx->sc->agitation[r]:0.f;
    return agit < 30.f && re->satisfaction > 0.55f;                  /* la diversité INTÈGRE */
}
/* Miroir XÉNOPHOBE (province) : la cohésion par la MÉTABOLISATION. Symétrique du creuset, mais
 * l'éthos MARTIAL (Dominateur « conquête » · Honneur « gloire », les mauvais intégrateurs) ne
 * GARDE pas la diversité : il la DIGÈRE. Il faut donc qu'il y ait EU de la diversité (plusieurs
 * peuples, off-culture réelle) ET qu'elle soit DIGÉRÉE — les minorités assimilées en profondeur
 * (intégration pop-pondérée haute, `g->integration`, qui met des décennies à monter → le tirage
 * est TARDIF, le déterminisme court terme tient). Les conquis sont devenus un seul sang. */
static bool trig_xenophobe(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    Ethos e=owner_ethos(cx,r);
    if (e!=ETHOS_DOMINATEUR && e!=ETHOS_HONNEUR) return false;
    const ProvincePop *pp=&re->pop;
    if (pp->n_groups < 2 || econ_off_culture_fraction(pp) < 0.15f) return false;  /* de la diversité À DIGÉRER */
    /* MÉTABOLISATION = intégration pop-pondérée des MINORITÉS (non-dominantes). Haute → digérées. */
    int dom=0; long best=pp->groups[0].count;
    for (int i=1;i<pp->n_groups;i++) if (pp->groups[i].count>best){ best=pp->groups[i].count; dom=i; }
    double w=0.0, t=0.0;
    for (int i=0;i<pp->n_groups;i++) if (i!=dom){ w+=(double)pp->groups[i].count; t+=(double)pp->groups[i].count*pp->groups[i].integration; }
    if (w < 1.0) return false;
    float metab=(float)(t/w);
    return metab > 0.75f && re->satisfaction > 0.55f;   /* les peuples conquis fondus en un seul */
}

/* MARBRIVE (province) : le contremaître réclame — une province AGITÉE (soutenue,
 * pas un pic), BÂTIE (des institutions à sauvegarder/à perdre) et déjà COERCITIVE
 * (RELOC_COERCION_BASE=0.25, la ligne de base d'un pic de relocalisation — au-delà,
 * la corde est déjà tendue). Les trois lecture directes de l'agrégat région (SAFE :
 * build.K_inst/coercion sont des LECTURES, jamais une écriture hors province-owned). */
static bool trig_marbrive(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    return statecraft_agitation(cx->sc, r) >= 55
        && re->build.K_inst >= 1.0f
        && re->coercion     >= 0.25f;
}
/* PONT EFFONDRÉ (province) : la suite CONSÉQUENTE du choix « Envoyer les prévôts »
 * de Marbrive — le sabotage qu'on n'a pas vu venir mûrit en catastrophe (délai posé
 * au push, 180-540 j). CONSOMME la cicatrice au tir (une cicatrice = un tir). */
static bool trig_pont_effondre(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    return decision_memory_has_ripe(cx->ev, r, SCAR_SABOTAGE_CHANTIER, cx->ev->ages.days_elapsed);
}

/* ═══════════════════════════════════════════════════════════════════
 * CONTENU W1 — six triggers neufs (voir la table EVENTS[] pour les ancres).
 * ═══════════════════════════════════════════════════════════════════ */

/* CLOCHES (province) : la surtaxe fait taire les cloches — une province SURTAXÉE
 * (ressentie), PEU SATISFAITE, et PEU LÉGITIME (le consentement régional flanche). */
static bool trig_cloches(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions || r>=SCPS_MAX_REG) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    float L = cx->wl ? cx->wl->L[r] : 10.f;
    return re->over_tax > 0.15f && re->satisfaction < 0.50f && L < 5.5f;
}
/* ENTREPÔTS FERMÉS (province) : un carrefour SATURÉ (route_pe haut) dont le grief
 * marchand du pays propriétaire déborde — les Marchands ferment boutique. */
static bool trig_entrepots_fermes(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    if (re->route_pe <= 0.8f) return false;
    return faction_grievance(re->owner, FAC_MARCHAND) > 0.35f;
}
/* DEUX CARTES (province) : une conquête RÉCENTE (peu d'ancienneté de tutelle — le
 * substitut documenté pour « years_held » : `wl->years_held[r]` EXISTE et porte
 * exactement cette ancienneté, cf. son usage dans integ_base ci-dessus) sur un
 * carrefour actif et déjà AGITÉ — la carte de la capitale et celle du terrain ne
 * s'accordent pas encore, et ça se voit au double péage. */
static bool trig_deux_cartes(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    float yh = (r<SCPS_MAX_REG) ? cx->wl->years_held[r] : 50.f;
    float agit = (cx->sc && r<SCPS_MAX_REG) ? cx->sc->agitation[r] : 0.f;
    return yh < 15.f && re->route_pe > 0.4f && agit >= 35.f;
}
/* EAU NOIRE (province) : le présage du directeur-amplitude (le monde « vibre ») se
 * lit dans le puits — une province NOURRIE (grenier bâti) où l'amplitude dramatique
 * est déjà haute voit son eau tourner noire. */
static bool trig_eau_noire(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    return director_amplitude(cx->ev) > 0.45f && re->build.food_cap >= 1.0f;
}
/* DERNIÈRE DÉCISION (province) : une cicatrice NON-mûre (posée par un choix passé,
 * qui n'a pas encore mûri) hante encore cette province COERCITIVE — le passé n'a
 * pas fini d'arriver. */
static bool trig_derniere_decision(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    if (!decision_memory_pending(cx->ev, r, cx->ev->ages.days_elapsed)) return false;
    return re->coercion > 0.30f;
}
/* SALVE RUNIQUE (pays) : la première fois que l'apex Arquebuse runique (Méca ×
 * Métal × Éso, tier-5) est déverrouillé — déclenchement UNIQUE (le cooldown
 * énorme du hook, 36500 j, fait le reste : un pays qui a déjà répondu à sa
 * première salve ne re-tire jamais). */
static bool trig_salve_runique(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->ts) return false;
    return cx->ts[c].unlocked[TECH_APEX_ARQUEBUSE];
}

/* ═══════════════════════════════════════════════════════════════════
 * CONTENU W2 (lot 2) — §A tech (déclenchement unique) · §B culturel ·
 * §C religieux · §D chaînage de cicatrices. Voir la table EVENTS[] pour les
 * ancres de calibrage (mêmes fourchettes que W1/Marbrive, comparées à la
 * même table existante).
 * ═══════════════════════════════════════════════════════════════════ */

/* Helper commun §A : « la tech `tid` vient d'être déverrouillée quelque part »
 * (déclenchement UNIQUE, motif SALVE_RUNIQUE) — pays `c` porte `unlocked[tid]`. */
static bool trig_tech_unique(const EventCtx *cx, int c, int tid){
    if (c<0 || c>=cx->w->n_countries || !cx->ts) return false;
    return cx->ts[c].unlocked[tid];
}
static bool trig_chaines_rapportent(const EventCtx *cx,int c){ return trig_tech_unique(cx,c,TECH_ESCLAVAGE); }
static bool trig_oeuvre_noire      (const EventCtx *cx,int c){ return trig_tech_unique(cx,c,TECH_OEUVRE_NOIRE); }
static bool trig_savoir_interdit   (const EventCtx *cx,int c){ return trig_tech_unique(cx,c,TECH_SAVOIR_INTERDIT); }
static bool trig_culte_imperial    (const EventCtx *cx,int c){ return trig_tech_unique(cx,c,TECH_CULTE_IMPERIAL); }
static bool trig_eveil             (const EventCtx *cx,int c){ return trig_tech_unique(cx,c,TECH_EVEIL); }

/* A6 — LA FOREUSE MORD (province) : le pays possède la tech, et CETTE région
 * porte une foreuse (BLD_FOREUSE) déjà bâtie — la mine saigne, littéralement. */
static bool trig_foreuse_saigne(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    if (!cx->ts || re->owner<0 || re->owner>=cx->w->n_countries || !cx->ts[re->owner].unlocked[TECH_FOREUSE]) return false;
    for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==BLD_FOREUSE) return true;
    return false;
}

/* B1 — LE DROIT D'INTÉGRATION DIVISE (pays) : tech débloquée + une moyenne
 * pays (pop-pondérée sur ses régions) d'off-culture > seuil — la fraction
 * DÉJÀ MINORITAIRE (non digérée) dépasse un quart de la population. */
static float country_mean_off_culture(const EventCtx *cx, int cid){
    double num=0.0, den=0.0;
    for (int r=0;r<cx->econ->n_regions;r++){
        const RegionEconomy *re=&cx->econ->region[r];
        if (re->owner!=cid || !re->culture.settled) continue;
        float pop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        if (pop<=0.f) continue;
        num += (double)pop * (double)econ_off_culture_fraction(&re->pop);
        den += (double)pop;
    }
    return (den>1.0) ? (float)(num/den) : 0.f;
}
static bool trig_droit_integration(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->ts) return false;
    if (!cx->ts[c].unlocked[TECH_INTEGRATION]) return false;
    return country_mean_off_culture(cx,c) > 0.25f;
}

/* B4 — LA DIASPORA TIENT LES COMPTOIRS (pays) : fraction diaspora du pays
 * (Σ groupes diaspora / Σ pop, sur ses régions) > seuil, ET un route_pe moyen
 * marqué (des comptoirs qui comptent vraiment). */
static float country_diaspora_fraction(const EventCtx *cx, int cid){
    double dia=0.0, tot=0.0;
    for (int r=0;r<cx->econ->n_regions;r++){
        const RegionEconomy *re=&cx->econ->region[r];
        if (re->owner!=cid || !re->culture.settled) continue;
        const ProvincePop *pp=&re->pop;
        for (int i=0;i<pp->n_groups;i++){
            tot += (double)pp->groups[i].count;
            if (pp->groups[i].diaspora) dia += (double)pp->groups[i].count;
        }
    }
    return (tot>1.0) ? (float)(dia/tot) : 0.f;
}
static bool trig_diaspora_comptoirs(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    if (country_diaspora_fraction(cx,c) <= 0.15f) return false;
    double sum_pe=0.0; int n=0;
    for (int r=0;r<cx->econ->n_regions;r++) if (cx->econ->region[r].owner==c && cx->econ->region[r].culture.settled){
        sum_pe += (double)cx->econ->region[r].route_pe; n++;
    }
    return n>0 && (float)(sum_pe/n) > 0.4f;
}

/* C1 — LA FOI VA SE FENDRE (pays) : religion_schism_eligible!=RSE_NONE. Le
 * choix pose des deltas ; l'appel direct au schisme reste la voie normale
 * (l'éligibilité persiste — voir le commentaire dans la table EVENTS[]). */
static bool trig_foi_fendre(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return religion_schism_eligible(cx->w, cx->econ, cx->wl, c) != RSE_NONE;
}

/* C5 — LA BRÈCHE A TROUVÉ SON PROPHÈTE (pays) : pression faustienne du pays
 * (Σ faust_charge de ses régions — l'accumulateur EXISTANT, FAU0/FAU1) haute,
 * ET la faction Transgresseur n'est PAS déjà comblée (grievance basse = elle
 * n'a pas encore eu gain de cause — le prophète a un terreau). */
static float country_faust_charge(const EventCtx *cx, int cid){
    float s=0.f;
    for (int r=0;r<cx->econ->n_regions;r++)
        if (cx->econ->region[r].owner==cid) s += cx->econ->region[r].faust_charge;
    return s;
}
static bool trig_prophete_breche(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    if (country_faust_charge(cx,c) <= 3.0f) return false;
    return faction_grievance(c, FAC_TRANSGRESSEUR) < 0.25f;
}

/* C6 — LA RELIQUE FAIT DES MIRACLES DOUTEUX (province) : au moins un édifice
 * de foi bâti ICI (sanctuaire/temple/cathédrale), et le directeur-amplitude
 * (le monde « vibre ») est déjà haut — le miracle arrive à point nommé. */
static bool trig_relique_douteuse(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    uint32_t mask=(1u<<EDI_SANCTUAIRE)|(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE);
    if (!(re->edi_built & mask)) return false;
    return director_amplitude(cx->ev) > 0.30f;
}

/* ═══ §D — CHAÎNAGE DE CICATRICES : motif EVID_PONT_EFFONDRE (has_ripe, la
 * résolution CONSOMME). Chaque trigger relit une cicatrice posée ailleurs. ═══ */
static bool trig_remede_morts     (const EventCtx *cx,int r){ return r>=0 && r<cx->econ->n_regions && decision_memory_has_ripe(cx->ev, r, SCAR_SCANDALE_SANITAIRE, cx->ev->ages.days_elapsed); }
static bool trig_cellule_faubourgs(const EventCtx *cx,int r){ return r>=0 && r<cx->econ->n_regions && decision_memory_has_ripe(cx->ev, r, SCAR_RADICALISATION,    cx->ev->ages.days_elapsed); }
/* K5/K6/K7 — la cicatrice est posée sur le `subject` EXACT de l'évènement d'origine
 * (decision_memory_push(ev, subject, kind, …) dans apply_choice_hook — subject est
 * celui passé à resolve_choice, JAMAIS remappé sur une capitale). Ça dépend du SCOPE
 * de l'évènement qui a posé la cicatrice :
 *  - SCAR_PROLIFERATION_ARME est posée par SALVE_RUNIQUE (EV_COUNTRY, subject=cid) → on
 *    relit directement sur `c` (le pays).
 *  - SCAR_FUITE_CERVEAUX est posée par SAVOIR_INTERDIT (EV_COUNTRY, subject=cid, ce lot) →
 *    idem, relue sur `c`.
 *  - SCAR_EXEMPTION_ACHETEE est posée par DEUX_CARTES (EV_PROVINCE, subject=région) → K7
 *    reste donc un évènement PROVINCIAL (pas pays), pour relire la MÊME clé de sujet. */
static bool trig_fusils_reviennent(const EventCtx *cx,int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return decision_memory_has_ripe(cx->ev, c, SCAR_PROLIFERATION_ARME, cx->ev->ages.days_elapsed);
}
static bool trig_savants_ennemi(const EventCtx *cx,int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return decision_memory_has_ripe(cx->ev, c, SCAR_FUITE_CERVEAUX, cx->ev->ages.days_elapsed);
}
static bool trig_tarif_appris(const EventCtx *cx,int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    return decision_memory_has_ripe(cx->ev, r, SCAR_EXEMPTION_ACHETEE, cx->ev->ages.days_elapsed);
}

/* ═══════════════════════════════════════════════════════════════════
 * V2b LOT 1 — LA MERVEILLE EN 3 ÉTAPES (pays, joueur SEUL — human gate).
 * FONDATION : tire quand le JOUEUR devient éligible au palier 1
 * (metab_count≥3 && merv==MERV_NONE). CONSTRUCTION (sacrifice) : récurrent
 * pendant que merv est actif (ni NONE ni ASCENDED). ASCENSION : une fois, à
 * MERV_SAVOIR_DONE (juste avant que wonder_tick ne bascule tout seul en
 * MERV_ASCENDED — le dernier choix intercepte CE palier précis). Les trois
 * sont HUMAN-ONLY par construction (`cx->eg` n'existe QUE pour la voie
 * joueur en pratique — la chronique appelle world_events_tick avec eg=NULL,
 * cf. sim_day) mais on vérifie aussi human_player>=0 explicitement : un banc
 * qui passerait un eg non-NULL en chronique (human=-1) ne doit jamais fonder
 * la Merveille à la place de l'IA (elle ne la poursuit pas, cf. scps_endgame.h).
 * ═══════════════════════════════════════════════════════════════════ */
static bool trig_merv_fondation(const EventCtx *cx, int c){
    if (!cx->eg || cx->human_player<0 || c!=cx->human_player) return false;
    if (cx->eg->merv != MERV_NONE) return false;
    return endgame_metab_count(cx->w, cx->econ, c) >= 3;
}
static bool trig_merv_sacrifice(const EventCtx *cx, int c){
    if (!cx->eg || cx->human_player<0 || c!=cx->human_player) return false;
    if (cx->eg->merv_country != c) return false;
    return cx->eg->merv!=MERV_NONE && cx->eg->merv!=MERV_ASCENDED && cx->eg->merv!=MERV_SAVOIR_DONE;
}
static bool trig_merv_ascension(const EventCtx *cx, int c){
    if (!cx->eg || cx->human_player<0 || c!=cx->human_player) return false;
    if (cx->eg->merv_country != c) return false;
    return cx->eg->merv == MERV_SAVOIR_DONE;
}

/* ═══════════════════════════════════════════════════════════════════
 * V2b LOT 2 — LE CONSEIL (signaux V2a : statecraft_council_betrayal_ready /
 * _pair_state). human gate : l'IA remplace déjà son ministre au bord (cf.
 * statecraft_council_ai_replace_count) — ces évènements sont donc, comme
 * la Merveille, une décision QUE LE JOUEUR doit prendre lui-même.
 * ═══════════════════════════════════════════════════════════════════ */
static bool trig_trahison_seat(const EventCtx *cx, int c, int seat){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    return statecraft_council_betrayal_ready(cx->sc, c, seat);
}
static bool trig_trahison_savoir   (const EventCtx *cx,int c){ return trig_trahison_seat(cx,c,0); }
static bool trig_trahison_societe  (const EventCtx *cx,int c){ return trig_trahison_seat(cx,c,1); }
static bool trig_trahison_industrie(const EventCtx *cx,int c){ return trig_trahison_seat(cx,c,2); }

/* raccord 7 — L'ÂGE DES HÉROS : « Le nom du siècle » n'est JAMAIS scanné par le
 * balayage mtth (le fait est déjà avéré au moment de l'appel — ages_hero_fire
 * appelle fire_event DIRECTEMENT) ; ce trigger neutre n'existe que parce que
 * EventDef.trigger n'est pas un pointeur optionnel dans la table. */
static bool trig_never(const EventCtx *cx, int c){ (void)cx;(void)c; return false; }

/* SUCCESSION — un ministre LOYAL (le seuil-miroir de betrayal_ready : PAS à
 * l'agonie) en poste depuis plus de SC_COUNCIL_GEN_YEARS (20 ans, la longueur
 * d'une génération de pool — la retraite naturelle documentée en scps_statecraft.h,
 * "premier départ possible an 16 > fenêtre golden"). Un seul siège au hasard
 * déterministe (balayage croissant, le premier éligible) — un règne ne voit
 * pas trois départs le même jour. */
static bool trig_conseil_succession(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    int year = cx->ev->ages.days_elapsed/365;
    for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
        int slot = statecraft_council_seated(cx->sc, c, seat);
        if (slot<0) continue;
        if (statecraft_council_betrayal_ready(cx->sc, c, seat)) continue;   /* SUCCESSION = FIN de carrière, pas une trahison */
        int age = statecraft_council_seated_age(cx->sc, cx->w->seed, c, seat, year);
        if (age >= 62) return true;
    }
    return false;
}
/* R1/R2/R3 — INTER-CONSEILLERS EN RIVALITÉ : deux sièges pourvus du MÊME pays
 * en COUNCIL_PAIR_RIVALITE (factions opposées, tous deux enracinés — cf.
 * statecraft_council_pair_state). R1=Savoir(0)/Société(1) · R2=Industrie(2)/
 * Société(1) · R3=Savoir(0)/Industrie(2). */
static bool trig_conseil_pair(const EventCtx *cx, int c, int a, int b, CouncilPairState want){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) return false;
    int year = cx->ev->ages.days_elapsed/365;
    return statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == want;
}
static bool trig_conseil_r1(const EventCtx *cx,int c){ return trig_conseil_pair(cx,c,0,1,COUNCIL_PAIR_RIVALITE); }
static bool trig_conseil_r2(const EventCtx *cx,int c){ return trig_conseil_pair(cx,c,2,1,COUNCIL_PAIR_RIVALITE); }
static bool trig_conseil_r3(const EventCtx *cx,int c){ return trig_conseil_pair(cx,c,0,2,COUNCIL_PAIR_RIVALITE); }
/* A1 — l'ALLIANCE de deux sièges (COUNCIL_PAIR_ALLIANCE, n'importe quelle paire
 * pourvue — balayage déterministe des 3 paires possibles, la première trouvée). */
static bool trig_conseil_a1(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    static const int PAIRS[3][2]={{0,1},{1,2},{0,2}};
    int year = cx->ev->ages.days_elapsed/365;
    for (int i=0;i<3;i++){
        int a=PAIRS[i][0], b=PAIRS[i][1];
        if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) continue;
        if (statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == COUNCIL_PAIR_ALLIANCE)
            return true;
    }
    return false;
}
/* A2 — leur candidat au 3e siège : une alliance de sièges (comme A1) EXISTE
 * déjà (deux sièges pourvus alliés) et le 3e siège restant est VACANT — un
 * strict sous-cas d'A1 (pas de nouveau signal moteur), sur le siège qui manque. */
static bool trig_conseil_a2(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    static const int PAIRS[3][2]={{0,1},{1,2},{0,2}};
    static const int THIRD[3]={2,0,1};
    int year = cx->ev->ages.days_elapsed/365;
    for (int i=0;i<3;i++){
        int a=PAIRS[i][0], b=PAIRS[i][1];
        if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) continue;
        if (statecraft_council_seated(cx->sc,c,THIRD[i])>=0) continue;   /* le 3e doit être VACANT */
        if (statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == COUNCIL_PAIR_ALLIANCE)
            return true;
    }
    return false;
}
/* C1 — LA CONSPIRATION : une paire en COUNCIL_PAIR_CONSPIRATION (les DEUX
 * factions aliénées — grief haut des deux côtés). */
static bool trig_conseil_c1(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries || !cx->sc) return false;
    if (cx->human_player<0 || c!=cx->human_player) return false;
    static const int PAIRS[3][2]={{0,1},{1,2},{0,2}};
    int year = cx->ev->ages.days_elapsed/365;
    for (int i=0;i<3;i++){
        int a=PAIRS[i][0], b=PAIRS[i][1];
        if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) continue;
        if (statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == COUNCIL_PAIR_CONSPIRATION)
            return true;
    }
    return false;
}
/* trouve la première paire dans l'état demandé (miroir des triggers pair —
 * utilisé par resolve_choice pour savoir QUELS sièges renvoyer/rééquilibrer). */
static bool conseil_pair_find(const EventCtx *cx, int c, CouncilPairState want, int *out_a, int *out_b){
    static const int PAIRS[3][2]={{0,1},{1,2},{0,2}};
    int year = cx->ev->ages.days_elapsed/365;
    for (int i=0;i<3;i++){
        int a=PAIRS[i][0], b=PAIRS[i][1];
        if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) continue;
        if (statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == want){
            *out_a=a; *out_b=b; return true;
        }
    }
    return false;
}
/* P2 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md §« Événements Conseil ») — les biais
 * DYNAMIQUES appliqués EN PLUS de ce que statecraft_council_dismiss/_hire appliquent
 * déjà eux-mêmes (dismiss grief +COUNCIL_DISMISS_GRIEF≈0.10 sur SA PROPRE faction ;
 * hire lève +COUNCIL_HIRE_LEVER≈0.10 sur la faction du candidat) — ces deux-là ne sont
 * JAMAIS redupliqués ici. */
#define COUNCIL_HOOK_OPPOSED_BIAS 0.10f   /* renvoi "dur" (Trahison) / rivalité tranchée (R1-R3) : Opp(F) ou F avance */
#define COUNCIL_HOOK_KEEP_BIAS    0.05f   /* Trahison "composer"/"négocier" : F avance un peu (gardé, pas renvoyé) */
#define COUNCIL_HOOK_ALLY_BIAS    0.05f   /* A1 "laisser faire" (aux deux alliées) / "contrebalancer" (au 3e siège) */
#define COUNCIL_HOOK_RETIRE_BIAS  0.05f   /* Succession "remercier publiquement" */
/* miroir de trig_conseil_a2 : la paire alliée + le 3e siège VACANT (celui que
 * "Accepter leur candidat" va pourvoir). Re-scanné à la résolution (le monde a pu
 * bouger depuis le trigger — même discipline que conseil_pair_find). */
static bool conseil_a2_find(const EventCtx *cx, int c, int *out_a, int *out_b, int *out_third){
    static const int PAIRS[3][2]={{0,1},{1,2},{0,2}};
    static const int THIRD[3]={2,0,1};
    int year = cx->ev->ages.days_elapsed/365;
    for (int i=0;i<3;i++){
        int a=PAIRS[i][0], b=PAIRS[i][1];
        if (statecraft_council_seated(cx->sc,c,a)<0 || statecraft_council_seated(cx->sc,c,b)<0) continue;
        if (statecraft_council_seated(cx->sc,c,THIRD[i])>=0) continue;
        if (statecraft_council_pair_state(cx->sc, cx->w, cx->econ, cx->w->seed, c, a, b, year) == COUNCIL_PAIR_ALLIANCE){
            *out_a=a; *out_b=b; *out_third=THIRD[i]; return true;
        }
    }
    return false;
}
/* miroir de trig_conseil_succession : le premier siège pourvu, PAS à l'agonie
 * (betrayal_ready), dont le titulaire a ≥62 ans — celui que la SUCCESSION retire. */
static int conseil_succession_seat_find(const EventCtx *cx, int c){
    int year = cx->ev->ages.days_elapsed/365;
    for (int seat=0; seat<SC_COUNCIL_SEATS; seat++){
        if (statecraft_council_seated(cx->sc, c, seat)<0) continue;
        if (statecraft_council_betrayal_ready(cx->sc, c, seat)) continue;
        if (statecraft_council_seated_age(cx->sc, cx->w->seed, c, seat, year) >= 62) return seat;
    }
    return -1;
}
/* P2 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) — biaise la faction RÉELLE du
 * titulaire de `seat` (no-op si vacant : jamais un biais fantôme). Le pas commun
 * de « Trancher pour X » (R1/R2/R3), « Laisser faire »/« Contrebalancer » (A1) et
 * « Remercier publiquement » (Succession, appelé APRÈS lecture — cf. le bloc). */
static void council_lever_seat(const EventCtx *cx, int cid, int seat, float strength){
    int fac = statecraft_council_seat_faction(cx->sc, cx->w->seed, cid, seat);
    if (fac>=0) faction_lever_apply(cid, (EthosFaction)fac, strength);
}

/* ═══════════════════════════════════════════════════════════════════
 * V2b LOT 3 — LE CONTENU DÉBLOQUÉ (lecteurs P7a) : B2/B3 (culture_relation_of),
 * B6 (region_ethos_drift), C2 (religion_fracture_level), C3 (religion_scholar_
 * drift), C4 (religion_credo_drift). EV_COUNTRY (pays) sauf B6 (province).
 * ═══════════════════════════════════════════════════════════════════ */
/* B2 — DEUX CULTURES VOISINES NE S'ACCORDENT PLUS : la capitale du pays et une
 * région ÉTRANGÈRE voisine (adjacence, un AUTRE owner) sont en relation RIVALE
 * OU SCHISMATIQUE (culture_relation_of, champs nus — déballés depuis les deux
 * PopCulture). Rivalité de VOISINAGE (≠ B3, la parenté LOINTAINE ci-dessous). */
static bool country_has_border_relation(const EventCtx *cx, int cid, CultureRelation want, bool adjacent_only){
    int capr = world_capital_region(cx->w, cid);
    if (capr<0 || capr>=cx->econ->n_regions || !cx->econ->region[capr].culture.settled) return false;
    const PopCulture *home = &cx->econ->region[capr].culture;
    for (int r=0;r<cx->econ->n_regions;r++){
        const RegionEconomy *re=&cx->econ->region[r];
        if (re->owner<0 || re->owner==cid || !re->culture.settled) continue;
        if (adjacent_only && !(capr<SCPS_MAX_REG && r<SCPS_MAX_REG && cx->econ->adj[capr][r])) continue;
        const PopCulture *away=&re->culture;
        CultureRelation rel = culture_relation_of(
            home->langue, home->valeurs, home->subsistance, home->parente, home->religion, home->credo, home->rel_branch,
            away->langue, away->valeurs, away->subsistance, away->parente, away->religion, away->credo, away->rel_branch);
        if (rel==want) return true;
    }
    return false;
}
static bool trig_rivaux_voisins(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    /* ENNEMIS_SCHISME : branche religieuse proche + prosélytisme haut — le plus
     * âpre des désaccords de VOISINAGE (deux crédos cousins qui se disputent le
     * même fidèle, à la frontière). */
    return country_has_border_relation(cx, c, REL_ENNEMIS_SCHISME, true);
}
/* B3 — UNE PARENTÉ LOINTAINE SE SOUVIENT : une région étrangère NON adjacente
 * (loin, aucune frontière commune) mais culturellement PROCHE (REL_PARENTS —
 * horloge ET contenu proches, culture_relation_of) — les cousins d'outre-monde
 * qui n'ont jamais partagé de frontière. */
static bool trig_parente_lointaine(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return country_has_border_relation(cx, c, REL_PARENTS, false);
}
/* B6 — UNE RÉGION MARCHE LOIN DE L'ÉTHOS RÉGNANT (province) : region_ethos_drift
 * (l'éthos DOMINANT local vs le régnant de la capitale) au-delà d'un seuil — une
 * marche qui ne pense plus comme le trône, même sans off-culture (c'est l'ÉTHOS
 * qui dérive, pas la culture entière — orthogonal à XENOPHILE/XENOPHOBE). */
static bool trig_marche_ethos(const EventCtx *cx, int r){
    if (r<0 || r>=cx->econ->n_regions) return false;
    const RegionEconomy *re=&cx->econ->region[r];
    if (!re->culture.settled || re->owner<0) return false;
    Ethos ruling = owner_ethos(cx, r);
    if (re->culture.ethos == ruling) return false;   /* même éthos que la couronne : rien à raconter */
    return region_ethos_drift(re->culture.ethos, ruling) > 0.6f;
}
/* C2 — LE DÉCRET DE TOLÉRANCE (pays) : religion_fracture_level au-delà d'un
 * seuil — une part pop-pondérée notable du pays professe un AUTRE culte que
 * la foi d'État. */
static bool trig_tolerance_credo(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return religion_fracture_level(cx->w, cx->econ, c) > 0.25f;
}
/* C3 — LE LETTRÉ PÉRIMÉ (pays) : religion_scholar_drift == 1 (le lettré actif
 * porte une face que le crédo d'État courant ne reconnaît plus). */
static bool trig_lettre_perime(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return religion_scholar_drift(c) >= 1.f;
}
/* C4 — LA PRATIQUE DÉRIVE (pays) : religion_credo_drift (alias documenté de
 * fracture_level) au-delà d'un seuil PLUS HAUT que C2 (une dérive PROFONDE,
 * pas la simple tolérance de C2 — les deux lisent le même signal à deux
 * hauteurs de crise, comme MARBRIVE/CLOCHES lisent tous deux l'agitation à
 * des seuils différents). */
static bool trig_pratique_derive(const EventCtx *cx, int c){
    if (c<0 || c>=cx->w->n_countries) return false;
    return religion_credo_drift(cx->w, cx->econ, c) > 0.45f;
}

/* ===================================================================== */
/* LA TABLE D'ÉVÈNEMENTS (effets = coordonnées ; textes = mots)           */
/* ===================================================================== */
static const EventDef EVENTS[EVID_COUNT] = {
    [EVID_QUAKE] = { EVID_QUAKE, EV_PROVINCE, "La nuit où la terre cria", trig_quake, 8000.f, NULL,
        {{ "Subir le séisme", "Les chiens hurlèrent avant les cloches. Puis les murs ondulèrent comme des draps et la ville disparut sous une poussière blanche.",
           { .d_K_inst=-1.5f, .d_agitation=12.f, .pop_mult=0.90f, .d_treasury=-20.f, .unlock_branch=-1 }, 1.f,
           .flavor="Au matin, les survivants comptèrent les absents avant les pierres. Le royaume, lui, dut compter les deux." }}, 1 },
    [EVID_FLOOD] = { EVID_FLOOD, EV_PROVINCE, "Le fleuve réclame ses rives", trig_flood, 5000.f, NULL,
        {{ "Subir la crue", "L'eau franchit les digues avant l'aube, portant toits, bétail et berceaux. Lorsqu'elle se retire, elle laisse autant de limon que de deuil.",
           { .d_food_cap=1.5f, .d_agitation=6.f, .pop_mult=0.97f, .d_treasury=-10.f, .unlock_branch=-1 }, 1.f,
           .flavor="Nous rebâtirons sur cette terre enrichie, en espérant que le fleuve oublie ce qu'il vient de reprendre." }}, 1 },
    [EVID_DROUGHT] = { EVID_DROUGHT, EV_PROVINCE, "Le ciel demeure de cuivre", trig_drought, 6000.f, NULL,
        {{ "Subir la sécheresse", "Les puits résonnent creux et les processions soulèvent plus de poussière que de prières. Dans les greniers, chaque mesure devient une querelle.",
           { .d_food_cap=-1.5f, .d_agitation=15.f, .pop_mult=0.98f, .d_treasury=-10.f, .unlock_branch=-1 }, 1.f,
           .flavor="La couronne ne peut commander aux nuages. Elle devra pourtant répondre de chaque écuelle vide." }}, 1 },
    [EVID_FIRE] = { EVID_FIRE, EV_PROVINCE, "La forêt brûle jusqu'à l'horizon", trig_fire, 7000.f, NULL,
        {{ "Subir l'incendie", "Le vent poussa les flammes d'arbre en arbre avec la hâte d'un messager. Au soir, le ciel était noir et la terre, nue.",
           { .d_food_cap=0.8f, .d_agitation=5.f, .pop_mult=0.97f, .unlock_branch=-1 }, 1.f,
           .flavor="Nous avons perdu une forêt. Déjà, les arpenteurs discutent de ce qu'ils planteront dans ses cendres." }}, 1 },
    [EVID_PLAGUE] = { EVID_PLAGUE, EV_PROVINCE, "Les cloches sonnent sans relâche", NULL, 12000.f, NULL,
        {{ "Subir l'épidémie", "La maladie est arrivée avec une caravane ordinaire. À présent, les portes restent closes et les fossoyeurs n'attendent plus les noms.",
           { .d_agitation=18.f, .pop_mult=0.78f, .d_treasury=-15.f, .unlock_branch=-1 }, 1.f,
           .flavor="Le commerce avait uni nos routes ; le mal les emprunte à son tour. Que chacun enterre les siens et craigne le prochain voyageur." }}, 1 },

    /* ---- Intégration des marches : MÊME état, quatre récits par la fiche ---- */
    [EVID_INTEG_DOMINATEUR] = { EVID_INTEG_DOMINATEUR, EV_PROVINCE, "Les marges osent gronder",
        trig_integ_dom, 900.f, NULL, {
        { "Mater dans le sang", "Une bannière étrangère a été dressée sur la place provinciale. Vos officiers demandent combien d'exemples il faudra faire pour qu'elle tombe.",
          { .d_H_coerc=2.0f, .d_L=-1.0f, .d_coercion=0.5f, .d_agitation=-40.f, .unlock_branch=-1 }, 0.7f,
          .flavor="Que les pavés se souviennent du prix de la défiance, même si les familles s'en souviennent plus longtemps encore." },
        { "Lever une horde punitive", "Une bannière étrangère a été dressée sur la place provinciale. Vos officiers demandent combien d'exemples il faudra faire pour qu'elle tombe.",
          { .d_H_coerc=1.0f, .d_agitation=-22.f, .pop_mult=0.98f, .unlock_branch=-1 }, 0.3f,
          .flavor="Qu'ils voient arriver nos cavaliers avant d'entendre nos conditions. La peur voyagera plus vite que les survivants." } }, 2 },
    [EVID_INTEG_MERCANTILE] = { EVID_INTEG_MERCANTILE, EV_PROVINCE, "Le comptoir réclame son autonomie",
        trig_integ_merc, 900.f, NULL, {
        { "Affranchir le commerce", "Les négociants de la province ont fermé leurs livres et leurs boutiques. Ils ne réclament pas l'indépendance, seulement le droit d'en fixer le prix.",
          { .d_food_cap=0.6f, .d_L=0.5f, .d_coercion=-0.3f, .d_agitation=-30.f, .unlock_branch=-1 }, 0.6f,
          .flavor="Ouvrons les portes du comptoir. Une cité qui s'enrichit sous notre sceau reste une cité nôtre." },
        { "Acheter les meneurs", "Les négociants de la province ont fermé leurs livres et leurs boutiques. Ils ne réclament pas l'indépendance, seulement le droit d'en fixer le prix.",
          { .d_treasury=-50.f, .d_agitation=-25.f, .unlock_branch=-1 }, 0.4f,
          .flavor="Toute conviction a son tarif. Trouvez les hommes qui savent le recevoir discrètement." } }, 2 },
    [EVID_INTEG_BUREAUCRATE] = { EVID_INTEG_BUREAUCRATE, EV_PROVINCE, "Une province mal arrimée",
        trig_integ_bur, 900.f, NULL, {
        { "Réforme d'intégration", "Trois registres donnent trois noms à la même terre. Tant que le droit hésite, la province saura exactement où cacher ses obligations.",
          { .d_L=1.5f, .d_agitation=-20.f, .unlock_branch=-1 }, 0.65f,
          .flavor="Envoyez des scribes, des juges et assez d'encre pour que nul ne puisse prétendre ignorer la loi." },
        { "Garnison", "Trois registres donnent trois noms à la même terre. Tant que le droit hésite, la province saura exactement où cacher ses obligations.",
          { .d_H_coerc=1.5f, .d_agitation=-15.f, .unlock_branch=-1 }, 0.35f,
          .flavor="La loi viendra plus tard. Pour aujourd'hui, une garnison écrira des réponses que tous sauront lire." } }, 2 },
    [EVID_INTEG_ANCIEN] = { EVID_INTEG_ANCIEN, EV_PROVINCE, "La longue mémoire des conquis",
        trig_integ_anc, 900.f, NULL, {
        { "Patience des siècles", "Les anciens récitent encore la liste de leurs rois déposés. Les plus jeunes ne la connaissent qu'à moitié, ce qui inquiète davantage les prêtres que les soldats.",
          { .d_L=0.3f, .d_agitation=-10.f, .unlock_branch=-1 }, 0.6f,
          .flavor="Ne disputons pas leurs morts. Le temps fera de leurs petits-enfants nos sujets sans leur demander de trahir leurs grands-pères." },
        { "Concession rituelle", "Les anciens récitent encore la liste de leurs rois déposés. Les plus jeunes ne la connaissent qu'à moitié, ce qui inquiète davantage les prêtres que les soldats.",
          { .d_L=0.8f, .d_influence=-5.f, .d_agitation=-12.f, .unlock_branch=-1 }, 0.4f,
          .flavor="Rendons-leur un rite, une procession et quelques larmes officielles. Il est moins coûteux d'honorer une mémoire que de la combattre." } }, 2 },

    /* ---- Génériques (légitimité / credo) ---- */
    [EVID_SUCCESSION] = { EVID_SUCCESSION, EV_COUNTRY, "La couronne n'a plus de tête",
        trig_succession, 6000.f, NULL, {
        { "Transition ordonnée", "Le souverain est mort avant d'avoir réduit ses dernières volontés à une seule version. Dans l'antichambre, les grands parlent déjà plus bas.",
          { .d_L=0.6f, .d_influence=3.f, .unlock_branch=-1 }, 0.5f,
          .flavor="Fermons les portes jusqu'à ce qu'un seul nom en sorte. Le royaume n'a pas besoin de connaître le prix de cet accord." },
        { "Régence contestée", "Le souverain est mort avant d'avoir réduit ses dernières volontés à une seule version. Dans l'antichambre, les grands parlent déjà plus bas.",
          { .d_L=-1.0f, .d_agitation=15.f, .unlock_branch=-1 }, 0.5f,
          .flavor="Qu'un régent tienne le sceau. Chacun prétendra servir l'héritier tandis qu'il mesurera le trône." } }, 2 },
    [EVID_SCHISM] = { EVID_SCHISM, EV_COUNTRY, "Deux vérités sous un même toit",
        trig_schism, 7000.f, NULL, {
        { "Tolérer les deux courants", "Les docteurs se sont séparés sur un mot que le peuple ne comprend pas. Des quartiers entiers savent pourtant déjà pour quelle lecture ils mourraient.",
          { .d_agitation=-10.f, .d_L=-0.3f, .unlock_branch=-1 }, 0.5f,
          .flavor="Que les deux chaires parlent. La vérité survivra peut-être à la dispute, et le royaume avec elle." },
        { "Imposer l'orthodoxie", "Les docteurs se sont séparés sur un mot que le peuple ne comprend pas. Des quartiers entiers savent pourtant déjà pour quelle lecture ils mourraient.",
          { .d_H_coerc=1.0f, .d_coercion=0.3f, .d_agitation=10.f, .unlock_branch=-1 }, 0.5f,
          .flavor="Une foi, une formule, une autorité. Les dissidents apprendront que la grammaire peut avoir un bourreau." } }, 2 },
    /* ---- Floraison cosmopolite : le creuset qui réussit (positif, par l'éthos) ---- */
    [EVID_XENOPHILE] = { EVID_XENOPHILE, EV_PROVINCE, "Une cour aux cent accents",
        trig_xenophile, 2400.f, NULL, {
        { "Célébrer la concorde", "Dans les rues de la capitale, les prières, recettes et berceuses se répondent "
          "sans se confondre. Les étrangers commencent à appeler cette ville la leur.",
          { .d_L=1.2f, .d_food_cap=0.5f, .d_treasury=120.f, .d_influence=4.f, .pop_mult=1.02f, .unlock_branch=-1 }, 1.f,
          /* V2b LOT 3 (hook Communautaire) : le peuple qu'on épargne — la concorde
           * COSMOPOLITE célébrée est, par nature, un vote pour le bien-commun. */
          { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.08f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          .flavor="Que chacun apporte sa langue et son talent. Notre unité sera assez vaste pour ne pas exiger l'oubli." } }, 1 },
    /* ---- Miroir xénophobe : la cohésion du creuset DIGÉRÉ (positif pour l'éthos martial) ---- */
    [EVID_XENOPHOBE] = { EVID_XENOPHOBE, EV_PROVINCE, "Un seul visage dans le miroir",
        trig_xenophobe, 3000.f, NULL, {
        { "Sceller l'unité", "Les anciens costumes ne paraissent plus qu'aux fêtes, et les enfants ignorent "
          "les mots de leurs aïeux. La conquête est devenue habitude.",
          { .d_L=1.0f, .d_H_coerc=0.5f, .d_agitation=-15.f, .d_influence=3.f, .unlock_branch=-1 }, 1.f,
          .flavor="Scellons ce que les générations ont accompli. Une loi, une mémoire et aucun refuge pour les anciennes différences." } }, 1 },

    /* ═══════════════════════════════════════════════════════════════════
     * MEMBRANE DE DÉCISION — MARBRIVE, la crise phare (3 choix imparfaits)
     * ═══════════════════════════════════════════════════════════════════
     * ANCRES DE CALIBRAGE (comparées aux 13 événements existants + aux constantes
     * moteur — tous les deltas sont des NUDGES, pas des tsunamis) :
     *   - d_agitation : la table va de -40 (« Mater dans le sang », un choc DUR déjà
     *     en pleine intégration de marche) à -8/-10 (petits apaisements) ; +10/+15
     *     pour un choix qui LAISSE POURRIR. Marbrive n'est encore qu'à 55 d'agitation
     *     (pas la révolte) → mes 3 choix visent -12/-8/+4, dans la fourchette basse.
     *   - d_H_coerc/d_L : la table va de 0.3 à 2.0 (H_coerc) et -1.0 à +1.5 (L) ; je
     *     reste À 0.2-0.6, un cran sous les chocs d'intégration de marche (Marbrive
     *     est une crise DOMESTIQUE, pas une marche qui gronde de loin).
     *   - d_coercion : RELOC_COERCION_BASE=0.25 (scps_econ.c) est le pic de base d'une
     *     relocalisation — mon +0.22 (Prévôts) reste SOUS cette ligne (un sursaut, pas
     *     un état d'urgence) ; les événements existants vont 0.3-0.5 pour un choc dur.
     *   - d_treasury_mois : NOUVEAU (fraction du revenu MENSUEL, SIGNÉ). -0.15/-0.6/-0.25
     *     = 15 %/60 %/25 % d'un mois de taxes — un mois de solde de garnison, six mois
     *     de subvention, un quart de mois d'enquête : DES ORDRES DE GRANDEUR d'un budget
     *     réel, jamais un « bonus » — à comparer aux 20-50 or fixes des chocs existants
     *     (ceux-là restent inchangés, c'est justement le legs qu'on ne retouche pas).
     *   - hooks : FAC_MARCHAND (payer double = un vote marchand, cf. AI_LEVER_BUILD=0.035
     *     à AI_LEVER_WAR=0.05 dans scps_ai.c — mon 0(concède) est une CAPTURE, plus fort
     *     qu'un simple vote, cohérent avec CAPTURE_LEVER=0.06) ; cooldown=540 j (~1.5 an,
     *     le temps que Marbrive ne puisse pas retirer immédiatement sur la même province).
     */
    [EVID_MARBRIVE] = { EVID_MARBRIVE, EV_PROVINCE, "Les hommes de %s refusent le chantier",
        trig_marbrive, 540.f, NULL, {
        { "Envoyer les prévôts",
          "À %s, les ouvriers ont posé leurs outils devant la carrière. Leur porte-parole affirme que la pierre prend plus de vies que le chantier ne paie de familles.",
          { .d_H_coerc=0.6f, .d_L=-0.4f, .d_coercion=0.22f, .d_agitation=-12.f, .d_treasury_mois=-0.15f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.f, .scar_kind=SCAR_SABOTAGE_CHANTIER, .cooldown_days=540 },
          "Un chantier royal ne négocie pas avec les mains qu'il emploie. Les prévôts leur rendront le sens de la mesure." },
        { "Payer double jusqu'aux pluies",
          "À %s, les ouvriers ont posé leurs outils devant la carrière. Leur porte-parole affirme que la pierre prend plus de vies que le chantier ne paie de familles.",
          { .d_L=0.3f, .d_coercion=-0.10f, .d_agitation=-8.f, .d_treasury_mois=-0.6f, .unlock_branch=-1 },
          0.7f, { .faction=FAC_MARCHAND, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Doublez la paie jusqu'aux pluies. Même la colère travaille lorsqu'elle peut nourrir ses enfants." },
        { "Reporter et ouvrir une enquête",
          "À %s, les ouvriers ont posé leurs outils devant la carrière. Leur porte-parole affirme que la pierre prend plus de vies que le chantier ne paie de familles.",
          { .d_K_inst=0.2f, .d_L=0.1f, .d_agitation=4.f, .d_treasury_mois=-0.25f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Suspendez les travaux et comptez les morts. Si quelqu'un a menti, qu'il redoute davantage l'enquête que le retard." } }, 3 },

    /* PONT EFFONDRÉ — la suite CONSÉQUENTE : le sabotage qu'on n'a pas vu venir (« Envoyer
     * les prévôts » posait la cicatrice SABOTAGE_CHANTIER) mûrit en catastrophe concrète.
     * ANCRES : les 3 choix restent dans la MÊME fourchette que Marbrive (une crise locale,
     * pas une guerre) — d_treasury_mois -0.2/-0.7/0 (traquer coûte peu, reconstruire coûte
     * cher, abandonner ne coûte rien MAIS perd l'institution : d_K_inst négatif au lieu
     * d'un coût d'or, un delta de coordonnée plutôt qu'un montant, cohérent avec la charte
     * « on lit des coordonnées, jamais un bonus plat »). */
    [EVID_PONT_EFFONDRE] = { EVID_PONT_EFFONDRE, EV_PROVINCE, "Le pont de %s s'est effondré une nuit",
        trig_pont_effondre, 1500.f, NULL, {
        { "Traquer les saboteurs",
          "Le pont de %s s'est rompu sans crue ni tempête. Parmi les débris, on a trouvé des chevilles sciées et le sceau d'un contremaître disparu.",
          { .d_H_coerc=0.4f, .d_coercion=0.15f, .d_agitation=-6.f, .d_treasury_mois=-0.2f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Fermez les routes et ouvrez les geôles. Que les coupables apprennent qu'un pont détruit peut encore mener à l'échafaud." },
        { "Reconstruire, en payant cette fois",
          "Le pont de %s s'est rompu sans crue ni tempête. Parmi les débris, on a trouvé des chevilles sciées et le sceau d'un contremaître disparu.",
          { .d_K_inst=0.5f, .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-0.7f, .unlock_branch=-1 },
          0.7f, { .faction=FAC_LEGISTE, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Rebâtissez plus large, plus solide et sous des yeux mieux payés. Cette fois, l'économie serait une faute trop coûteuse." },
        { "Abandonner la route",
          "Le pont de %s s'est rompu sans crue ni tempête. Parmi les débris, on a trouvé des chevilles sciées et le sceau d'un contremaître disparu.",
          { .d_K_inst=-0.4f, .d_agitation=6.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "La rivière gardera son passage. Que les marchands trouvent une autre route et les saboteurs, une victoire sans lendemain." } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * CONTENU W1 — six évènements neufs. ANCRES DE CALIBRAGE (mêmes fourchettes
     * que Marbrive/Pont Effondré ci-dessus, comparées à la même table existante) :
     *   - d_agitation/d_L/d_H_coerc/d_coercion : je reste dans la fourchette basse
     *     documentée pour Marbrive (±0.1-0.6 sur H_coerc/L, ±4-15 sur agitation,
     *     ≤0.22 sur coercion) — ce sont des crises DOMESTIQUES, pas des chocs.
     *   - d_treasury_mois (SIGNÉ, fraction du revenu MENSUEL) : -0.15..-1.0 pour
     *     un coût (d'un sursaut d'enquête à une charte durable), +0.3..+2.0 pour
     *     un gain (réquisition, vente d'eau, vente d'un secret) — même ordre de
     *     grandeur que Marbrive/Pont (-0.15..-0.7) ; la vente du secret runique
     *     (+2.0, six mois de taxes) est le gain le plus lourd de la table, à la
     *     mesure d'un secret d'État vendu — mais laisse une PROLIFÉRATION.
     *   - gamble_p 0.25-0.5 : un pari a plus de chances de tourner que pas (0.5)
     *     pour les paris « discrets » (la gratitude, le convoi, la vente), moins
     *     (0.25-0.3) pour les paris qui demandent un concours de circonstances
     *     (le double péage remarqué, une découverte de savants, un secret qui
     *     s'ébruite deux fois).
     *   - cooldown_days=720 (2 ans, cf. le titre EV_PROVINCE) sauf SALVE_RUNIQUE
     *     (36500 j = jamais deux fois, le déclenchement UNIQUE du mécanisme 2) et
     *     DERNIÈRE_DÉCISION (360 j, une crise plus resserrée par nature).
     */

    /* CLOCHES — la surtaxe fait taire les cloches. */
    [EVID_CLOCHES] = { EVID_CLOCHES, EV_PROVINCE, "Les cloches de %s ne sonnent plus pour l'impôt",
        trig_cloches, 720.f, NULL, {
        { "Lever quand même",
          "À %s, le sonneur refuse d'appeler les habitants à l'impôt. Les notables parlent de tradition ; le receveur, de sédition.",
          { .d_L=-0.4f, .d_H_coerc=0.4f, .d_coercion=0.18f, .d_agitation=10.f, .d_treasury_mois=0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_DETTE_OBEISSANCE, .cooldown_days=720 },
          "Que le collecteur frappe aux portes une par une. Le silence des cloches ne rend pas la dette muette." },
        { "Accorder une remise d'une année",
          "À %s, le sonneur refuse d'appeler les habitants à l'impôt. Les notables parlent de tradition ; le receveur, de sédition.",
          { .d_L=0.5f, .d_coercion=-0.15f, .d_agitation=-10.f, .d_treasury_mois=-1.0f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : épargner le peuple d'une surtaxe — le vote
           * évident du bien-commun, cohérent avec l'ai_chance déjà la plus haute (0.7). */
          0.7f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.06f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Accordons-leur une année. Une grâce proclamée par le trône vaut mieux qu'un refus arraché par la rue.",
          { .pop_mult=1.004f, .unlock_branch=-1 }, 0.3f },
        { "Acheter les notables",
          "À %s, le sonneur refuse d'appeler les habitants à l'impôt. Les notables parlent de tradition ; le receveur, de sédition.",
          { .d_L=0.2f, .d_agitation=-8.f, .d_treasury_mois=-0.6f, .d_influence=-2.f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Les notables aiment les principes jusqu'à ce qu'une bourse les oblige à les compter." } }, 3 },

    /* ENTREPÔTS FERMÉS — le grief marchand ferme boutique. */
    [EVID_ENTREPOTS_FERMES] = { EVID_ENTREPOTS_FERMES, EV_PROVINCE, "Les entrepôts de %s ferment sans édit",
        trig_entrepots_fermes, 720.f, NULL, {
        { "Accorder une charte de transit",
          "Les négociants de %s ont cadenassé leurs entrepôts le même matin. Aucun édit ne l'ordonne, mais chacun jure avoir agi seul.",
          { .d_L=0.4f, .d_coercion=-0.12f, .d_agitation=-8.f, .d_treasury_mois=-0.45f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.6f, { .faction=FAC_MARCHAND, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Donnons au transit une charte que nul caprice de marchand ne pourra refermer." },
        { "Réquisitionner les entrepôts",
          "Les négociants de %s ont cadenassé leurs entrepôts le même matin. Aucun édit ne l'ordonne, mais chacun jure avoir agi seul.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_coercion=0.2f, .d_agitation=6.f, .d_treasury_mois=0.3f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RANCUNE_MARCHANDE, .cooldown_days=720 },
          "Brisez les cadenas au nom de la nécessité publique. Ils réclameront leurs biens lorsqu'ils auront retrouvé leur loyauté." },
        { "Escorter les convois concurrents",
          "Les négociants de %s ont cadenassé leurs entrepôts le même matin. Aucun édit ne l'ordonne, mais chacun jure avoir agi seul.",
          { .d_H_coerc=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.75f, .d_influence=2.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Que nos soldats escortent leurs concurrents. Rien ne rouvre plus vite une boutique que le succès du voisin.",
          { .d_C_global=0.08f, .unlock_branch=-1 }, 0.35f } }, 3 },

    /* DEUX CARTES — une conquête récente, deux frontières. */
    [EVID_DEUX_CARTES] = { EVID_DEUX_CARTES, EV_PROVINCE, "Deux cartes, deux frontières à %s",
        trig_deux_cartes, 720.f, NULL, {
        { "Imposer la carte de la capitale",
          "La carte royale place trois villages à l'est de la frontière ; celle de %s les place à l'ouest. Les deux administrations y prélèvent déjà leur dû.",
          { .d_K_inst=0.3f, .d_L=-0.4f, .d_coercion=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Déployez la carte de la capitale et les hommes chargés de la rendre vraie." },
        { "Arbitrer sur place",
          "La carte royale place trois villages à l'est de la frontière ; celle de %s les place à l'ouest. Les deux administrations y prélèvent déjà leur dû.",
          { .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.5f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Qu'un arbitre entende les bornes, les actes et les vieillards. Une frontière acceptée tient mieux qu'une ligne imposée." },
        { "Laisser l'ambiguïté rapporter",
          "La carte royale place trois villages à l'est de la frontière ; celle de %s les place à l'ouest. Les deux administrations y prélèvent déjà leur dû.",
          { .d_L=-0.3f, .d_agitation=8.f, .d_treasury_mois=0.35f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EXEMPTION_ACHETEE, .cooldown_days=720 },
          "Tant que les deux péages remontent au trésor, l'incertitude possède des vertus remarquablement précises.",
          { .d_agitation=8.f, .unlock_branch=-1 }, 0.25f } }, 3 },

    /* EAU NOIRE — le puits vire noir, la Brèche s'invite. */
    [EVID_EAU_NOIRE] = { EVID_EAU_NOIRE, EV_PROVINCE, "Le puits de %s donne une eau noire",
        trig_eau_noire, 1095.f, NULL, {
        { "Fermer le puits",
          "Le puits de %s donne depuis hier une eau noire qui ne tache pas les mains. Les malades y voient un remède, les savants une invitation.",
          { .d_food_cap=-0.3f, .d_L=0.4f, .d_agitation=-6.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Murez le puits avant que la peur ne lui invente des miracles." },
        { "Laisser les savants prélever",
          "Le puits de %s donne depuis hier une eau noire qui ne tache pas les mains. Les malades y voient un remède, les savants une invitation.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.08f, .d_treasury_mois=-0.45f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Prélevez une fiole, puis une autre. La prudence ferme des portes que la connaissance ne retrouvera peut-être jamais.",
          { .d_breach=-0.04f, .d_influence=1.f, .unlock_branch=-1 }, 0.3f },
        { "Vendre l'eau comme remède",
          "Le puits de %s donne depuis hier une eau noire qui ne tache pas les mains. Les malades y voient un remède, les savants une invitation.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_treasury_mois=0.5f, .d_breach=0.05f, .pop_mult=0.997f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1095 },
          "Bouteillez-la sous un beau nom. Les gens paient volontiers pour croire qu'un mystère les a choisis." } }, 3 },

    /* DERNIÈRE DÉCISION — le passé n'a pas fini d'arriver. */
    [EVID_DERNIERE_DECISION] = { EVID_DERNIERE_DECISION, EV_PROVINCE, "À %s, la dernière décision n'a pas fini d'arriver",
        trig_derniere_decision, 360.f, NULL, {
        { "Corriger publiquement",
          "Un ancien décret vient seulement d'atteindre %s. Il contredit les mesures prises depuis et chacun possède désormais un ordre légal pour désobéir à l'autre.",
          { .d_L=0.4f, .d_coercion=-0.15f, .d_agitation=-8.f, .d_treasury_mois=-0.75f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "Admettons l'erreur devant tous. Une autorité qui se corrige perd une journée ; celle qui s'entête peut perdre une province." },
        { "Laisser les vagues mourir",
          "Un ancien décret vient seulement d'atteindre %s. Il contredit les mesures prises depuis et chacun possède désormais un ordre légal pour désobéir à l'autre.",
          { .d_L=-0.15f, .d_agitation=5.f, .pop_mult=0.999f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "N'ajoutons pas un nouvel ordre au désordre. Avec assez de temps, même une décision royale finit par devenir une rumeur.",
          { .d_agitation=-6.f, .unlock_branch=-1 }, 0.4f },
        { "Achever la décision par la force",
          "Un ancien décret vient seulement d'atteindre %s. Il contredit les mesures prises depuis et chacun possède désormais un ordre légal pour désobéir à l'autre.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.2f, .d_agitation=-6.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "Envoyez des hommes capables de terminer la phrase que nos scribes ont commencée." } }, 3 },

    /* SALVE RUNIQUE — la première fois que le feu runique parle (pays, unique). */
    [EVID_SALVE_RUNIQUE] = { EVID_SALVE_RUNIQUE, EV_COUNTRY, "La première salve runique",
        trig_salve_runique, 3650.f, NULL, {
        { "Doctriner l'usage",
          "La première salve runique a traversé les boucliers, le mur derrière eux et l'assurance de l'état-major. Les survivants réclament déjà davantage d'armes.",
          { .d_K_inst=0.4f, .d_L=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.6f, .d_breach=-0.02f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_LEGISTE, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Avant d'armer l'empire, apprenons-lui où viser. Une merveille sans doctrine n'est qu'un accident reproductible." },
        { "Armer toutes les légions",
          "La première salve runique a traversé les boucliers, le mur derrière eux et l'assurance de l'état-major. Les survivants réclament déjà davantage d'armes.",
          { .d_H_coerc=0.6f, .d_L=-0.4f, .d_agitation=8.f, .d_breach=0.10f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_CONQUERANT, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Équipez toutes les légions. La prudence est un luxe que nos ennemis ne nous laisseront pas." },
        { "Vendre le secret",
          "La première salve runique a traversé les boucliers, le mur derrière eux et l'assurance de l'état-major. Les survivants réclament déjà davantage d'armes.",
          { .d_agitation=6.f, .d_treasury_mois=2.0f, .d_influence=-3.f, .d_breach=0.15f, .unlock_branch=-1 },
          0.15f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_PROLIFERATION_ARME, .cooldown_days=36500 },
          "Un secret vaut surtout avant que quelqu'un d'autre ne le découvre. Trouvez un acheteur et exigez le prix de son avenir.",
          { .d_influence=-2.f, .unlock_branch=-1 }, 0.5f } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * CONTENU W2 (lot 2) — §A tech (déclenchement unique) · §B culturel ·
     * §C religieux · §D chaînage. ANCRES DE CALIBRAGE (mêmes fourchettes que
     * W1/Marbrive ci-dessus, comparées à la MÊME table existante) :
     *   - d_agitation/d_L/d_H_coerc/d_coercion : fourchette basse (±0.1-0.6 sur
     *     H_coerc/L, ±4-16 sur agitation, ≤0.22 sur coercion) — crises DOMESTIQUES.
     *   - d_treasury_mois (SIGNÉ, fraction du revenu MENSUEL) : -0.15..-1.6 pour
     *     un coût, +0.3..+0.8 pour un gain — même ordre de grandeur que W1/Marbrive.
     *   - gamble_p 0.15-0.5 selon la vraisemblance du pari (discret → probable ;
     *     concours de circonstances → rare).
     *   - cooldown_days=1460-1825 (4-5 ans, EV_COUNTRY structurels §A/§B/§C) ou
     *     360-900 (les chaînages §D, des crises plus resserrées) ; SALVE_RUNIQUE
     *     avait 36500 (jamais deux fois) — les §A tech sont RE-TIRABLES (une
     *     institution ne se referme pas nécessairement pour toujours), donc un
     *     cooldown ORDINAIRE (1825 j) plutôt que le énorme de SALVE_RUNIQUE : la
     *     tech reste débloquée en permanence, mais l'ÉVÈNEMENT (la première
     *     décision qu'elle impose) ne doit pas re-tirer chaque année tant que le
     *     répit tient — c'est la MTTH (720-3650 j) qui fait le gros du travail
     *     d'espacement, le cooldown n'est qu'un filet après un CHOIX déjà pris.
     */

    /* A1 — LES CHAÎNES RAPPORTENT (pays, Économie servile). */
    [EVID_CHAINES_RAPPORTENT] = { EVID_CHAINES_RAPPORTENT, EV_COUNTRY, "Les chaînes rapportent",
        trig_chaines_rapportent, 1825.f, NULL, {
        { "Institutionnaliser",
          "Les registres montrent une hausse parfaite de la production. Dans les marges, un scribe a noté combien de chaînes il faut pour l'obtenir.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.18f, .d_treasury_mois=0.6f, .pop_mult=1.004f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_REVOLTE_SERVILE, .cooldown_days=1825 },
          "Cessons l'hypocrisie : si le royaume vit de leur servitude, qu'il l'organise avec toute la froideur de ses autres revenus." },
        { "Limiter aux vaincus",
          "Les registres montrent une hausse parfaite de la production. Dans les marges, un scribe a noté combien de chaînes il faut pour l'obtenir.",
          { .d_L=-0.15f, .d_coercion=0.10f, .d_treasury_mois=0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Les vaincus porteront seuls ce fardeau. Que la frontière entre châtiment et avidité demeure inscrite dans la loi." },
        { "Abolir",
          "Les registres montrent une hausse parfaite de la production. Dans les marges, un scribe a noté combien de chaînes il faut pour l'obtenir.",
          { .d_L=0.5f, .d_agitation=-8.f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_CONQUERANT, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Brisez les chaînes avant qu'elles ne deviennent les fondations du trésor. Certaines richesses rendent un royaume plus pauvre.",
          { .d_influence=2.f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* A2 — L'ŒUVRE NOIRE NE S'ÉTEINT PAS LA NUIT (pays, L'Œuvre noire). */
    [EVID_OEUVRE_NOIRE] = { EVID_OEUVRE_NOIRE, EV_COUNTRY, "L'Œuvre noire ne s'éteint pas la nuit",
        trig_oeuvre_noire, 1825.f, NULL, {
        { "Sceller",
          "Même froide, l'Œuvre noire murmure dans son caveau. Les gardes changent plus souvent, car aucun ne supporte longtemps ses propres rêves.",
          { .d_L=0.5f, .d_agitation=-6.f, .d_treasury_mois=-0.6f, .d_breach=-0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Trois serrures, trois gardiens et aucun nom dans le registre. Ce qui ne peut être détruit doit au moins être oublié." },
        { "Déployer aux frontières",
          "Même froide, l'Œuvre noire murmure dans son caveau. Les gardes changent plus souvent, car aucun ne supporte longtemps ses propres rêves.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.10f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Portez-la aux frontières. Si elle doit inspirer la peur, qu'elle la fasse naître chez ceux qui nous menacent.",
          { .d_influence=2.f, .unlock_branch=-1 }, 0.4f },
        { "Disséminer",
          "Même froide, l'Œuvre noire murmure dans son caveau. Les gardes changent plus souvent, car aucun ne supporte longtemps ses propres rêves.",
          { .d_agitation=8.f, .d_L=-0.3f, .d_breach=0.16f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_PROLIFERATION_ARME, .cooldown_days=1825 },
          "Divisez son secret entre les légions. Une terreur partagée cesse d'être un tabou et devient une doctrine." } }, 3 },

    /* A3 — LE SAVOIR INTERDIT TIENT SES PROMESSES (pays, Savoir interdit). */
    [EVID_SAVOIR_INTERDIT] = { EVID_SAVOIR_INTERDIT, EV_COUNTRY, "Le Savoir interdit tient ses promesses",
        trig_savoir_interdit, 1825.f, NULL, {
        { "École close",
          "Les calculs du texte scellé se vérifient. Chaque page offre un pouvoir réel et une raison supplémentaire de ne pas tourner la suivante.",
          { .d_K_inst=0.4f, .d_L=0.2f, .d_treasury_mois=-0.75f, .d_breach=0.04f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Choisissez peu d'élèves, des murs épais et des maîtres qui comprennent que leur diplôme sera une surveillance à vie." },
        { "Exploiter sans le dire",
          "Les calculs du texte scellé se vérifient. Chaque page offre un pouvoir réel et une raison supplémentaire de ne pas tourner la suivante.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.12f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.20f, .scar_kind=SCAR_FUITE_CERVEAUX, .cooldown_days=1825 },
          "Employons ce savoir sans le nommer. Les résultats parleront ; nous nierons seulement avoir entendu leur voix.",
          { .d_agitation=6.f, .unlock_branch=-1 }, 0.35f },
        { "Bannir et brûler",
          "Les calculs du texte scellé se vérifient. Chaque page offre un pouvoir réel et une raison supplémentaire de ne pas tourner la suivante.",
          { .d_L=-0.2f, .d_H_coerc=0.3f, .d_coercion=0.15f, .d_agitation=6.f, .d_breach=-0.06f, .unlock_branch=-1 },
          0.15f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_FUITE_CERVEAUX, .cooldown_days=1825 },
          "Brûlez les copies, puis les notes de ceux qui les ont lues. Il existe des victoires que l'on remporte en restant ignorant." } }, 3 },

    /* A4 — LE TRÔNE EST DEVENU UN AUTEL (pays, Culte impérial). */
    [EVID_CULTE_IMPERIAL] = { EVID_CULTE_IMPERIAL, EV_COUNTRY, "Le trône est devenu un autel",
        trig_culte_imperial, 1825.f, NULL, {
        { "Imposer le culte",
          "Dans la capitale, des fidèles s'agenouillent désormais devant le portrait impérial avant même d'entrer au temple. Le clergé attend votre réponse.",
          { .d_K_inst=0.3f, .d_H_coerc=0.4f, .d_L=0.4f, .d_coercion=0.15f, .d_agitation=-10.f, .unlock_branch=-1 },
          0.6f, { .faction=FAC_GARDIEN, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Si le peuple cherche le divin dans la couronne, ne le détrompons pas. Qu'un même geste serve la foi et l'obéissance." },
        { "Cohabiter",
          "Dans la capitale, des fidèles s'agenouillent désormais devant le portrait impérial avant même d'entrer au temple. Le clergé attend votre réponse.",
          { .d_L=0.2f, .d_agitation=-6.f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le souverain régnera sur les vivants ; les prêtres parleront de l'éternité. Chacun possède assez de territoire pour éviter la guerre." },
        { "Y renoncer",
          "Dans la capitale, des fidèles s'agenouillent désormais devant le portrait impérial avant même d'entrer au temple. Le clergé attend votre réponse.",
          { .d_L=-0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Retirez nos portraits des autels. Un trône qui exige des prières avoue qu'il ne suffit plus à inspirer le respect.",
          { .d_L=0.3f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* A5 — QUELQUE CHOSE S'EST ÉVEILLÉ DANS LES GLYPHES (pays, L'Éveil). */
    [EVID_EVEIL] = { EVID_EVEIL, EV_COUNTRY, "Quelque chose s'est éveillé dans les glyphes",
        trig_eveil, 1825.f, NULL, {
        { "L'atteler à la guerre",
          "Les glyphes ont ouvert des yeux qui n'étaient dessinés nulle part. Autour du cercle, des silhouettes attendent un ordre dans un silence parfaitement militaire.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_agitation=8.f, .d_breach=0.12f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une armée sans faim, sans sommeil et sans pitié : envoyons-la là où nos soldats hésiteraient." },
        { "Vase clos",
          "Les glyphes ont ouvert des yeux qui n'étaient dessinés nulle part. Autour du cercle, des silhouettes attendent un ordre dans un silence parfaitement militaire.",
          { .d_K_inst=0.4f, .d_treasury_mois=-0.75f, .d_breach=0.05f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Fermez les portes du laboratoire. Nous l'étudierons assez longtemps pour savoir si c'est nous qui observons.",
          { .d_influence=2.f, .d_breach=-0.02f, .unlock_branch=-1 }, 0.3f },
        { "Refermer",
          "Les glyphes ont ouvert des yeux qui n'étaient dessinés nulle part. Autour du cercle, des silhouettes attendent un ordre dans un silence parfaitement militaire.",
          { .d_L=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.5f, .d_breach=-0.08f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EVEIL_SOMMEIL, .cooldown_days=1825 },
          "Effacez le cercle, pierre après pierre. Nous renoncerons à ce pouvoir avant qu'il ne découvre nos besoins." } }, 3 },

    /* A6 — LA FOREUSE MORD DANS QUELQUE CHOSE QUI SAIGNE (province, Foreuse arcanique bâtie ICI). */
    [EVID_FOREUSE_SAIGNE] = { EVID_FOREUSE_SAIGNE, EV_PROVINCE, "À %s, la foreuse mord dans quelque chose qui saigne",
        trig_foreuse_saigne, 1825.f, NULL, {
        { "Plein régime",
          "Sous %s, la foreuse est remontée couverte d'un sang tiède. Les ouvriers refusent de redescendre ; l'ingénieur assure que le rendement n'a jamais été meilleur.",
          { .d_agitation=10.f, .d_breach=0.12f, .d_treasury_mois=0.8f, .pop_mult=0.998f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1825 },
          "Relancez à pleine puissance. Si la profondeur peut saigner, elle peut aussi céder." },
        { "Sous relevés",
          "Sous %s, la foreuse est remontée couverte d'un sang tiède. Les ouvriers refusent de redescendre ; l'ingénieur assure que le rendement n'a jamais été meilleur.",
          { .d_K_inst=0.3f, .d_treasury_mois=0.4f, .d_breach=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Consignez la couleur, la chaleur et le rythme. Même l'effroi devient utile lorsqu'on lui donne des colonnes.",
          { .d_treasury_mois=0.4f, .unlock_branch=-1 }, 0.35f },
        { "Reboucher",
          "Sous %s, la foreuse est remontée couverte d'un sang tiède. Les ouvriers refusent de redescendre ; l'ingénieur assure que le rendement n'a jamais été meilleur.",
          { .d_L=0.4f, .d_treasury_mois=-0.6f, .d_breach=-0.04f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Bouchez le puits et payez ceux qui se tairont. Certaines profondeurs sont des frontières, non des ressources." } }, 3 },

    /* B1 — LE DROIT D'INTÉGRATION DIVISE CEUX QU'IL UNIT (pays, off-culture > 25 %). */
    [EVID_DROIT_INTEGRATION] = { EVID_DROIT_INTEGRATION, EV_COUNTRY, "Le droit d'intégration divise ceux qu'il unit",
        trig_droit_integration, 1460.f, NULL, {
        { "Forcer l'assimilation",
          "La nouvelle loi promet l'égalité, mais les tribunaux de quartier ne s'accordent ni sur sa langue ni sur ceux qu'elle oblige à changer.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.18f, .d_agitation=8.f, .d_C_global=-0.02f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_FRACTURE_CULTURELLE, .cooldown_days=1460 },
          "Une langue, une procédure, une couronne. L'unité naîtra de la répétition, même si elle commence par la contrainte." },
        { "Langue franque",
          "La nouvelle loi promet l'égalité, mais les tribunaux de quartier ne s'accordent ni sur sa langue ni sur ceux qu'elle oblige à changer.",
          { .d_K_inst=0.4f, .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.75f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Donnons-leur des mots communs sans confisquer les anciens. Un pont n'exige pas que les deux rives se ressemblent." },
        { "Laisser les communautés",
          "La nouvelle loi promet l'égalité, mais les tribunaux de quartier ne s'accordent ni sur sa langue ni sur ceux qu'elle oblige à changer.",
          { .d_L=0.2f, .d_agitation=-6.f, .d_C_global=0.03f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : le pluralisme qui laisse chacun chez soi —
           * le vote le plus évident du bien-commun de ce dilemme. */
          0.3f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Que chaque communauté conserve ses usages. Nous gouvernerons la mosaïque sans prétendre la fondre.",
          { .pop_mult=1.004f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* B4 — LA DIASPORA TIENT LES COMPTOIRS (pays, diaspora > 15 %, comptoirs actifs). */
    [EVID_DIASPORA_COMPTOIRS] = { EVID_DIASPORA_COMPTOIRS, EV_COUNTRY, "La diaspora tient les comptoirs",
        trig_diaspora_comptoirs, 1460.f, NULL, {
        { "Charte de protection",
          "Les comptoirs vivent grâce à des familles venues d'ailleurs. Elles connaissent chaque route et chaque prix, mais les habitants leur reprochent de ne jamais oublier d'où elles viennent.",
          { .d_L=0.2f, .d_agitation=6.f, .d_treasury_mois=0.3f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_MARCHAND, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Placez-les sous notre protection. Quiconque enrichit le royaume doit pouvoir dormir sans craindre sa propre rue." },
        { "Taxe spéciale",
          "Les comptoirs vivent grâce à des familles venues d'ailleurs. Elles connaissent chaque route et chaque prix, mais les habitants leur reprochent de ne jamais oublier d'où elles viennent.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_treasury_mois=0.5f, .pop_mult=0.998f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RANCUNE_MARCHANDE, .cooldown_days=1460 },
          "Leur réussite prouve qu'ils peuvent contribuer davantage. Donnons à cette jalousie la forme respectable d'un impôt." },
        { "Expulser",
          "Les comptoirs vivent grâce à des familles venues d'ailleurs. Elles connaissent chaque route et chaque prix, mais les habitants leur reprochent de ne jamais oublier d'où elles viennent.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_C_global=-0.10f, .pop_mult=0.985f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Expulsez-les et saisissez les boutiques. Nous découvrirons bientôt si les murs savaient commercer sans leurs propriétaires.",
          { .d_treasury_mois=-1.0f, .unlock_branch=-1 }, 0.5f } }, 3 },

    /* C1 — LA FOI VA SE FENDRE (pays, schisme éligible : RUPTURE ou DÉRIVE). */
    [EVID_FOI_FENDRE] = { EVID_FOI_FENDRE, EV_COUNTRY, "La foi va se fendre",
        trig_foi_fendre, 1825.f, NULL, {
        { "Forcer l'unité",
          "Les fidèles récitent encore le même credo, mais ils ne donnent plus le même sens aux mots. Les prêtres demandent une décision avant que les foules ne la prennent.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.15f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une foi ne survivra pas à mille interprétations. Imposons celle qui protège l'ordre." },
        { "Laisser dériver",
          "Les fidèles récitent encore le même credo, mais ils ne donnent plus le même sens aux mots. Les prêtres demandent une décision avant que les foules ne la prennent.",
          { .d_L=-0.15f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissons le nouveau courant trouver son lit. On ne retient pas une croyance en dressant des barrages de papier." },
        { "Concile",
          "Les fidèles récitent encore le même credo, mais ils ne donnent plus le même sens aux mots. Les prêtres demandent une décision avant que les foules ne la prennent.",
          { .d_K_inst=0.4f, .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.9f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_GARDIEN, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Convoquez un concile et verrouillez les portes. Que personne ne sorte avant d'avoir trouvé une formule que tous puissent détester également.",
          { .d_L=0.3f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* C5 — LA BRÈCHE A TROUVÉ SON PROPHÈTE (pays, charge faustienne haute, Transgresseurs pas comblés). */
    [EVID_PROPHETE_BRECHE] = { EVID_PROPHETE_BRECHE, EV_COUNTRY, "La brèche a trouvé son prophète",
        trig_prophete_breche, 1460.f, NULL, {
        { "Livrer aux gardiens",
          "Un prédicateur décrit la Brèche avec des détails absents de nos archives. Chaque sermon attire plus de miséreux, et fait vibrer les instruments des savants.",
          { .d_L=0.2f, .d_H_coerc=0.3f, .d_coercion=0.15f, .d_agitation=8.f, .d_breach=-0.06f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=1460 },
          "Livrez-le aux gardiens avant que son bûcher ne devienne lui-même un lieu de pèlerinage." },
        { "L'écouter",
          "Un prédicateur décrit la Brèche avec des détails absents de nos archives. Chaque sermon attire plus de miséreux, et fait vibrer les instruments des savants.",
          { .d_L=-0.3f, .d_agitation=8.f, .d_breach=0.14f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Faites-le parler devant ceux qui étudient la Brèche. Une vérité dangereuse ne devient pas fausse parce qu'un fou la prononce." },
        { "Le récupérer",
          "Un prédicateur décrit la Brèche avec des détails absents de nos archives. Chaque sermon attire plus de miséreux, et fait vibrer les instruments des savants.",
          { .d_K_inst=0.3f, .d_L=0.2f, .d_agitation=-6.f, .d_breach=0.05f, .d_influence=1.f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Donnez-lui une chaire, un texte approuvé et des gardes discrets. Un prophète encadré est plus utile qu'un martyr libre.",
          { .d_breach=0.06f, .unlock_branch=-1 }, 0.4f } }, 3 },

    /* C6 — LA RELIQUE FAIT DES MIRACLES DOUTEUX (province, édifice de foi + amplitude haute). */
    [EVID_RELIQUE_DOUTEUSE] = { EVID_RELIQUE_DOUTEUSE, EV_PROVINCE, "La relique de %s fait des miracles douteux",
        trig_relique_douteuse, 1095.f, NULL, {
        { "Authentifier",
          "À %s, une phalange noircie guérirait les fièvres et ferait pleurer les statues. Les aubergistes y croient déjà avec une ferveur très rentable.",
          { .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=0.4f, .d_breach=0.06f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1095 },
          "Que l'Église l'authentifie. Un miracle reconnu rassemble mieux qu'un doute savant." },
        { "Enquêter",
          "À %s, une phalange noircie guérirait les fièvres et ferait pleurer les statues. Les aubergistes y croient déjà avec une ferveur très rentable.",
          { .d_K_inst=0.3f, .d_L=-0.15f, .d_treasury_mois=-0.4f, .d_breach=-0.03f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Ouvrez le reliquaire devant des témoins compétents. Si le ciel agit ici, il supportera bien quelques questions.",
          { .d_L=0.3f, .d_breach=-0.03f, .unlock_branch=-1 }, 0.35f },
        { "Interdire le pèlerinage",
          "À %s, une phalange noircie guérirait les fièvres et ferait pleurer les statues. Les aubergistes y croient déjà avec une ferveur très rentable.",
          { .d_H_coerc=0.2f, .d_L=-0.3f, .d_coercion=0.12f, .d_agitation=8.f, .d_breach=-0.05f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Fermez la route avant que la foule n'y apporte ses malades, son argent et ses émeutes." } }, 3 },

    /* ═══ §D — CHAÎNAGE DE CICATRICES : les scars posées plus haut (§A/W1) mûrissent ici ═══ */

    /* K2 — LE REMÈDE FAIT DES MORTS (SCAR_SCANDALE_SANITAIRE mûrit — posée par EAU_NOIRE
     * "Vendre l'eau comme remède" ou RELIQUE_DOUTEUSE "Authentifier", délai 360-720 j
     * — scar_delay_range, voir apply_choice_hook plus bas dans le fichier). */
    [EVID_REMEDE_MORTS] = { EVID_REMEDE_MORTS, EV_PROVINCE, "Le remède de %s fait des morts",
        trig_remede_morts, 900.f, NULL, {
        { "Sacrifier les vendeurs",
          "Le remède vendu à %s promettait de chasser la fièvre. Les fiévreux sont morts les premiers ; le marchand a quitté la ville avant les funérailles.",
          { .d_L=0.3f, .d_H_coerc=0.3f, .d_coercion=0.12f, .d_agitation=-8.f, .d_influence=-2.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Trouvez les vendeurs et dressez l'échafaud sur leur étal. La justice doit être vue là où la fraude fut achetée." },
        { "Indemniser",
          "Le remède vendu à %s promettait de chasser la fièvre. Les fiévreux sont morts les premiers ; le marchand a quitté la ville avant les funérailles.",
          { .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-1.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Indemnisez les familles. Nous ne rendrons pas les morts, mais nous pouvons empêcher leur absence de condamner les vivants." },
        { "Nier en bloc",
          "Le remède vendu à %s promettait de chasser la fièvre. Les fiévreux sont morts les premiers ; le marchand a quitté la ville avant les funérailles.",
          { .d_L=-0.3f, .d_agitation=8.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=720 },
          "Il n'existe ni remède autorisé ni victimes reconnues. Que les registres guérissent ce que la médecine a aggravé.",
          { .d_agitation=-6.f, .unlock_branch=-1 }, 0.5f } }, 3 },

    /* K3 — UNE CELLULE DANS LES FAUBOURGS (SCAR_RADICALISATION mûrit — posée par
     * PROPHETE_BRECHE "Livrer aux gardiens" ou REMEDE_MORTS "Nier en bloc"). */
    [EVID_CELLULE_FAUBOURGS] = { EVID_CELLULE_FAUBOURGS, EV_PROVINCE, "Une cellule dans les faubourgs de %s",
        trig_cellule_faubourgs, 900.f, NULL, {
        { "Rafle générale",
          "Une cellule clandestine recrute dans les faubourgs de %s. Les informateurs connaissent trois noms ; les geôles pourraient en fournir trente de plus, vrais ou non.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.20f, .d_agitation=-10.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=720 },
          "Bouclez le quartier et remplissez les chariots. L'innocence sera examinée lorsque le danger aura cessé de courir." },
        { "Amnistie et emplois",
          "Une cellule clandestine recrute dans les faubourgs de %s. Les informateurs connaissent trois noms ; les geôles pourraient en fournir trente de plus, vrais ou non.",
          { .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-1.0f, .pop_mult=1.004f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : épargner des gens qu'on aurait pu rafler —
           * l'amnistie EST le vote du bien-commun de cet évènement. */
          0.5f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.07f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Offrez l'amnistie et du travail à ceux qui sortent avant l'aube. Retirons à la révolte ce que la misère lui prête." },
        { "Infiltrer",
          "Une cellule clandestine recrute dans les faubourgs de %s. Les informateurs connaissent trois noms ; les geôles pourraient en fournir trente de plus, vrais ou non.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Laissez entrer notre homme. Un réseau que l'on observe conduit parfois plus haut que celui que l'on brise.",
          { .d_agitation=-8.f, .unlock_branch=-1 }, 0.4f } }, 3 },

    /* K5 — NOS PROPRES FUSILS NOUS REVIENNENT (SCAR_PROLIFERATION_ARME mûrit — posée par
     * SALVE_RUNIQUE "Vendre le secret" ou OEUVRE_NOIRE "Disséminer", pays). */
    [EVID_FUSILS_REVIENNENT] = { EVID_FUSILS_REVIENNENT, EV_COUNTRY, "Nos propres fusils nous reviennent",
        trig_fusils_reviennent, 1460.f, NULL, {
        { "Surenchère",
          "Des éclaireurs ont retrouvé nos marques d'arsenal sur les armes ennemies. Ce que nous avons vendu pour financer une guerre servira peut-être à gagner la suivante contre nous.",
          { .d_agitation=8.f, .d_treasury_mois=-1.6f, .d_breach=0.08f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Produisons une arme qu'ils ne possèdent pas encore. La course nous appartient tant que nous gardons une longueur d'avance." },
        { "Négocier le désarmement",
          "Des éclaireurs ont retrouvé nos marques d'arsenal sur les armes ennemies. Ce que nous avons vendu pour financer une guerre servira peut-être à gagner la suivante contre nous.",
          { .d_L=-0.15f, .d_influence=-2.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Réunissons les ambassadeurs autour d'une table avant que nos ouvriers ne se rencontrent sur un champ de bataille.",
          { .d_influence=2.f, .d_breach=-0.05f, .unlock_branch=-1 }, 0.35f },
        { "Guerre préventive",
          "Des éclaireurs ont retrouvé nos marques d'arsenal sur les armes ennemies. Ce que nous avons vendu pour financer une guerre servira peut-être à gagner la suivante contre nous.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_agitation=8.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          .flavor="Frappons les arsenaux maintenant. Mieux vaut affronter nos fusils dans leurs caisses que dans les mains de soldats." } }, 3 },

    /* K6 — LES SAVANTS SONT PASSÉS À L'ENNEMI (SCAR_FUITE_CERVEAUX mûrit — posée par
     * SAVOIR_INTERDIT "Exploiter sans le dire" ou "Bannir et brûler", pays). */
    [EVID_SAVANTS_ENNEMI] = { EVID_SAVANTS_ENNEMI, EV_COUNTRY, "Les savants sont passés à l'ennemi",
        trig_savants_ennemi, 1095.f, NULL, {
        { "Rappeler à prix d'or",
          "Nos anciens chercheurs signent désormais des traités à la cour adverse. Ils y disposent de meilleurs laboratoires et d'une rancune parfaitement financée.",
          { .d_K_inst=0.3f, .d_L=0.2f, .d_treasury_mois=-1.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Offrez-leur l'or, les titres et les excuses nécessaires. La fierté coûte moins cher qu'une génération de retard." },
        { "Espionner",
          "Nos anciens chercheurs signent désormais des traités à la cour adverse. Ils y disposent de meilleurs laboratoires et d'une rancune parfaitement financée.",
          { .d_agitation=4.f, .d_treasury_mois=-0.6f, .d_breach=0.05f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "S'ils refusent de revenir, que nos espions rapportent au moins leurs cahiers.",
          { .d_K_inst=0.3f, .unlock_branch=-1 }, 0.4f },
        { "Renoncer à la branche",
          "Nos anciens chercheurs signent désormais des traités à la cour adverse. Ils y disposent de meilleurs laboratoires et d'une rancune parfaitement financée.",
          { .d_L=0.2f, .d_C_global=-0.03f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          .flavor="Fermez cette voie de recherche. Un royaume doit savoir abandonner une porte lorsque l'ennemi en possède déjà la clé." } }, 3 },

    /* K7 — LES AUTRES VILLES ONT APPRIS LE TARIF (SCAR_EXEMPTION_ACHETEE mûrit — posée par
     * DEUX_CARTES "Laisser l'ambiguïté rapporter", PROVINCE (même clé de sujet que le
     * poseur — DEUX_CARTES est EV_PROVINCE, cf. note du trigger)). */
    [EVID_TARIF_APPRIS] = { EVID_TARIF_APPRIS, EV_PROVINCE, "Les autres villes ont appris le tarif de %s",
        trig_tarif_appris, 720.f, NULL, {
        { "Refuser toute exemption",
          "Les villes voisines ont observé les prélèvements de %s et adopté les mêmes méthodes. Les marchands ne parlent plus de fraude locale, mais d'un système.",
          { .d_L=0.2f, .d_H_coerc=0.3f, .d_coercion=0.12f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Aucune exemption. Si la règle doit nous rendre impopulaires, qu'elle ait au moins la décence de rapporter partout." },
        { "Tarifer officiellement",
          "Les villes voisines ont observé les prélèvements de %s et adopté les mêmes méthodes. Les marchands ne parlent plus de fraude locale, mais d'un système.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_treasury_mois=0.8f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EXEMPTION_ACHETEE, .cooldown_days=720 },
          "Inscrivons officiellement le tarif. Une pratique honteuse devient politique dès qu'elle reçoit un sceau." },
        { "Faire un exemple",
          "Les villes voisines ont observé les prélèvements de %s et adopté les mêmes méthodes. Les marchands ne parlent plus de fraude locale, mais d'un système.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.15f, .d_agitation=6.f, .d_treasury_mois=0.6f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Choisissez la ville la plus récalcitrante et faites-en une leçon que les autres sauront chiffrer.",
          { .d_agitation=-8.f, .unlock_branch=-1 }, 0.4f } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * V2b LOT 1 — LA MERVEILLE EN 3 ÉTAPES (pays, joueur seul). ANCRES : même
     * fourchette que le reste du module (d_L/d_H_coerc ≤0.6, d_agitation ≤16,
     * d_treasury_mois -1.6..0) — le legs ; K/H ancrent la couleur d'étape via
     * EvEffect DÉJÀ existant (d_K_inst/d_H_coerc/d_L), jamais un bonus plat.
     * EXEMPTÉS du plafond mondial (EV_CAPPED, cf. plus bas) : personnels au
     * joueur, leur propre cooldown (1200-36500 j) suffit.
     * ═══════════════════════════════════════════════════════════════════ */

    /* FONDATION — le monde reconnaît que le joueur a métabolisé les autres :
     * PAR LA FOI (lever GARDIEN, grief TRANSGRESSEUR) / PAR LA SCIENCE (lever
     * TRANSGRESSEUR, grief GARDIEN) / PAR LA FORCE (lever CONQUERANT, grief
     * COMMUNAUTAIRE). Chaque voie colore un bonus léger d'étape (K/H/L). */
    [EVID_MERV_FONDATION] = { EVID_MERV_FONDATION, EV_COUNTRY, "La première pierre du monde digéré",
        trig_merv_fondation, 1200.f, NULL, {
        { "Fonder par la foi",
          "Nos architectes promettent une Merveille qui résumera tout ce que la civilisation a conquis, appris et absorbé. Reste à décider ce qui en portera le poids.",
          { .d_K_inst=0.3f, .d_L=0.5f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Consacrons les fondations. Chaque peuple y reconnaîtra un ciel différent au-dessus d'un même temple." },
        { "Fonder par la science",
          "Nos architectes promettent une Merveille qui résumera tout ce que la civilisation a conquis, appris et absorbé. Reste à décider ce qui en portera le poids.",
          { .d_K_inst=0.5f, .d_H_coerc=0.2f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Mesurons, calculons et bâtissons sans invoquer d'autre miracle que l'intelligence accumulée." },
        { "Fonder par la force",
          "Nos architectes promettent une Merveille qui résumera tout ce que la civilisation a conquis, appris et absorbé. Reste à décider ce qui en portera le poids.",
          { .d_H_coerc=0.5f, .d_L=-0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_CONQUERANT, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Élevons-la sur les trophées de nos victoires. Le monde comprendra ce monument dans la langue universelle de la puissance." } }, 3 },

    /* CONSTRUCTION — LES DILEMMES DE SACRIFICE : le chantier réclame, récurrent
     * tant que la Merveille est active. Trois voies : le trésor (mois), les
     * bras (pop+agitation), ou ralentir (petit malus, pas de sacrifice —
     * L descend un peu : un chantier qu'on freine perd de sa superbe). */
    [EVID_MERV_SACRIFICE] = { EVID_MERV_SACRIFICE, EV_COUNTRY, "Ce que réclame la Merveille",
        trig_merv_sacrifice, 1200.f, NULL, {
        { "Saigner le trésor",
          "Le chantier dépasse chaque estimation. Les maîtres d'œuvre jurent qu'il peut continuer, pourvu que le royaume accepte enfin de nommer ce qu'il sacrifiera.",
          { .d_treasury_mois=-1.5f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Ouvrez le trésor et laissez l'or disparaître dans les fondations. Une génération paiera pour que les suivantes lèvent les yeux." },
        { "Saigner les bras",
          "Le chantier dépasse chaque estimation. Les maîtres d'œuvre jurent qu'il peut continuer, pourvu que le royaume accepte enfin de nommer ce qu'il sacrifiera.",
          { .pop_mult=0.99f, .d_agitation=10.f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Levez des corvées dans chaque province. La Merveille portera l'empreinte de toutes les mains, consentantes ou non." },
        { "Ralentir le chantier",
          "Le chantier dépasse chaque estimation. Les maîtres d'œuvre jurent qu'il peut continuer, pourvu que le royaume accepte enfin de nommer ce qu'il sacrifiera.",
          { .d_L=-0.2f, .unlock_branch=-1 },
          0.25f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Ralentissez. Un monument qui exige de détruire son peuple ne commémorera que notre vanité." } }, 3 },

    /* ASCENSION — LE DERNIER CHOIX, à MERV_SAVOIR_DONE, une fois (cooldown
     * énorme, motif SALVE_RUNIQUE). ACTIVER laisse le latch MERV_ASCENDED
     * suivre son cours (wonder_tick le déroulera au tick d'après — endgame_
     * select_and_fire lira le MERV_ASCENDED existant, rien à muter ici) ;
     * REFUSER fige merv (aucun mutateur : le palier reste MERV_SAVOIR_DONE,
     * gelé — wonder_tick ne le fait progresser QUE depuis les paliers actifs,
     * jamais depuis _DONE sans franchir ce trigger — un monument qui attend) ;
     * DÉTRUIRE court-circuite via endgame_start_wonder... non : DÉTRUIRE doit
     * ANNULER, pas refonder — on écrit directement eg->merv=MERV_NONE (le seul
     * champ qu'aucune API publique ne remet à zéro ; le module events A DÉJÀ
     * accès à EndgameState via cx->eg, struct PLATE, écriture directe sûre
     * comme apply_region_eff écrit re->build.* directement). */
    [EVID_MERV_ASCENSION] = { EVID_MERV_ASCENSION, EV_COUNTRY, "Au sommet, une dernière porte",
        trig_merv_ascension, 720.f, NULL, {
        { "Activer",
          "La Merveille est achevée, mais son cœur attend encore. L'activer pourrait transformer notre civilisation ; nul ne promet qu'elle demeurera reconnaissable.",
          { .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Nous avons gravi chaque marche pour atteindre cet instant. Ouvrons la porte et laissons l'histoire chercher à nous suivre." },
        { "Refuser",
          "La Merveille est achevée, mais son cœur attend encore. L'activer pourrait transformer notre civilisation ; nul ne promet qu'elle demeurera reconnaissable.",
          { .d_L=-0.6f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Laissons le cœur silencieux. La mortalité n'est pas une défaite lorsque l'alternative exige de cesser d'être soi." },
        { "Détruire",
          "La Merveille est achevée, mais son cœur attend encore. L'activer pourrait transformer notre civilisation ; nul ne promet qu'elle demeurera reconnaissable.",
          { .d_L=-1.0f, .d_agitation=16.f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_GARDIEN, .faction_strength=0.30f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Détruisez le mécanisme. Ce pouvoir ne doit appartenir ni à nos héritiers ni à ceux qui viendront les conquérir." } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * V2b LOT 2 — LE CONSEIL (signaux V2a). human gate (l'IA remplace déjà
     * son ministre au bord, cf. statecraft_council_ai_replace_count). Sujet =
     * le PAYS joueur (EV_COUNTRY) ; le siège concerné est retrouvé au moment
     * de la résolution (resolve_choice) par le même balayage que le trigger.
     * EXEMPTÉS du plafond mondial (personnels, cooldowns propres 1460-2555 j).
     * ═══════════════════════════════════════════════════════════════════ */

    /* Trahison — Savoir : le savant publie tes secrets. lot M : le %s de ces trois
     * gabarits = le MINISTRE assis (maison V2a, résolu par event_title — pas le pays).
     * P2 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) — le hook de faction de ces trois
     * options est RÉSOLU DYNAMIQUEMENT (F = faction réelle du titulaire du siège,
     * Opp(F) = sa plus opposée) dans le bloc conseil dédié de resolve_choice — le
     * hook STATIQUE ci-dessous reste NEUTRE (faction=-1) par construction. */
    [EVID_TRAHISON_SAVOIR] = { EVID_TRAHISON_SAVOIR, EV_COUNTRY, "%s publie les secrets du trône",
        trig_trahison_savoir, 1825.f, NULL, {
        { "Le faire taire",
          "%s a livré aux gazettes des documents que seuls le trône et le Conseil devaient connaître. La cour lit désormais ses propres secrets chez les colporteurs.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Un procès bref, une cellule profonde et plus aucune plume entre ses doigts." },
        { "Le renvoyer sans bruit",
          "%s a livré aux gazettes des documents que seuls le trône et le Conseil devaient connaître. La cour lit désormais ses propres secrets chez les colporteurs.",
          { .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Renvoyez-le sans éclat. Nous ne pouvons rappeler les pages, mais nous pouvons éviter d'en écrire une seconde édition." },
        { "Faire un exemple public",
          "%s a livré aux gazettes des documents que seuls le trône et le Conseil devaient connaître. La cour lit désormais ses propres secrets chez les colporteurs.",
          { .d_L=0.3f, .d_agitation=8.f, .d_influence=-2.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Jugez-le devant toute la cour. Que chaque conseiller voie combien de marches séparent encore son siège de l'échafaud." } }, 3 },

    /* Trahison — Société : le notable place ses familles. P2 : hook dynamique (cf. Savoir ci-dessus). */
    [EVID_TRAHISON_SOCIETE] = { EVID_TRAHISON_SOCIETE, EV_COUNTRY, "%s a placé ses familles avant le trône",
        trig_trahison_societe, 1825.f, NULL, {
        { "Purger les places",
          "%s a rempli les charges de cousins, d'alliés et de débiteurs. Dans certaines provinces, servir sa famille est devenu la seule manière de servir l'État.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Videz les bureaux et recommencez les nominations. Une place donnée par faveur peut être reprise par nécessité." },
        { "Composer",
          "%s a rempli les charges de cousins, d'alliés et de débiteurs. Dans certaines provinces, servir sa famille est devenu la seule manière de servir l'État.",
          { .d_L=-0.1f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Conservons son réseau contre un service que sa famille seule peut rendre. La vertu gouverne rarement sans quelques arrangements." },
        { "Laisser faire",
          "%s a rempli les charges de cousins, d'alliés et de débiteurs. Dans certaines provinces, servir sa famille est devenu la seule manière de servir l'État.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissons-les s'enraciner. Peut-être découvrirons-nous si la loyauté familiale peut, par accident, profiter au trône." } }, 3 },

    /* Trahison — Industrie : le marchand détourne. P2 : hook dynamique (cf. Savoir ci-dessus). */
    [EVID_TRAHISON_INDUSTRIE] = { EVID_TRAHISON_INDUSTRIE, EV_COUNTRY, "%s a détourné plus que sa part",
        trig_trahison_industrie, 1825.f, NULL, {
        { "Le poursuivre",
          "%s a prélevé sur les péages, les marchés et jusqu'aux pierres des routes. Les comptes sont faux, mais les caravanes continuent d'arriver à l'heure.",
          { .d_treasury_mois=0.3f, .d_agitation=4.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Saisissez ses livres et ses biens. Ce que la justice ne récupérera pas, elle le fera au moins regretter." },
        { "Négocier un remboursement",
          "%s a prélevé sur les péages, les marchés et jusqu'aux pierres des routes. Les comptes sont faux, mais les caravanes continuent d'arriver à l'heure.",
          { .d_treasury_mois=0.15f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Exigez un remboursement discret et complet. Un procès nourrit la rumeur ; un accord peut encore nourrir le trésor." },
        { "Fermer les yeux",
          "%s a prélevé sur les péages, les marchés et jusqu'aux pierres des routes. Les comptes sont faux, mais les caravanes continuent d'arriver à l'heure.",
          { .d_treasury_mois=-0.2f, .d_C_global=0.03f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Fermez les yeux tant que les routes restent ouvertes. Certains voleurs coûtent moins cher en fonction qu'en cellule." } }, 3 },

    /* ═══ ÂGES SANS ORDRE IMPOSÉ — « Le nom du siècle » (raccord 7, l'Âge des Héros) ═══
     * Un ministre rang III, encore assis, vient de mener une mission décennale à bien
     * avec efficacité ≥1.00 et loyauté ≥75 : SON siège consacre un héros. %s = « [Prénom]
     * [Maison], [titre] » (event_title, hero_seat_of ci-dessus) — jamais le pays. Les
     * hooks des trois options sont RÉSOLUS DYNAMIQUEMENT (la faction RÉELLE du titulaire,
     * cf. le bloc conseil dédié de resolve_choice) — le hook STATIQUE reste NEUTRE. Jamais
     * scanné par le balayage mtth (trig_never) : ages_hero_fire appelle fire_event
     * DIRECTEMENT depuis scps_sim.c, le fait est déjà avéré. */
    [EVID_HERO_SAVOIR] = { EVID_HERO_SAVOIR, EV_COUNTRY, "%s a mené à bien sa décennie",
        trig_never, 0.f, NULL, {
        { "Lui confier la prochaine décennie",
          "%s a mené à bien une décennie de labeur au service du Savoir : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il poursuive son œuvre. Une décennie de plus, avec toute notre confiance derrière lui." },
        { "Lui donner les clefs",
          "%s a mené à bien une décennie de labeur au service du Savoir : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il tienne les clefs autant qu'il le voudra. Le mérite, à ce niveau, se paie d'un peu de pouvoir." },
        { "Il a fait son devoir",
          "%s a mené à bien une décennie de labeur au service du Savoir : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Le devoir accompli n'appelle pas de récompense. Qu'il retourne à son pupitre, satisfait d'avoir servi." } }, 3 },
    [EVID_HERO_SOCIETE] = { EVID_HERO_SOCIETE, EV_COUNTRY, "%s a mené à bien sa décennie",
        trig_never, 0.f, NULL, {
        { "Lui confier la prochaine décennie",
          "%s a mené à bien une décennie de labeur au service du Royaume : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il poursuive son œuvre. Une décennie de plus, avec toute notre confiance derrière lui." },
        { "Lui donner les clefs",
          "%s a mené à bien une décennie de labeur au service du Royaume : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il tienne les clefs autant qu'il le voudra. Le mérite, à ce niveau, se paie d'un peu de pouvoir." },
        { "Il a fait son devoir",
          "%s a mené à bien une décennie de labeur au service du Royaume : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Le devoir accompli n'appelle pas de récompense. Qu'il retourne à son pupitre, satisfait d'avoir servi." } }, 3 },
    [EVID_HERO_INDUSTRIE] = { EVID_HERO_INDUSTRIE, EV_COUNTRY, "%s a mené à bien sa décennie",
        trig_never, 0.f, NULL, {
        { "Lui confier la prochaine décennie",
          "%s a mené à bien une décennie de labeur au service des Ouvrages : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il poursuive son œuvre. Une décennie de plus, avec toute notre confiance derrière lui." },
        { "Lui donner les clefs",
          "%s a mené à bien une décennie de labeur au service des Ouvrages : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Qu'il tienne les clefs autant qu'il le voudra. Le mérite, à ce niveau, se paie d'un peu de pouvoir." },
        { "Il a fait son devoir",
          "%s a mené à bien une décennie de labeur au service des Ouvrages : son nom passera à la postérité. La cour attend de savoir ce que le trône en fera.",
          { .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=0 },
          "Le devoir accompli n'appelle pas de récompense. Qu'il retourne à son pupitre, satisfait d'avoir servi." } }, 3 },

    /* SUCCESSION — la retraite d'un loyal (>20 ans). Un moment, deux choix légers.
     * P2 : les DEUX options retirent IMMÉDIATEMENT le titulaire, SANS le grief de
     * renvoi (bypass COUNCIL_DISMISS_GRIEF — résolu dans le bloc conseil dédié). */
    [EVID_CONSEIL_SUCCESSION] = { EVID_CONSEIL_SUCCESSION, EV_COUNTRY, "Le dernier jour d'un serviteur loyal",
        trig_conseil_succession, 1460.f, NULL, {
        { "Le remercier publiquement",
          "Après des décennies au Conseil, un ministre demande à quitter son siège. Il n'emporte qu'une canne, une mémoire dangereuse et l'espoir d'être remercié.",
          { .d_L=0.3f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Que la cour se lève pour lui. La loyauté future se nourrit du souvenir de ceux que le trône n'a pas oubliés." },
        { "Le laisser partir sans bruit",
          "Après des décennies au Conseil, un ministre demande à quitter son siège. Il n'emporte qu'une canne, une mémoire dangereuse et l'espoir d'être remercié.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Acceptez sa lettre et laissez-le partir. Tous les services n'ont pas besoin d'une cérémonie pour avoir compté." } }, 2 },

    /* R1 — Savoir vs Société en RIVALITÉ : trancher pour l'un/l'autre/renvoyer les deux.
     * P2 : « Trancher » biaise la faction RÉELLE du titulaire choisi (résolu dans le
     * bloc conseil dédié de resolve_choice) — le hook statique reste NEUTRE. */
    [EVID_CONSEIL_R1] = { EVID_CONSEIL_R1, EV_COUNTRY, "Le Savoir et la Société se disputent l'oreille du trône",
        trig_conseil_r1, 1825.f, NULL, {
        { "Trancher pour le Savoir",
          "Le ministre du Savoir accuse les grandes familles d'étouffer le mérite. Leur représentant répond que les savants veulent gouverner un peuple qu'ils ne fréquentent jamais.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le royaume a besoin de compétences avant d'avoir besoin de susceptibilités. Le Savoir aura notre oreille." },
        { "Trancher pour la Société",
          "Le ministre du Savoir accuse les grandes familles d'étouffer le mérite. Leur représentant répond que les savants veulent gouverner un peuple qu'ils ne fréquentent jamais.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une idée brillante ne tient pas une province sans les familles qui y vivent. La Société l'emportera." },
        { "Renvoyer les deux",
          "Le ministre du Savoir accuse les grandes familles d'étouffer le mérite. Leur représentant répond que les savants veulent gouverner un peuple qu'ils ne fréquentent jamais.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Ils confondent conseil et querelle. Qu'ils poursuivent la seconde loin du premier." } }, 3 },

    /* R2 — Industrie vs Société : la route. P2 : hook dynamique (cf. R1 ci-dessus). */
    [EVID_CONSEIL_R2] = { EVID_CONSEIL_R2, EV_COUNTRY, "L'Industrie et la Société se disputent la route",
        trig_conseil_r2, 1825.f, NULL, {
        { "Trancher pour l'Industrie",
          "L'Industrie veut ouvrir une route à travers les terres ancestrales ; la Société jure qu'aucun pavé ne sera posé sans l'accord des familles.",
          { .d_treasury_mois=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Ouvrez la route. Une coutume qui interdit tout passage finit seulement par contourner ceux qu'elle protège." },
        { "Trancher pour la Société",
          "L'Industrie veut ouvrir une route à travers les terres ancestrales ; la Société jure qu'aucun pavé ne sera posé sans l'accord des familles.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Respectez les terres des familles. Le commerce trouvera un détour plus facilement que le trône ne réparera une offense." },
        { "Renvoyer les deux",
          "L'Industrie veut ouvrir une route à travers les terres ancestrales ; la Société jure qu'aucun pavé ne sera posé sans l'accord des familles.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Ni privilège marchand ni veto familial. Le projet attendra des défenseurs moins intéressés." } }, 3 },

    /* R3 — Savoir vs Industrie : le cadastre. P2 : hook dynamique (cf. R1 ci-dessus). */
    [EVID_CONSEIL_R3] = { EVID_CONSEIL_R3, EV_COUNTRY, "Le Savoir et l'Industrie se disputent le cadastre",
        trig_conseil_r3, 1825.f, NULL, {
        { "Trancher pour le Savoir",
          "Le Savoir réclame un cadastre exact ; l'Industrie, un cadastre utile. Depuis trois séances, ils déplacent les mêmes frontières avec des règles différentes.",
          { .d_K_inst=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Mesurez d'abord, exploitez ensuite. Une carte fausse reste fausse même lorsqu'elle rapporte." },
        { "Trancher pour l'Industrie",
          "Le Savoir réclame un cadastre exact ; l'Industrie, un cadastre utile. Depuis trois séances, ils déplacent les mêmes frontières avec des règles différentes.",
          { .d_treasury_mois=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Que la carte serve les routes et les marchés. La perfection qui retarde tout n'est qu'une autre erreur." },
        { "Renvoyer les deux",
          "Le Savoir réclame un cadastre exact ; l'Industrie, un cadastre utile. Depuis trois séances, ils déplacent les mêmes frontières avec des règles différentes.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Reprenez les cartes et les sceaux. Le royaume peut attendre une semaine ; leur vanité, davantage." } }, 3 },

    /* A1 — L'ALLIANCE de deux sièges : laisser la synergie ×rot / contrebalancer / séparer.
     * P2 : Laisser faire = +0,05 biais aux DEUX alliées (factions réelles) · Contrebalancer
     * = +0,05 au 3e siège (vacant ⇒ aucun hook) · Séparer = renvoi du 2e (dismiss(), qui
     * grief déjà SA faction +0,10) — tout résolu dans le bloc conseil dédié. */
    [EVID_CONSEIL_A1] = { EVID_CONSEIL_A1, EV_COUNTRY, "Deux sièges du Conseil s'entendent trop bien",
        trig_conseil_a1, 1825.f, NULL, {
        { "Laisser faire",
          "Deux ministres votent désormais ensemble avant même d'entendre les dossiers. Le Conseil gagne en vitesse ce que le trône perd peut-être en liberté.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissons cette entente produire ses fruits. Un Conseil efficace mérite parfois d'inquiéter son souverain." },
        { "Contrebalancer",
          "Deux ministres votent désormais ensemble avant même d'entendre les dossiers. Le Conseil gagne en vitesse ce que le trône perd peut-être en liberté.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Élevons discrètement une troisième voix. L'équilibre est plus sûr lorsqu'aucun allié ne le remarque." },
        { "Séparer",
          "Deux ministres votent désormais ensemble avant même d'entendre les dossiers. Le Conseil gagne en vitesse ce que le trône perd peut-être en liberté.",
          { .d_H_coerc=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Éloignez l'un des deux. Deux serviteurs trop unis peuvent finir par oublier qui ils servent." } }, 3 },

    /* A2 — leur candidat au 3e siège. P2 : « Accepter leur candidat » NOMME RÉELLEMENT
     * (le bloc conseil dédié de resolve_choice choisit + statecraft_council_hire, qui
     * pousse DÉJÀ la faction du candidat ~0.10 — pas de hook statique redondant ici). */
    [EVID_CONSEIL_A2] = { EVID_CONSEIL_A2, EV_COUNTRY, "L'alliance du Conseil propose son candidat au 3e siège",
        trig_conseil_a2, 1825.f, NULL, {
        { "Accepter leur candidat",
          "Les deux ministres alliés proposent d'une seule voix leur candidat au siège vacant. Ils promettent l'harmonie avec un empressement qui ressemble à une condition.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Acceptons leur candidat. Un Conseil uni agira vite, et portera ensemble la responsabilité de ses erreurs." },
        { "Imposer son propre choix",
          "Les deux ministres alliés proposent d'une seule voix leur candidat au siège vacant. Ils promettent l'harmonie avec un empressement qui ressemble à une condition.",
          { .d_H_coerc=0.1f, .d_agitation=4.f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le dernier siège appartient au trône. Qu'ils apprennent que deux voix ne font pas encore une couronne." },
        { "Laisser le siège vacant",
          "Les deux ministres alliés proposent d'une seule voix leur candidat au siège vacant. Ils promettent l'harmonie avec un empressement qui ressemble à une condition.",
          { .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissez le fauteuil vide. Un silence au Conseil peut être préférable à une majorité déjà écrite." } }, 3 },

    /* C1 — LA CONSPIRATION : les renvoyer les deux / en sacrifier un / céder.
     * P2 : « Renvoyer les deux »/« En sacrifier un » n'ont RIEN à ajouter — dismiss()
     * grief DÉJÀ (+0,10) la faction propre de chaque renvoyé, SANS capture (aucun appel
     * à faction_concede) — exactement la rancœur demandée. « Céder » (SEUL choix qui
     * abandonne une part durable de l'État) appelle une DOUBLE concession (+9 Corr,
     * +0,06 biais chacune) dans le bloc conseil dédié — impossible via le hook générique
     * à UNE faction. */
    [EVID_CONSEIL_C1] = { EVID_CONSEIL_C1, EV_COUNTRY, "Deux sièges aliénés complotent contre le trône",
        trig_conseil_c1, 2555.f, NULL, {
        { "Les renvoyer les deux",
          "Des lettres interceptées prouvent que deux ministres coordonnent leurs oppositions et leurs nominations. Ils ne parlent pas encore de renverser le trône, seulement de le rendre docile.",
          { .d_H_coerc=0.5f, .d_L=0.2f, .d_agitation=10.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Renvoyez-les ensemble avant qu'ils ne découvrent combien de conspirateurs tiennent dans une majorité." },
        { "En sacrifier un",
          "Des lettres interceptées prouvent que deux ministres coordonnent leurs oppositions et leurs nominations. Ils ne parlent pas encore de renverser le trône, seulement de le rendre docile.",
          { .d_H_coerc=0.3f, .d_L=0.1f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Sacrifiez le plus coupable et gardez l'autre sous vos yeux. La peur fera peut-être ce que la loyauté n'a pas su faire." },
        { "Céder",
          "Des lettres interceptées prouvent que deux ministres coordonnent leurs oppositions et leurs nominations. Ils ne parlent pas encore de renverser le trône, seulement de le rendre docile.",
          { .d_L=-0.6f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Accordons-leur ce qu'ils demandent. Un trône qui plie aujourd'hui peut encore choisir quand se redresser." } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * V2b LOT 3 — LE CONTENU DÉBLOQUÉ (lecteurs P7a). ANCRES : même fourchette
     * que le reste du module. human gate (comme LOT 1/2, cf. les triggers).
     * EXEMPTÉS du plafond mondial (personnels, cooldowns 1460-2555 j).
     * ═══════════════════════════════════════════════════════════════════ */

    /* B2 — deux cultures voisines ne s'accordent plus (rivalité de voisinage). */
    [EVID_RIVAUX_VOISINS] = { EVID_RIVAUX_VOISINS, EV_COUNTRY, "Deux peuples voisins ne s'accordent plus sur rien",
        trig_rivaux_voisins, 1825.f, NULL, {
        { "Fermer la frontière",
          "Deux peuples frontaliers se disputent désormais les pâturages, les marchés et jusqu'à l'origine d'une chanson. Une étincelle suffirait à rendre ces querelles héréditaires.",
          { .d_H_coerc=0.2f, .d_C_global=-0.05f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_CONQUERANT, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Fermez la frontière jusqu'à ce que la distance refroidisse ce que la proximité envenime." },
        { "Ouvrir un dialogue",
          "Deux peuples frontaliers se disputent désormais les pâturages, les marchés et jusqu'à l'origine d'une chanson. Une étincelle suffirait à rendre ces querelles héréditaires.",
          { .d_L=0.2f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Envoyez des émissaires et faites dresser une table au milieu du pont. Qu'ils se voient avant de s'imaginer." },
        { "Attiser la rivalité",
          "Deux peuples frontaliers se disputent désormais les pâturages, les marchés et jusqu'à l'origine d'une chanson. Une étincelle suffirait à rendre ces querelles héréditaires.",
          { .d_agitation=6.f, .d_influence=1.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_CONQUERANT, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Nourrissez leur rivalité avec soin. Deux voisins occupés à se haïr regardent moins souvent vers le trône." } }, 3 },

    /* B3 — une parenté lointaine se souvient. */
    [EVID_PARENTE_LOINTAINE] = { EVID_PARENTE_LOINTAINE, EV_COUNTRY, "Des cousins venus d'un autre monde",
        trig_parente_lointaine, 1825.f, NULL, {
        { "Renouer",
          "Une ambassade inconnue parle une langue qui ressemble à la nôtre dans les prières et les insultes. Elle affirme que nos peuples partagent un ancêtre oublié.",
          { .d_L=0.2f, .d_influence=2.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Recevons-les comme des parents que l'histoire a séparés. L'or ouvrira l'ambassade ; la curiosité fera le reste." },
        { "Rester distant",
          "Une ambassade inconnue parle une langue qui ressemble à la nôtre dans les prières et les insultes. Elle affirme que nos peuples partagent un ancêtre oublié.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Un air de famille ne suffit pas à partager une frontière ou un secret. Restons courtois et lointains." },
        { "Exploiter la parenté",
          "Une ambassade inconnue parle une langue qui ressemble à la nôtre dans les prières et les insultes. Elle affirme que nos peuples partagent un ancêtre oublié.",
          { .d_treasury_mois=0.3f, .d_influence=-1.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_MARCHAND, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "S'ils nous appellent cousins, rappelons-leur ce que la parenté doit aux aînés, en présents comme en faveur." } }, 3 },

    /* B6 — une région marche loin de l'éthos régnant (province). */
    [EVID_MARCHE_ETHOS] = { EVID_MARCHE_ETHOS, EV_PROVINCE, "À %s, l'éthos ne ressemble plus à celui du trône",
        trig_marche_ethos, 1825.f, NULL, {
        { "Ramener dans le rang",
          "À %s, les magistrats invoquent des valeurs que la capitale ne reconnaît plus comme siennes. La marche obéit encore, mais elle ne se raconte plus la même histoire.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_coercion=0.12f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Rappelez les magistrats et les principes qui les ont nommés. Une frontière politique commence souvent par une différence tolérée trop longtemps." },
        { "Laisser sa différence",
          "À %s, les magistrats invoquent des valeurs que la capitale ne reconnaît plus comme siennes. La marche obéit encore, mais elle ne se raconte plus la même histoire.",
          { .d_L=0.2f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissons la marche penser autrement tant qu'elle agit loyalement. L'uniformité n'est pas la seule forme de l'unité." },
        { "En faire un exemple d'autonomie",
          "À %s, les magistrats invoquent des valeurs que la capitale ne reconnaît plus comme siennes. La marche obéit encore, mais elle ne se raconte plus la même histoire.",
          { .d_K_inst=0.2f, .d_L=0.1f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Faisons de sa différence un privilège reconnu. Ce que nous accordons librement ne pourra pas nous être arraché comme une victoire." } }, 3 },

    /* C2 — le décret de tolérance (fracture religieuse notable). */
    [EVID_TOLERANCE_CREDO] = { EVID_TOLERANCE_CREDO, EV_COUNTRY, "Le décret de tolérance attend une signature",
        trig_tolerance_credo, 1825.f, NULL, {
        { "Signer le décret",
          "Le décret attend sur votre bureau. D'un trait, chaque culte obtiendra le droit de prier ouvertement ; d'un refus, chacun saura quelle foi possède vraiment le royaume.",
          { .d_L=0.3f, .d_agitation=-8.f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Signez. Le trône jugera les actes, et laissera aux dieux la tâche plus ingrate de juger les âmes." },
        { "Refuser",
          "Le décret attend sur votre bureau. D'un trait, chaque culte obtiendra le droit de prier ouvertement ; d'un refus, chacun saura quelle foi possède vraiment le royaume.",
          { .d_H_coerc=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Refusez. Une seule loi ne tiendra pas longtemps si elle doit s'agenouiller devant plusieurs vérités." },
        { "Tolérance limitée",
          "Le décret attend sur votre bureau. D'un trait, chaque culte obtiendra le droit de prier ouvertement ; d'un refus, chacun saura quelle foi possède vraiment le royaume.",
          { .d_L=0.1f, .d_agitation=-3.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Tolérons leurs temples, leurs offices et rien de plus. La liberté restera sous surveillance, ce qui rassurera au moins ses adversaires." } }, 3 },

    /* C3 — le lettré porte une face périmée. */
    [EVID_LETTRE_PERIME] = { EVID_LETTRE_PERIME, EV_COUNTRY, "Le lettré du trône prêche une foi d'hier",
        trig_lettre_perime, 1825.f, NULL, {
        { "Le remplacer",
          "Le lettré du trône cite encore une doctrine officiellement corrigée depuis des années. La foule l'écoute avec respect ; les nouveaux théologiens, avec une colère croissante.",
          { .d_L=0.2f, .d_treasury_mois=-0.5f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Remplacez-le par une voix qui parle la foi présente, pas le souvenir de celle qu'elle fut." },
        { "Le laisser prêcher",
          "Le lettré du trône cite encore une doctrine officiellement corrigée depuis des années. La foule l'écoute avec respect ; les nouveaux théologiens, avec une colère croissante.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Laissez le vieil homme à ses sermons. Une formule périmée n'est dangereuse que lorsqu'on lui offre des persécuteurs." },
        { "Le corriger publiquement",
          "Le lettré du trône cite encore une doctrine officiellement corrigée depuis des années. La foule l'écoute avec respect ; les nouveaux théologiens, avec une colère croissante.",
          { .d_L=0.1f, .d_agitation=-2.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_GARDIEN, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Corrigez-le devant ses étudiants et maintenez-le en charge. Il enseignera désormais aussi l'humilité." } }, 3 },

    /* C4 — la pratique dérive de la foi professée (crise plus profonde que C2). */
    [EVID_PRATIQUE_DERIVE] = { EVID_PRATIQUE_DERIVE, EV_COUNTRY, "La pratique du peuple ne ressemble plus à la foi qu'il professe",
        trig_pratique_derive, 2190.f, NULL, {
        { "Réaffirmer la doctrine",
          "Le peuple professe toujours la foi officielle, mais ses mariages, deuils et fêtes suivent désormais d'autres usages. La doctrine règne sur les mots ; la pratique, sur les vies.",
          { .d_H_coerc=0.3f, .d_L=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "Renvoyez les prédicateurs et rappelez la règle. Une foi qui ne façonne plus les gestes n'est bientôt qu'un nom." },
        { "Laisser la pratique dériver",
          "Le peuple professe toujours la foi officielle, mais ses mariages, deuils et fêtes suivent désormais d'autres usages. La doctrine règne sur les mots ; la pratique, sur les vies.",
          { .d_L=-0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "Laissons vivre ce qui a su changer. Une croyance immobile finit parfois plus morte que fidèle." },
        { "Codifier la nouvelle pratique",
          "Le peuple professe toujours la foi officielle, mais ses mariages, deuils et fêtes suivent désormais d'autres usages. La doctrine règne sur les mots ; la pratique, sur les vies.",
          { .d_K_inst=0.3f, .d_L=0.3f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "Écrivons ce que le peuple pratique déjà. Pour une fois, les docteurs suivront les fidèles." } }, 3 },
};

const EventDef *event_def(int evid){ return (evid>=0&&evid<EVID_COUNT)?&EVENTS[evid]:NULL; }

/* lot M — NOMS DE MINISTRE dans les trahisons du Conseil : le %s de ces trois
 * gabarits désigne le MINISTRE assis (la maison V2a), pas le pays. event_title
 * garde sa signature publique (scps_events.h appartient à un autre lot) : le
 * Statecraft courant est LATCHÉ par world_events_tick — DISPLAY-ONLY, jamais
 * sérialisé ni lu par un chemin moteur ; NULL (avant le 1er tick / bancs) ou
 * siège vacant (ministre déjà renvoyé) ⇒ repli sur le mot de classe. */
static const Statecraft *g_title_sc = NULL;
static int treason_seat_of(int evid){
    switch (evid){
        case EVID_TRAHISON_SAVOIR:    return 0;
        case EVID_TRAHISON_SOCIETE:   return 1;
        case EVID_TRAHISON_INDUSTRIE: return 2;
        default:                      return -1;
    }
}
/* P2 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) — fallback de nom PAR SIÈGE (le mot
 * de la charge, jamais un mot de classe fixe qui trahirait la faction assise —
 * « Le marchand » suggérait FAC_MARCHAND à tort quel que soit le titulaire réel). */
static const char *TREASON_FALLBACK[3] = { "Le conseiller du Savoir", "Le conseiller du Royaume", "Le conseiller des Ouvrages" };

/* raccord 7 (Âge des Héros) — même motif que treason_seat_of, sur les 3 EVID_HERO_*. */
static int hero_seat_of(int evid){
    switch (evid){
        case EVID_HERO_SAVOIR:    return 0;
        case EVID_HERO_SOCIETE:   return 1;
        case EVID_HERO_INDUSTRIE: return 2;
        default:                  return -1;
    }
}
/* TITRES (spec verbatim, docs/AGES_FINS_2026-07-11.md) : Savoir INVARIANT (« Grand
 * Esprit », aucun genre donné) ; Société/Industrie genrés sur l'index de prénom
 * (statecraft_council_cand_female — 12 premiers masculins, 12 suivants féminins). */
static const char *hero_title(int seat, bool female){
    switch (seat){
        case 0:  return "Grand Esprit";
        case 1:  return female ? "Mère de la nation"  : "Père de la nation";
        case 2:  return female ? "Grande Capitaine"   : "Grand Capitaine";
        default: return "Héros du siècle";
    }
}

/* TITRE PRÉSENTÉ (display-only) : le nom de la table est un GABARIT — s'il porte
 * un « %s », on y coud le NOM RÉEL du sujet (région en EV_PROVINCE, pays en
 * EV_COUNTRY, MINISTRE pour les trahisons du Conseil) au moment de la PRÉSENTATION
 * (« Marbrive » était un placeholder de registre : le monde a ses propres noms).
 * Assemblage MANUEL (pas de format non littéral) ; buf est rendu pour chaîner les appels. */
const char *event_title(const World *w, int evid, int subject, char *buf, int n){
    const EventDef *d = event_def(evid);
    if (!buf || n<=0) return "";
    if (!d){ buf[0]='\0'; return buf; }
    const char *p = strstr(d->name, "%s");
    if (!p || !w){ snprintf(buf, (size_t)n, "%s", d->name); return buf; }
    const char *nom = "?";
    int hseat = hero_seat_of(evid);
    int seat = (hseat>=0) ? -1 : treason_seat_of(evid);
    if (hseat >= 0){
        /* raccord 7 — « [Prénom] [Maison], [titre] » (spec verbatim). Fallback
         * (siège vacant / avant le 1er tick) : le titre seul, générique. */
        nom = hero_title(hseat, false);
        if (g_title_sc && subject>=0 && subject<w->n_countries){
            int slot = statecraft_council_seated(g_title_sc, subject, hseat);
            if (slot >= 0){
                static char heronom[160];
                int gen = statecraft_council_seated_gen(g_title_sc, subject, hseat);
                bool female = statecraft_council_cand_female(w->seed, subject, hseat, slot, gen);
                snprintf(heronom, sizeof heronom, "%s %s, %s",
                        statecraft_council_cand_firstname(w->seed, subject, hseat, slot, gen),
                        statecraft_council_cand_house(w->seed, subject, hseat, slot, gen),
                        hero_title(hseat, female));
                nom = heronom;
            }
        }
    } else if (seat >= 0){
        nom = TREASON_FALLBACK[seat];
        if (g_title_sc && subject>=0 && subject<w->n_countries){
            int slot = statecraft_council_seated(g_title_sc, subject, seat);
            if (slot >= 0){
                int gen = statecraft_council_seated_gen(g_title_sc, subject, seat);
                nom = tr((StrId)statecraft_council_cand_name(w->seed, subject, seat, slot, gen));
            }
        }
    } else {
        if (d->scope==EV_PROVINCE  && subject>=0 && subject<w->n_regions)   nom = w->region[subject].name;
        if (d->scope==EV_COUNTRY   && subject>=0 && subject<w->n_countries) nom = w->country[subject].name;
    }
    snprintf(buf, (size_t)n, "%.*s%s%s", (int)(p - d->name), d->name, nom, p+2);
    return buf;
}
/* Anneau de titres pour les CONSOMMATEURS À POINTEUR (provlog stocke le pointeur,
 * pas une copie) : 32 tirs de recyclage — un journal provincial a tourné bien
 * avant (display-only, jamais lu par le moteur). */
static const char *event_title_ring(const World *w, int evid, int subject){
    static char ring[32][96]; static int head = 0;
    char *b = ring[head]; head = (head+1) & 31;
    return event_title(w, evid, subject, b, 96);
}

/* ===================================================================== */
/* APPLICATION DES EFFETS (déplace des coordonnées/métriques)            */
/* ===================================================================== */
static void apply_region_eff(EventCtx *cx, int r, const EvEffect *e){
    if (r<0||r>=cx->econ->n_regions) return;
    /* RE-KEY PROVINCE : build.K_inst/H_coerc/food_cap, coercion, treasury, strata sont
     * PROVINCE-OWNED (charte règle 1) — econ->region[r] est un DÉRIVÉ (econ_aggregate_regions
     * le reconstruit
     * ENTIER à chaque econ_tick), un écrivain direct s'y perdrait au tick suivant. Route
     * sur la province représentative de la région (grain public historique inchangé
     * pour les appelants : ils passent toujours une région). */
    int pid=econ_region_rep_province(cx->econ, r);
    if (pid<0 || pid>=cx->econ->n_prov) return;
    ProvinceEconomy *re=&cx->econ->prov[pid];
    re->build.K_inst  = fmaxf(0.f, re->build.K_inst  + e->d_K_inst);
    re->build.H_coerc = fmaxf(0.f, re->build.H_coerc + e->d_H_coerc);
    re->build.food_cap= fmaxf(0.f, re->build.food_cap+ e->d_food_cap);
    re->coercion      = clampf(re->coercion + e->d_coercion, 0.f, 1.f);
    re->treasury      = fmaxf(0.f, re->treasury + e->d_treasury);
    /* ESCLAVAGE — FUITE #9 : un évènement (peste/famine/vague migratoire…) multiplie la pop
     * de TOUTES les strates — mais aucun évènement ne touche les PopGroup (ce module ne les
     * connaît même pas). CLASS_SLAVE en sortait donc désynchronisée de Σgroupes klass==
     * CLASS_SLAVE (mesuré SLAVEDIAG seed 10 : le drop 40→34→28… coïncide avec la fenêtre
     * mensuelle, pas le bloc annuel — un évènement générique en est la source). La strate
     * TENUE est exclue du multiplicateur générique (elle ne bouge que par capture/achat/
     * vente/mobilisation-de-révolte, §II.6, H). */
    if (e->pop_mult>0.f && e->pop_mult!=1.f)
        for (int k=0;k<CLASS_COUNT;k++) if (k!=CLASS_SLAVE) re->strata[k].pop *= e->pop_mult;
    if (cx->wl && r<SCPS_MAX_REG) cx->wl->L[r]=clampf(cx->wl->L[r]+e->d_L,0.f,10.f);
    if (cx->sc && r<SCPS_MAX_REG) cx->sc->agitation[r]=clampf(cx->sc->agitation[r]+e->d_agitation,0.f,100.f);
}
/* MEMBRANE DE DÉCISION — d_treasury_mois : fraction SIGNÉE du revenu MENSUEL du pays
 * (Σtaxes de l'an écoulé / 12), déflatée par l'IPM courant (un même choix pèse pareil
 * en monnaie constante quel que soit l'âge de la partie), routée sur la région-SUJET
 * (EV_PROVINCE) ou la CAPITALE (EV_COUNTRY) via econ_region_treasury_add (la province
 * représentative — jamais un écrivain direct sur l'agrégat région). */
static void resolve_treasury_mois(EventCtx *cx, int cid, int region, const EvEffect *e){
    if (e->d_treasury_mois==0.f || region<0) return;
    float revenu_mois = econ_country_tax_year(cid) / 12.f;
    float montant = e->d_treasury_mois * revenu_mois * econ_world_ipm(cx->econ);
    econ_region_treasury_add(cx->econ, region, montant);
}
static void apply_effect(EventCtx *cx, EvScope scope, int subject, const EvEffect *e){
    if (scope==EV_WORLD){
        if (cx->wp){
            cx->wp->age_C_bonus     = clampf(cx->wp->age_C_bonus + e->d_C_global, 0.f, 5.f);
            cx->wp->age_breach_flux = clampf(cx->wp->age_breach_flux + e->d_breach, 0.f, 10.f);
            /* raccord 3 — tier_open vit désormais sur wp (bitmask age_tech_mask :
             * ai_research_step a `wp` sous la main, jamais `ev` — cf. scps_events.h). */
            if (e->unlock_branch>=0 && e->unlock_branch<THM_COUNT && e->unlock_tier>=0 && e->unlock_tier<8){
                unsigned bit = (unsigned)e->unlock_branch*8u + (unsigned)e->unlock_tier;
                if (bit<32) cx->wp->age_tech_mask |= (1u<<bit);
            }
        }
        cx->ev->ages.breach_pressure = clampf(cx->ev->ages.breach_pressure + e->d_breach, 0.f, 10.f);
        return;
    }
    if (scope==EV_PROVINCE){
        apply_region_eff(cx, subject, e);
        int cid = (subject>=0&&subject<cx->econ->n_regions)?cx->econ->region[subject].owner:-1;
        resolve_treasury_mois(cx, cid, subject, e);
        if (cid>=0 && cx->sc && cid<cx->sc->n_countries)
            cx->sc->influence[cid]=clampf(cx->sc->influence[cid]+e->d_influence,0.f,100.f);
    } else { /* EV_COUNTRY : cœur à la capitale, humeur (L/agitation) au pays entier */
        int cid=subject, capr=world_capital_region(cx->w,cid);
        if (capr>=0) apply_region_eff(cx, capr, e);
        resolve_treasury_mois(cx, cid, capr, e);
        for (int r=0;r<cx->econ->n_regions;r++) if (r!=capr && cx->econ->region[r].owner==cid){
            if (cx->wl && r<SCPS_MAX_REG) cx->wl->L[r]=clampf(cx->wl->L[r]+e->d_L,0.f,10.f);
            if (cx->sc && r<SCPS_MAX_REG) cx->sc->agitation[r]=clampf(cx->sc->agitation[r]+e->d_agitation,0.f,100.f);
        }
        if (cid>=0 && cx->sc && cid<cx->sc->n_countries)
            cx->sc->influence[cid]=clampf(cx->sc->influence[cid]+e->d_influence,0.f,100.f);
    }
}

/* ===================================================================== */
/* MÉMOIRE DE DÉCISION — cicatrices (anneau) + cooldowns (anneau)          */
/* ===================================================================== */
void decision_memory_push(EventsState *ev, int subject, ScarKind kind,
                          int delai_min_j, int delai_max_j){
    if (!ev || kind<0 || kind>=SCAR_KIND_COUNT) return;
    int lo=delai_min_j, hi=delai_max_j; if (hi<lo) hi=lo;
    int delai = lo + (int)(frand(&ev->rng) * (float)(hi-lo+1));
    DecisionScar *s = &ev->scars[ev->scar_head];
    s->subject = (int16_t)subject; s->kind = (int8_t)kind;
    s->ripe_day = ev->ages.days_elapsed + delai;
    ev->scar_head = (ev->scar_head+1) % DECISION_SCAR_CAP;
}
bool decision_memory_has_ripe(const EventsState *ev, int subject, ScarKind kind, int today){
    if (!ev) return false;
    for (int i=0;i<DECISION_SCAR_CAP;i++){
        const DecisionScar *s=&ev->scars[i];
        if (s->subject==(int16_t)subject && s->kind==(int8_t)kind && s->ripe_day<=today && s->ripe_day>0)
            return true;
    }
    return false;
}
void decision_memory_consume(EventsState *ev, int subject, ScarKind kind, int today){
    if (!ev) return;
    for (int i=0;i<DECISION_SCAR_CAP;i++){
        DecisionScar *s=&ev->scars[i];
        if (s->subject==(int16_t)subject && s->kind==(int8_t)kind && s->ripe_day<=today && s->ripe_day>0){
            s->subject=-1; s->kind=0; s->ripe_day=0;   /* case vide (ripe_day=0 ⇒ jamais mûre) */
            return;
        }
    }
}

/* CONTENU W1 — EVID_DERNIERE_DECISION : trouve la cicatrice PENDANTE (ripe_day futur,
 * kind quelconque) la plus proche de mûrir sur `subject`. -1 si aucune. Balayage
 * d'index croissant, déterministe (départage par la première trouvée à égalité). */
static int decision_memory_find_pending(const EventsState *ev, int subject, int today){
    int best=-1, best_day=0x7fffffff;
    for (int i=0;i<DECISION_SCAR_CAP;i++){
        const DecisionScar *s=&ev->scars[i];
        if (s->subject!=(int16_t)subject || s->ripe_day<=today) continue;   /* case vide OU déjà mûre */
        if (s->ripe_day<best_day){ best_day=s->ripe_day; best=i; }
    }
    return best;
}
bool decision_memory_pending(const EventsState *ev, int subject, int today){
    if (!ev) return false;
    return decision_memory_find_pending(ev, subject, today) >= 0;
}
void decision_memory_cancel_next(EventsState *ev, int subject, int today){
    if (!ev) return;
    int i = decision_memory_find_pending(ev, subject, today);
    if (i<0) return;
    DecisionScar *s=&ev->scars[i];
    s->subject=-1; s->kind=0; s->ripe_day=0;   /* case vide — la dernière décision n'arrivera jamais */
}
void decision_memory_hasten(EventsState *ev, int subject, int today){
    if (!ev) return;
    int i = decision_memory_find_pending(ev, subject, today);
    if (i<0) return;
    DecisionScar *s=&ev->scars[i];
    /* rapproche le mûrissement sans le rendre déjà mûr (reste > today) : mi-chemin
     * entre aujourd'hui et l'échéance prévue, jamais moins que today+1. */
    int hastened = today + 1 + (s->ripe_day - today) / 2;
    if (hastened < s->ripe_day) s->ripe_day = hastened;
}
/* ===================================================================== */
/* LES ANNALES DU RÈGNE — anneau à SÉLECTION PAR POIDS (§ Annales)         */
/* ===================================================================== */
int annal_push(EventsState *ev, int year, int kind, int a, int b, int region,
              int weight, int option){
    if (!ev || kind<0 || kind>=ANNAL_KIND_COUNT) return -1;
    if (weight<0) weight=0; else if (weight>255) weight=255;
    AnnalEntry e;
    e.year=(int16_t)year; e.kind=(uint8_t)kind; e.a=(int16_t)a; e.b=(int16_t)b;
    e.region=(int16_t)region; e.weight=(uint8_t)weight; e.option=(int8_t)option; e.origin=-1;
    if (ev->annal_n < ANNALS_CAP){
        /* anneau pas encore plein : écriture round-robin normale */
        int idx=ev->annal_head;
        ev->annals[idx]=e;
        ev->annal_head=(ev->annal_head+1)%ANNALS_CAP;
        ev->annal_n++;
        return idx;
    }
    /* ANNEAU PLEIN — SÉLECTION PAR POIDS : on évince l'entrée de poids MINIMAL parmi
     * la MOITIÉ la plus ANCIENNE (les ANALALS_CAP/2 cases qui suivent immédiatement
     * la tête d'écriture — c'est là que vivent les entrées les plus vieilles d'un
     * anneau plein). Balayage d'index CROISSANT depuis annal_head → déterministe,
     * départage par le PREMIER trouvé (poids égal). Si le nouveau fait est plus
     * léger que TOUT ce qui est éligible à l'éviction, il ne remplace rien (le
     * panthéon des anciens/lourds gagne) — sauf le cas dégénéré poids 0 partout,
     * où le round-robin naturel (premier balayé) s'applique quand même. */
    int half = ANNALS_CAP/2;
    int worst_idx=-1; int worst_w=256;
    for (int k=0;k<half;k++){
        int idx=(ev->annal_head+k)%ANNALS_CAP;
        int w=ev->annals[idx].weight;
        if (w<worst_w){ worst_w=w; worst_idx=idx; }
    }
    if (worst_idx>=0 && (int)e.weight >= worst_w){
        ev->annals[worst_idx]=e;
        /* si l'éviction touche la case de tête, on avance la tête pour ne pas la
         * réévincer immédiatement au prochain push (round-robin cohérent). */
        if (worst_idx==ev->annal_head) ev->annal_head=(ev->annal_head+1)%ANNALS_CAP;
        return worst_idx;
    }
    return -1;   /* le nouveau fait est trop léger : rien n'est écrit */
}
int annals_count(const EventsState *ev){ return ev? ev->annal_n : 0; }
bool annals_at(const EventsState *ev, int i, AnnalEntry *out){
    if (!ev || !out || i<0 || i>=ev->annal_n) return false;
    *out = ev->annals[i];
    return true;
}
bool annals_save_sane(const EventsState *ev, int max_subject){
    if (!ev) return false;
    if (ev->annal_n < 0 || ev->annal_n > ANNALS_CAP) return false;
    if (ev->annal_head < 0 || ev->annal_head >= ANNALS_CAP) return false;
    for (int i=0;i<ev->annal_n;i++){
        const AnnalEntry *e=&ev->annals[i];
        if (e->kind >= ANNAL_KIND_COUNT) return false;   /* uint8_t : jamais < 0 */
        if (e->region < -1 || e->region >= max_subject) return false;
        if (e->origin < -1 || e->origin >= ANNALS_CAP) return false;
        if (e->option < -1 || e->option > 3) return false;   /* n_options ≤ 4 (0..3) */
    }
    return true;
}

void decision_cooldown_push(EventsState *ev, int evid, int subject, int until_day){
    if (!ev || evid<0 || evid>=EVID_COUNT) return;
    EvCooldown *c = &ev->cds[ev->cd_head];
    c->subject=(int16_t)subject; c->evid=(uint8_t)evid; c->until_day=until_day;
    ev->cd_head = (ev->cd_head+1) % DECISION_CD_CAP;
}
bool decision_cooldown_active(const EventsState *ev, int evid, int subject, int today){
    if (!ev) return false;
    for (int i=0;i<DECISION_CD_CAP;i++){
        const EvCooldown *c=&ev->cds[i];
        if (c->subject==(int16_t)subject && c->evid==(uint8_t)evid && c->until_day>today)
            return true;
    }
    return false;
}

/* ===================================================================== */
/* HOOKS DE CHOIX — faction (levier/concession) + cicatrice + cooldown     */
/* ===================================================================== */
/* CONTENU W2 (lot 2) §D : délai de MATURATION par ScarKind — le défaut [180,540] (le
 * legs W1/Marbrive, INCHANGÉ pour toute cicatrice non listée ici) reste le repli ; les
 * cicatrices neuves du chaînage (K2/K3/K5/K6/K7) obtiennent la fourchette DEMANDÉE pour
 * leur poseur (360-1460 j selon la nature de la crise — cf. la mission). Un simple
 * switch (≤10 lignes) : apply_choice_hook restait sinon UNE fourchette pour tous les
 * hooks, ce qui aurait forcé K2/K3/K5/K6/K7 sous [180,540] au lieu de leurs bornes
 * documentées. */
static void scar_delay_range(ScarKind k, int *lo, int *hi){
    switch (k){
        case SCAR_SCANDALE_SANITAIRE:   *lo=360; *hi=720;  break;   /* K2 */
        case SCAR_RADICALISATION:       *lo=360; *hi=900;  break;   /* K3 */
        case SCAR_PROLIFERATION_ARME:   *lo=720; *hi=1460; break;   /* K5 */
        case SCAR_FUITE_CERVEAUX:       *lo=540; *hi=1095; break;   /* K6 */
        case SCAR_EXEMPTION_ACHETEE:    *lo=360; *hi=900;  break;   /* K7 */
        default:                        *lo=180; *hi=540;  break;   /* legs W1/Marbrive, inchangé */
    }
}
/* `cid` = le pays PROPRIÉTAIRE du sujet (owner de la région / le pays lui-même) —
 * même convention que faction_lever_apply(a->cid,…) dans scps_ai.c. */
static void apply_choice_hook(EventCtx *cx, int evid, int subject, int cid,
                              const EvChoiceHook *h, int today){
    if (h->faction>=0 && h->faction<FAC_COUNT && cid>=0){
        if (h->faction_strength>0.f)
            faction_lever_apply(cid, (EthosFaction)h->faction, h->faction_strength);
        else
            faction_concede(cid, (EthosFaction)h->faction);
    }
    if (h->scar_kind>SCAR_NONE && h->scar_kind<SCAR_KIND_COUNT){
        int lo,hi; scar_delay_range((ScarKind)h->scar_kind, &lo, &hi);
        decision_memory_push(cx->ev, subject, (ScarKind)h->scar_kind, lo, hi);
    }
    if (h->cooldown_days>0)
        decision_cooldown_push(cx->ev, evid, subject, today + h->cooldown_days);
}

/* le pays CONCERNÉ par un évènement (owner de la région si provincial, le pays
 * lui-même sinon) — même calcul que le feed d'alertes ET les hooks de faction. */
static int event_owner_of(EventCtx *cx, EvScope scope, int subject){
    if (scope==EV_PROVINCE)
        return (cx->econ && subject>=0 && subject<cx->econ->n_regions)
            ? cx->econ->region[subject].owner : -1;
    return subject;
}

/* RÉSOUT un choix (option `oi` de l'évènement `evid` sur `subject`) : applique
 * l'effet + les hooks (faction/cicatrice/cooldown) du choix + le journal +
 * le fil d'alertes. CHEMIN COMMUN à l'auto-résolution IA (fire_event) et au
 * choix DRAINÉ du joueur (pending_event_resolve) — un seul acteur, deux voies
 * d'entrée. `today` = ev->ages.days_elapsed (pour les cicatrices/cooldowns). */
/* ANNALES — cherche, dans l'anneau, l'index de la plus RÉCENTE entrée ANNAL_DILEMME
 * qui a posé la cicatrice `evid_source` (Marbrive) sur `region` : c'est l'origine
 * CAUSALE d'un ANNAL_CICATRICE (Pont Effondré). -1 si introuvable (anneau tourné,
 * ou le dilemme d'origine n'a jamais été accroché — le joueur n'était pas le sujet). */
static int annal_find_dilemme_origin(const EventsState *ev, int evid_source, int region){
    int best=-1, best_year=-1;
    for (int i=0;i<ev->annal_n;i++){
        const AnnalEntry *e=&ev->annals[i];
        if (e->kind!=ANNAL_DILEMME || e->a!=(int16_t)evid_source || e->region!=(int16_t)region) continue;
        if (e->year>best_year){ best_year=e->year; best=i; }
    }
    return best;
}

static void resolve_choice(EventCtx *cx, int evid, int subject, int oi, int today){
    const EventDef *d=&EVENTS[evid];
    if (oi<0 || oi>=d->n_options) oi=0;
    const EvOption *opt=&d->options[oi];
    int cid = event_owner_of(cx, d->scope, subject);
    apply_effect(cx, d->scope, subject, &opt->eff);
    /* LE PARI (§ contenu W1) — APRÈS l'effet principal (CERTAIN), un tirage au rng
     * D'ÉVÈNEMENTS (déterministe) décide si le pari « tourne » : gamble_p=0 (défaut
     * des 15 évènements existants) ⇒ jamais de tirage, jamais d'effet — la table
     * existante est intacte par construction (comparaison stricte < sur un p nul
     * ne passe jamais, quel que soit frand). */
    if (opt->gamble_p > 0.f && frand(&cx->ev->rng) < opt->gamble_p){
        apply_effect(cx, d->scope, subject, &opt->gamble_eff);
        if (d->scope==EV_PROVINCE && subject>=0)
            provlog_push_event(subject, "Le pari a tourné.",
                               ev_sign(&opt->gamble_eff), ev_effdir(&opt->gamble_eff));
    }
    apply_choice_hook(cx, evid, subject, cid, &opt->hook, today);
    /* ESCLAVAGE (A1) — le choix « Abolir » de « Les chaînes rapportent » (option 2, la
     * vertu avant le profit) appelle le MÊME chemin d'affranchissement que le verbe
     * joueur CMD_MANUMIT : le canal le plus sobre (pas de nouvel EvEffect, un hook
     * d'effet) — cid = le pays propriétaire (event_owner_of, déjà résolu ci-dessus). */
    if (evid==EVID_CHAINES_RAPPORTENT && oi==2 && cid>=0 && cx->econ)
        demography_manumit_country(cx->econ, cid);
    /* PONT EFFONDRÉ CONSOMME la cicatrice qui l'a fait mûrir (une cicatrice = un tir —
     * sinon le trigger la relirait ENCORE mûre au prochain scan et re-déclencherait). */
    if (evid==EVID_PONT_EFFONDRE) decision_memory_consume(cx->ev, subject, SCAR_SABOTAGE_CHANTIER, today);
    /* CONTENU W2 (lot 2) §D : même motif — chaque évènement de chaînage CONSOMME la
     * cicatrice qui l'a fait mûrir (une cicatrice = un tir). */
    else if (evid==EVID_REMEDE_MORTS)      decision_memory_consume(cx->ev, subject, SCAR_SCANDALE_SANITAIRE, today);
    else if (evid==EVID_CELLULE_FAUBOURGS) decision_memory_consume(cx->ev, subject, SCAR_RADICALISATION,     today);
    else if (evid==EVID_FUSILS_REVIENNENT) decision_memory_consume(cx->ev, subject, SCAR_PROLIFERATION_ARME, today);
    else if (evid==EVID_SAVANTS_ENNEMI)    decision_memory_consume(cx->ev, subject, SCAR_FUITE_CERVEAUX,     today);
    else if (evid==EVID_TARIF_APPRIS)      decision_memory_consume(cx->ev, subject, SCAR_EXEMPTION_ACHETEE,  today);

    /* ═══ V2b LOT 1 — LA MERVEILLE : les mutations de eg (struct PLATE, écriture
     * directe sûre — même idiome qu'apply_region_eff sur RegionEconomy) ; le hook
     * de faction (lever/grief) est déjà posé par apply_choice_hook ci-dessus, ces
     * lignes ne posent QUE ce qu'aucune API publique ne fait (endgame_start_wonder
     * est idempotent — merv!=MERV_NONE ⇒ no-op — donc rappelable sans garde ici). */
    if (evid==EVID_MERV_FONDATION && cx->eg && cid>=0){
        int capr = world_capital_region(cx->w, cid);
        endgame_start_wonder(cx->eg, cid, capr);   /* idempotent : no-op si déjà en cours */
    }
    else if (evid==EVID_MERV_ASCENSION && cx->eg && cx->eg->merv_country==cid){
        if (oi==2)      cx->eg->merv = MERV_NONE;         /* Détruire : le monument brisé, la course s'arrête */
        /* Activer (oi==0) : rien à faire — MERV_SAVOIR_DONE laissé tel quel,
         * wonder_tick le fera basculer en MERV_ASCENDED puis endgame_select_and_fire
         * le lira au tick suivant (override MERVEILLE, déjà câblé, exempt du gate
         * temporel/entropie). Refuser (oi==1) : rien à faire non plus — le palier
         * reste MERV_SAVOIR_DONE, GELÉ (wonder_tick ne fait progresser un palier
         * _DONE QUE via ce trigger, qui ne re-tirera plus — cooldown 36500 j). */
    }

    /* ═══ V2b LOT 2 / P2 — LE CONSEIL : les actions qui MUTENT le Conseil (renvoi/
     * nomination/retraite) passent par les VERBES existants (statecraft_council_
     * dismiss/_hire), jamais un accès direct aux tableaux — même motif que
     * demography_manumit_country pour EVID_CHAINES_RAPPORTENT ci-dessus. SEULE
     * exception : la SUCCESSION (bypass explicite du grief de renvoi, cf. son bloc).
     *
     * P2 (docs/CONSEIL_ORIENTATIONS_2026-07-10.md) — LE HOOK DYNAMIQUE : chaque
     * branche calcule F = la faction RÉELLE du titulaire concerné (statecraft_
     * council_seat_faction, lue AVANT tout renvoi) et/ou Opp(F) = sa plus opposée
     * (faction_most_opposed, matrice de scps_factions), puis pousse le biais/la
     * rancœur/la concession voulue par la spec — jamais la faction hardcodée que
     * portait l'ancienne table statique (EVENTS[].options[].hook reste neutre,
     * faction=-1, pour tous ces choix : cf. les commentaires sur la table). Les
     * deltas que statecraft_council_dismiss (grief SA faction, COUNCIL_DISMISS_
     * GRIEF) et statecraft_council_hire (lève SA faction, COUNCIL_HIRE_LEVER)
     * appliquent DÉJÀ eux-mêmes ne sont jamais reproduits ici. */
    if (cx->sc && cx->w){
        uint32_t seed = cx->w->seed;

        if (evid==EVID_CONSEIL_SUCCESSION && cid>=0){
            /* Les DEUX options retirent IMMÉDIATEMENT le titulaire, SANS le grief de
             * renvoi (bypass COUNCIL_DISMISS_GRIEF — une retraite n'est pas une
             * disgrâce) : écriture DIRECTE des champs, miroir EXACT de statecraft_
             * council_dismiss MOINS l'appel à faction_grievance_add (même motif que
             * les écritures directes agitation/influence plus haut dans cette
             * fonction — Statecraft est un état SIM aux champs publics, ce module y
             * écrit déjà directement). Le siège retiré = celui désigné par le MÊME
             * balayage déterministe que le trigger. */
            int seat = conseil_succession_seat_find(cx, cid);
            if (seat>=0){
                int fac = statecraft_council_seat_faction(cx->sc, seed, cid, seat);
                cx->sc->council[cid][seat]     = -1;
                cx->sc->council_gen[cid][seat] = -1;
                cx->sc->loyalty[cid][seat]     = 0.f;
                cx->sc->pay[cid][seat]         = 1.f;
                if (oi==0 && fac>=0)   /* Le remercier publiquement : sa faction avance un peu */
                    faction_lever_apply(cid, (EthosFaction)fac, COUNCIL_HOOK_RETIRE_BIAS);
                /* oi==1 "Le laisser partir sans bruit" : rien de plus. */
            }
        }
        else if (evid==EVID_CONSEIL_R1 && cid>=0){
            if      (oi==0) council_lever_seat(cx, cid, 0, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour le Savoir  */
            else if (oi==1) council_lever_seat(cx, cid, 1, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour la Société */
            else if (oi==2) { statecraft_council_dismiss(cx->sc,seed,cid,0); statecraft_council_dismiss(cx->sc,seed,cid,1); }
        }
        else if (evid==EVID_CONSEIL_R2 && cid>=0){
            if      (oi==0) council_lever_seat(cx, cid, 2, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour l'Industrie */
            else if (oi==1) council_lever_seat(cx, cid, 1, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour la Société  */
            else if (oi==2) { statecraft_council_dismiss(cx->sc,seed,cid,2); statecraft_council_dismiss(cx->sc,seed,cid,1); }
        }
        else if (evid==EVID_CONSEIL_R3 && cid>=0){
            if      (oi==0) council_lever_seat(cx, cid, 0, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour le Savoir   */
            else if (oi==1) council_lever_seat(cx, cid, 2, COUNCIL_HOOK_OPPOSED_BIAS);  /* Trancher pour l'Industrie */
            else if (oi==2) { statecraft_council_dismiss(cx->sc,seed,cid,0); statecraft_council_dismiss(cx->sc,seed,cid,2); }
        }
        else if (evid==EVID_CONSEIL_A1 && cid>=0){
            /* Le trigger a déjà prouvé qu'une paire alliée existe — même balayage
             * déterministe (conseil_pair_find), re-scanné à la résolution. */
            int a=-1,b=-1;
            if (conseil_pair_find(cx, cid, COUNCIL_PAIR_ALLIANCE, &a, &b)){
                if (oi==0){                                     /* Laisser faire : +0,05 aux DEUX alliées */
                    council_lever_seat(cx, cid, a, COUNCIL_HOOK_ALLY_BIAS);
                    council_lever_seat(cx, cid, b, COUNCIL_HOOK_ALLY_BIAS);
                } else if (oi==1){                               /* Contrebalancer : +0,05 au 3e siège (vacant ⇒ rien) */
                    council_lever_seat(cx, cid, 3-a-b, COUNCIL_HOOK_ALLY_BIAS);
                } else if (oi==2){                               /* Séparer : renvoi du 2e (dismiss grief déjà SA faction) */
                    statecraft_council_dismiss(cx->sc,seed,cid,b);
                }
            }
        }
        else if (evid==EVID_CONSEIL_A2 && oi==0 && cid>=0){
            /* « Accepter leur candidat » NOMME RÉELLEMENT : parmi les candidats du
             * siège vacant (génération COURANTE), on choisit celui qui MINIMISE la
             * somme des oppositions aux deux factions alliées (départage : premier
             * minimum, balayage croissant des slots — déterministe), puis on
             * l'embauche pour de vrai. statecraft_council_hire lève DÉJÀ sa faction
             * (COUNCIL_HIRE_LEVER) : aucun hook de plus ici. */
            int a=-1,b=-1,third=-1;
            if (conseil_a2_find(cx, cid, &a, &b, &third)){
                int fa   = statecraft_council_seat_faction(cx->sc, seed, cid, a);
                int fb   = statecraft_council_seat_faction(cx->sc, seed, cid, b);
                int year = cx->ev->ages.days_elapsed/365;
                int gen  = statecraft_council_gen(year);
                int best_slot=0; float best_score=1e9f;
                for (int slot=0; slot<SC_COUNCIL_CANDS; slot++){
                    EthosFaction cf = statecraft_council_faction(seed, cid, third, slot, gen);
                    float score = 0.f;
                    if (fa>=0) score += faction_opposition(cf, (EthosFaction)fa);
                    if (fb>=0) score += faction_opposition(cf, (EthosFaction)fb);
                    if (score < best_score){ best_score=score; best_slot=slot; }
                }
                statecraft_council_hire(cx->sc, seed, cid, third, best_slot, gen);
            }
            /* oi==1 "Imposer son propre choix" : rien de plus (la liste normale).
             * oi==2 "Laisser le siège vacant" : rien. */
        }
        else if (evid==EVID_TRAHISON_SAVOIR && cid>=0){
            /* Les TROIS options renvoient (même « sans bruit » reste un renvoi,
             * juste discret — aucune ne GARDE le ministre) : F lu AVANT le renvoi. */
            int fac = statecraft_council_seat_faction(cx->sc, seed, cid, 0);
            statecraft_council_dismiss(cx->sc,seed,cid,0);            /* grief déjà SA faction (+0,10) */
            if ((oi==0 || oi==2) && fac>=0){                          /* Taire / Exemple public : Opp(F) +0,10 */
                EthosFaction opp = faction_most_opposed((EthosFaction)fac);
                if (opp>=0) faction_lever_apply(cid, opp, COUNCIL_HOOK_OPPOSED_BIAS);
            }
            /* oi==1 "Le renvoyer sans bruit" : renvoi seul, rien de plus. */
        }
        else if (evid==EVID_TRAHISON_SOCIETE && cid>=0){
            int fac = statecraft_council_seat_faction(cx->sc, seed, cid, 1);
            if (oi==0){                                               /* Purger les places : renvoi + Opp(F) */
                statecraft_council_dismiss(cx->sc,seed,cid,1);
                if (fac>=0){
                    EthosFaction opp = faction_most_opposed((EthosFaction)fac);
                    if (opp>=0) faction_lever_apply(cid, opp, COUNCIL_HOOK_OPPOSED_BIAS);
                }
            } else if (oi==1 && fac>=0){                              /* Composer : gardé, F +0,05 */
                faction_lever_apply(cid, (EthosFaction)fac, COUNCIL_HOOK_KEEP_BIAS);
            } else if (oi==2 && fac>=0){                              /* Laisser faire : gardé, CONCESSION à F */
                faction_concede(cid, (EthosFaction)fac);
            }
        }
        else if (evid==EVID_TRAHISON_INDUSTRIE && cid>=0){
            int fac = statecraft_council_seat_faction(cx->sc, seed, cid, 2);
            if (oi==0){                                               /* Le poursuivre : renvoi + Opp(F) */
                statecraft_council_dismiss(cx->sc,seed,cid,2);
                if (fac>=0){
                    EthosFaction opp = faction_most_opposed((EthosFaction)fac);
                    if (opp>=0) faction_lever_apply(cid, opp, COUNCIL_HOOK_OPPOSED_BIAS);
                }
            } else if (oi==1 && fac>=0){                              /* Négocier un remboursement : gardé, F +0,05 */
                faction_lever_apply(cid, (EthosFaction)fac, COUNCIL_HOOK_KEEP_BIAS);
            } else if (oi==2 && fac>=0){                              /* Fermer les yeux : gardé, CONCESSION à F */
                faction_concede(cid, (EthosFaction)fac);
            }
        }
        else if (hero_seat_of(evid)>=0 && cid>=0){
            /* raccord 7 — « Le nom du siècle ». Le siège est FIXE par EVID (mirroir des
             * TRAHISON_* ci-dessus) ; l'identité (slot,gen) est RE-DÉRIVÉE ici (le fait est
             * frais — même tick que ages_hero_fire). oi==0/1 posent un BONUS sur la
             * PROCHAINE mission du siège (cx->ms->hero_bonus, consommé par
             * mission_grant/scps_missions.c — successeur exclu si le titulaire a changé
             * d'ici là) ; oi==1 CAPTURE la faction (faction_concede — la Corruption qui
             * en découle EST l'effet, jamais un delta codé à côté) ; oi==2 aigrit SA
             * PROPRE faction (refusé, pas « la plus opposée » — motif RENVOYER de P1-3). */
            int seat = hero_seat_of(evid);
            int fac  = statecraft_council_seat_faction(cx->sc, seed, cid, seat);
            if (oi==0 || oi==1){
                if (cx->ms && cx->sc){
                    int slot = statecraft_council_seated(cx->sc, cid, seat);
                    if (slot>=0){
                        int gen = statecraft_council_seated_gen(cx->sc, cid, seat);
                        HeroMissionBonus *hb = &cx->ms->hero_bonus[cid][seat];
                        hb->mult = (oi==0) ? tune_f("AGE_HERO_MISSION_REWARD",1.20f)
                                           : tune_f("AGE_HERO_MISSION_REWARD_CAPTURED",1.30f);
                        hb->slot = (int8_t)slot; hb->gen = (int8_t)gen;
                    }
                }
                if (oi==0 && fac>=0) faction_lever_apply(cid, (EthosFaction)fac, tune_f("AGE_HERO_FACTION_LEVER",0.08f));
                else if (oi==1 && fac>=0) faction_concede(cid, (EthosFaction)fac);   /* Corruption : la capture EST l'effet */
            } else if (fac>=0){
                faction_grievance_add(cid, (EthosFaction)fac, tune_f("AGE_HERO_REFUSED_GRIEF",0.08f));
            }
        }
        else if (evid==EVID_CONSEIL_C1 && cid>=0){
            /* La conspiration porte sur une paire ALIÉNÉE (COUNCIL_PAIR_CONSPIRATION) —
             * « Les renvoyer les deux »/« En sacrifier un » n'ont RIEN à ajouter :
             * dismiss() grief DÉJÀ (+0,10) la faction propre de chaque renvoyé, SANS
             * capture (aucun appel à faction_concede) — exactement la rancœur voulue.
             * « Céder » (le SEUL choix Conseil qui abandonne une part durable de
             * l'État) ne renvoie personne : DOUBLE concession (+0,045 capture chacune
             * = +9 Corr, +0,06 biais chacune) — impossible via le hook générique à
             * UNE faction, donc résolu ici. */
            int a=-1,b=-1;
            if (conseil_pair_find(cx, cid, COUNCIL_PAIR_CONSPIRATION, &a, &b)){
                if (oi==0){ statecraft_council_dismiss(cx->sc,seed,cid,a); statecraft_council_dismiss(cx->sc,seed,cid,b); }
                else if (oi==1) statecraft_council_dismiss(cx->sc,seed,cid,a);
                else if (oi==2){
                    int fa = statecraft_council_seat_faction(cx->sc, seed, cid, a);
                    int fb = statecraft_council_seat_faction(cx->sc, seed, cid, b);
                    if (fa>=0) faction_concede(cid, (EthosFaction)fa);
                    if (fb>=0) faction_concede(cid, (EthosFaction)fb);
                }
            }
        }
    }

    if (evid==EVID_MARBRIVE) g_marbrive_fired++; else if (evid==EVID_PONT_EFFONDRE) g_pont_effondre_fired++;
    else if (evid==EVID_CLOCHES) g_cloches_fired++;
    else if (evid==EVID_ENTREPOTS_FERMES) g_entrepots_fermes_fired++;
    else if (evid==EVID_DEUX_CARTES) g_deux_cartes_fired++;
    else if (evid==EVID_EAU_NOIRE) g_eau_noire_fired++;
    else if (evid==EVID_DERNIERE_DECISION) g_derniere_decision_fired++;
    else if (evid==EVID_SALVE_RUNIQUE) g_salve_runique_fired++;
    else if (evid==EVID_CHAINES_RAPPORTENT) g_chaines_rapportent_fired++;
    else if (evid==EVID_OEUVRE_NOIRE) g_oeuvre_noire_fired++;
    else if (evid==EVID_SAVOIR_INTERDIT) g_savoir_interdit_fired++;
    else if (evid==EVID_CULTE_IMPERIAL) g_culte_imperial_fired++;
    else if (evid==EVID_EVEIL) g_eveil_fired++;
    else if (evid==EVID_FOREUSE_SAIGNE) g_foreuse_saigne_fired++;
    else if (evid==EVID_DROIT_INTEGRATION) g_droit_integration_fired++;
    else if (evid==EVID_DIASPORA_COMPTOIRS) g_diaspora_comptoirs_fired++;
    else if (evid==EVID_FOI_FENDRE) g_foi_fendre_fired++;
    else if (evid==EVID_PROPHETE_BRECHE) g_prophete_breche_fired++;
    else if (evid==EVID_RELIQUE_DOUTEUSE) g_relique_douteuse_fired++;
    else if (evid==EVID_REMEDE_MORTS) g_remede_morts_fired++;
    else if (evid==EVID_CELLULE_FAUBOURGS) g_cellule_faubourgs_fired++;
    else if (evid==EVID_FUSILS_REVIENNENT) g_fusils_reviennent_fired++;
    else if (evid==EVID_SAVANTS_ENNEMI) g_savants_ennemi_fired++;
    else if (evid==EVID_TARIF_APPRIS) g_tarif_appris_fired++;
    else if (evid==EVID_MERV_FONDATION) g_merv_fondation_fired++;
    else if (evid==EVID_MERV_SACRIFICE) g_merv_sacrifice_fired++;
    else if (evid==EVID_MERV_ASCENSION) g_merv_ascension_fired++;
    else if (evid==EVID_TRAHISON_SAVOIR) g_trahison_savoir_fired++;
    else if (evid==EVID_TRAHISON_SOCIETE) g_trahison_societe_fired++;
    else if (evid==EVID_TRAHISON_INDUSTRIE) g_trahison_industrie_fired++;
    else if (evid==EVID_CONSEIL_SUCCESSION) g_conseil_succession_fired++;
    else if (evid==EVID_CONSEIL_R1) g_conseil_r1_fired++;
    else if (evid==EVID_CONSEIL_R2) g_conseil_r2_fired++;
    else if (evid==EVID_CONSEIL_R3) g_conseil_r3_fired++;
    else if (evid==EVID_CONSEIL_A1) g_conseil_a1_fired++;
    else if (evid==EVID_CONSEIL_A2) g_conseil_a2_fired++;
    else if (evid==EVID_CONSEIL_C1) g_conseil_c1_fired++;
    else if (evid==EVID_RIVAUX_VOISINS) g_rivaux_voisins_fired++;
    else if (evid==EVID_PARENTE_LOINTAINE) g_parente_lointaine_fired++;
    else if (evid==EVID_MARCHE_ETHOS) g_marche_ethos_fired++;
    else if (evid==EVID_TOLERANCE_CREDO) g_tolerance_credo_fired++;
    else if (evid==EVID_LETTRE_PERIME) g_lettre_perime_fired++;
    else if (evid==EVID_PRATIQUE_DERIVE) g_pratique_derive_fired++;
    cx->ev->last_id=evid; cx->ev->last_name=d->name; cx->ev->n_fired++;
    if (d->scope==EV_PROVINCE && subject>=0)
        provlog_push_event(subject, event_title_ring(cx->w, evid, subject),   /* nom RÉEL du lieu */
                           ev_sign(&opt->eff), ev_effdir(&opt->eff));
    /* FIL D'ÉVÈNEMENTS (alertes/popup du front) : write-only, jamais relu → déterminisme
     * intact. Le focus du fil (feed_set_focus) filtre à l'entrée, le front re-filtre en ceinture. */
    feed_push(FEED_DIRECTOR, cid, -1, (d->scope==EV_PROVINCE) ? subject : -1, evid);
    /* LES ANNALES DU RÈGNE — n'accrochent QUE le pays JOUEUR (le récit DU joueur) ; en
     * chronique (human_player=-1) rien ne s'accroche jamais → golden intact PAR CONSTRUCTION. */
    if (cx->human_player>=0 && cid==cx->human_player){
        int year = cx->ev->ages.days_elapsed/365;
        float w = 30.f + 40.f*director_amplitude(cx->ev);   /* poids ∝ amplitude dramatique du moment */
        if (evid==EVID_PONT_EFFONDRE){
            /* ANNAL_CICATRICE : la cicatrice (posée par « Envoyer les prévôts » sur Marbrive)
             * a MÛRI en cette catastrophe — on retrouve l'entrée d'origine si elle est
             * ENCORE dans l'anneau (elle a pu tourner entretemps : origin=-1 alors). */
            int origin = annal_find_dilemme_origin(cx->ev, EVID_MARBRIVE, subject);
            int wi = annal_push(cx->ev, year, ANNAL_CICATRICE, evid, oi, subject, (int)w+20, oi);
            if (wi>=0 && origin>=0) cx->ev->annals[wi].origin=(int16_t)origin;
        } else if (evid==EVID_TRAHISON_SAVOIR || evid==EVID_TRAHISON_SOCIETE || evid==EVID_TRAHISON_INDUSTRIE){
            /* ANNAL_TRAHISON — un grand moment du Conseil : region porte le SIÈGE
             * concerné (0/1/2, pas une région géographique — la charte membrane
             * documente déjà region comme "sujet", ici un index de siège). */
            int seat = (evid==EVID_TRAHISON_SAVOIR)?0:(evid==EVID_TRAHISON_SOCIETE)?1:2;
            annal_push(cx->ev, year, ANNAL_TRAHISON, evid, 0, seat, (int)w+15, oi);
        } else if (evid==EVID_MERV_FONDATION || evid==EVID_MERV_ASCENSION){
            /* ANNAL_MERVEILLE_ETAPE — la Merveille en 3 étapes : poids PLEIN, le
             * moment le plus lourd de la partie (motif ANNAL_FIN/ANNAL_AGE, ex.). */
            annal_push(cx->ev, year, ANNAL_MERVEILLE_ETAPE, evid, 0, subject, 90, oi);
        } else if (d->n_options>1){
            /* ANNAL_DILEMME : un VRAI choix (n_options>1) résolu par le joueur — les chocs
             * géo/floraisons à option unique n'ont rien à raconter (pas un « choix »). */
            annal_push(cx->ev, year, ANNAL_DILEMME, evid, 0, subject, (int)w, oi);
        }
    }
}

/* true si un pending (evid,subject) est DÉJÀ en attente — anti-doublon : un trigger
 * dont l'état reste vrai plusieurs jours d'affilée (ex. Marbrive tant que l'agitation
 * ne redescend pas) ne doit pas empiler la même décision plusieurs fois avant que
 * le joueur n'ait répondu à la première. */
static bool pending_event_has(const EventsState *ev, int evid, int subject){
    for (int i=0;i<ev->pending_n;i++)
        if (ev->pending[i].evid==(uint8_t)evid && ev->pending[i].subject==(int16_t)subject) return true;
    return false;
}

/* Choix de l'option par l'IA (poids ai_chance) puis application + journal —
 * SAUF si le sujet est le JOUEUR et l'évènement porte une VRAIE décision
 * (n_options>1) : la membrane l'ENFILE alors pour son choix (§4) au lieu de
 * trancher à sa place. Les évènements à option UNIQUE (chocs géo, floraisons)
 * restent auto-résolus même pour le joueur — rien à choisir, pas de décision. */
/* PLAFOND DE TIRS À VIE (demande joueur : « 3-5 triggers PAR évènement ») — chaque
 * dilemme n'arrive que 3-5 fois dans TOUTE LA PARTIE, monde entier (la rareté fait
 * l'évènement). Les évènements PLAFONNÉS = les dilemmes narratifs (crise phare, W1,
 * §A latches tech, §B culturel, §C religieux) ; NON plafonnés (0) : les chocs géo/
 * floraisons (leur anti-acharnement propre) et le chaînage §D — une cicatrice mûrie
 * DOIT revenir, sinon les choix perdent leur sens (§D reste borné par ses cicatrices
 * = par les CHOIX). Le plafond effectif (3-5) est tiré à events_init d'un hash
 * graine×evid — déterministe, sérialisé (fire_cap). */
static const uint8_t EV_CAPPED[EVID_COUNT] = {
    [EVID_MARBRIVE]=1,          [EVID_CLOCHES]=1,        [EVID_ENTREPOTS_FERMES]=1,
    [EVID_DEUX_CARTES]=1,       [EVID_EAU_NOIRE]=1,      [EVID_DERNIERE_DECISION]=1,
    [EVID_DROIT_INTEGRATION]=1, [EVID_DIASPORA_COMPTOIRS]=1,
    [EVID_FOI_FENDRE]=1,        [EVID_PROPHETE_BRECHE]=1, [EVID_RELIQUE_DOUTEUSE]=1,
    [EVID_SALVE_RUNIQUE]=1,     [EVID_CHAINES_RAPPORTENT]=1, [EVID_OEUVRE_NOIRE]=1,
    [EVID_SAVOIR_INTERDIT]=1,   [EVID_CULTE_IMPERIAL]=1,  [EVID_EVEIL]=1,
    [EVID_FOREUSE_SAIGNE]=1,
};
static void events_fire_caps_seed(EventsState *ev, uint32_t seed){
    for (int e=0;e<EVID_COUNT;e++)
        ev->fire_cap[e] = EV_CAPPED[e]
            ? (uint8_t)(3u + ((seed*2654435761u ^ (unsigned)e*2246822519u) % 3u))
            : 0u;
}
int events_fire_cap(const EventsState *ev, int evid){
    return (ev && evid>=0 && evid<EVID_COUNT) ? ev->fire_cap[evid] : 0;
}
int events_fire_count(const EventsState *ev, int evid){
    return (ev && evid>=0 && evid<EVID_COUNT) ? ev->fires[evid] : 0;
}
static void ev_fire_note(EventsState *ev, int evid){
    if (ev->fires[evid]<255) ev->fires[evid]++;
}

static void fire_event(EventCtx *cx, int evid, int subject){
    const EventDef *d=&EVENTS[evid];
    /* RÉPIT (hook cooldown_days d'un choix précédent) : cet évènement ne retire pas
     * sur ce sujet tant qu'il n'a pas expiré — même acteur pour l'IA et le joueur. */
    if (decision_cooldown_active(cx->ev, evid, subject, cx->ev->ages.days_elapsed)) return;
    /* PLAFOND DE TIRS À VIE : le monde a déjà vécu cet évènement son content. */
    if (cx->ev->fire_cap[evid]>0 && cx->ev->fires[evid] >= cx->ev->fire_cap[evid]) return;
    if (d->n_options>1 && cx->human_player>=0){
        int owner = event_owner_of(cx, d->scope, subject);
        if (owner==cx->human_player){
            if (!pending_event_has(cx->ev, evid, subject) &&
                pending_event_push(cx->ev, evid, subject, cx->ev->ages.days_elapsed))
                ev_fire_note(cx->ev, evid);   /* compté à l'ENFILAGE (l'évènement a eu lieu) */
            return;
        }
    }
    int best=0; float bw=-1.f;
    for (int i=0;i<d->n_options;i++) if (d->options[i].ai_chance>bw){ bw=d->options[i].ai_chance; best=i; }
    resolve_choice(cx, evid, subject, best, cx->ev->ages.days_elapsed);
    ev_fire_note(cx->ev, evid);
}

/* ===================================================================== */
/* LA FILE JOUEUR                                                          */
/* ===================================================================== */
bool pending_event_push(EventsState *ev, int evid, int subject, int today){
    if (!ev || evid<0 || evid>=EVID_COUNT) return false;
    if (ev->pending_n >= PENDING_EVENT_CAP) return false;   /* file pleine : l'évènement se perd (silencieux) */
    PendingEvent *p = &ev->pending[ev->pending_n++];
    p->evid=(uint8_t)evid; p->subject=(int16_t)subject;
    p->fire_day=today; p->expire_day=today+180;
    return true;
}
int pending_event_count(const EventsState *ev){ return ev? ev->pending_n : 0; }
bool pending_event_at(const EventsState *ev, int slot, PendingEvent *out){
    if (!ev || !out || slot<0 || slot>=ev->pending_n) return false;
    *out = ev->pending[slot];
    return true;
}
/* retire le pending au slot `slot` (swap avec le dernier — l'ordre n'est pas garanti,
 * comme purge_slice/l'idiome des files bornées du dépôt). */
static void pending_event_remove(EventsState *ev, int slot){
    if (!ev || slot<0 || slot>=ev->pending_n) return;
    ev->pending[slot] = ev->pending[ev->pending_n-1];
    ev->pending_n--;
}
bool pending_event_resolve(EventsState *ev, World *w, WorldEconomy *econ,
                           WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                           RouteNetwork *rn, const TechState ts[], DiploState *dp,
                           EndgameState *eg,
                           int slot, int option, int today, int human_player){
    if (!ev || slot<0 || slot>=ev->pending_n) return false;
    PendingEvent p = ev->pending[slot];
    if (p.evid>=EVID_COUNT) { pending_event_remove(ev,slot); return false; }
    const EventDef *d=&EVENTS[p.evid];
    if (option<0 || option>=d->n_options) return false;
    /* human_player n'INFLUENCE PAS la résolution elle-même (déjà tranchée, ce choix ne
     * peut plus ré-enfiler) — il sert UNIQUEMENT à faire accrocher LES ANNALES quand
     * c'est bien le joueur qui a choisi (fire_event du tick régulier reste la seule
     * voie d'ENFILAGE ; ceci n'est que le point de sortie du choix DÉJÀ posé). */
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,human_player,NULL};
    resolve_choice(&cx, p.evid, p.subject, option, today);
    pending_event_remove(ev, slot);
    return true;
}
void pending_event_tick_expire(EventsState *ev, World *w, WorldEconomy *econ,
                               WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                               RouteNetwork *rn, const TechState ts[], DiploState *dp,
                               EndgameState *eg,
                               int today){
    if (!ev) return;
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,-1,NULL};
    /* parcours à REBOURS : pending_event_remove swap avec le dernier — reculer évite
     * de sauter un slot fraîchement déplacé dans la case qu'on vient de traiter. */
    for (int i=ev->pending_n-1; i>=0; i--){
        PendingEvent p = ev->pending[i];
        if (p.expire_day > today) continue;
        if (p.evid<EVID_COUNT){
            const EventDef *d=&EVENTS[p.evid];
            int best=0; float bw=-1.f;
            for (int k=0;k<d->n_options;k++) if (d->options[k].ai_chance>bw){ bw=d->options[k].ai_chance; best=k; }
            resolve_choice(&cx, p.evid, p.subject, best, today);
        }
        pending_event_remove(ev, i);
    }
}

/* NOM d'un évènement de la table (le fil le porte par ID ; la façade résout le MOT). */
const char *events_name_of(int evid){
    return (evid>=0 && evid<EVID_COUNT) ? EVENTS[evid].name : "";
}

/* ===================================================================== */
/* events_strike — applique un choc géo précis (déterministe)            */
/* ===================================================================== */
void events_strike(EventsState *ev, World *w, WorldEconomy *econ,
                   WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                   int region, EvId shock){
    EventCtx cx={ev,w,econ,wl,wp,sc,NULL,NULL,NULL,NULL,-1,NULL};
    if (shock<0||shock>=EVID_COUNT) return;
    apply_effect(&cx, EVENTS[shock].scope, region, &EVENTS[shock].options[0].eff);
    ev->last_id=shock; ev->last_name=EVENTS[shock].name; ev->n_fired++;
    if (EVENTS[shock].scope==EV_PROVINCE && region>=0)
        provlog_push_event(region, EVENTS[shock].name, ev_sign(&EVENTS[shock].options[0].eff), ev_effdir(&EVENTS[shock].options[0].eff));
    {   /* fil display : a = owner de la région frappée (le focus filtre à l'entrée) */
        int own = (econ && region>=0 && region<econ->n_regions) ? econ->region[region].owner : -1;
        feed_push(FEED_DIRECTOR, own, -1, region, (int)shock);
    }
}

/* ===================================================================== */
/* PESTE — diffusion BFS le long des ROUTES ouvertes                     */
/* ===================================================================== */
int events_plague_spread(EventsState *ev, World *w, WorldEconomy *econ,
                         WorldLegitimacy *wl, Statecraft *sc, RouteNetwork *rn,
                         int seed_region){
    if (!rn || seed_region<0 || seed_region>=econ->n_regions) return 0;
    int hop[SCPS_MAX_REG]; for (int i=0;i<SCPS_MAX_REG;i++) hop[i]=-1;
    int queue[SCPS_MAX_REG], qh=0, qt=0;
    hop[seed_region]=0; queue[qt++]=seed_region;
    int infected=0;
    while (qh<qt){
        int r=queue[qh++];
        int h=hop[r];
        /* sévérité décroît avec la distance le long des routes */
        float mult = (h==0)?0.78f:(h==1)?0.85f:(h==2)?0.90f:0.95f;
        EvEffect e = EVENTS[EVID_PLAGUE].options[0].eff;
        e.pop_mult = mult;
        e.d_agitation = 18.f - 3.f*h;
        EventCtx cx={ev,w,econ,wl,NULL,sc,rn,NULL,NULL,NULL,-1,NULL};
        apply_effect(&cx, EV_PROVINCE, r, &e);
        provlog_push_event(r, EVENTS[EVID_PLAGUE].name, +1, ev_effdir(&e));   /* journal provincial : la peste */
        infected++;
        if (h>=4) continue;                         /* portée bornée */
        for (int i=0;i<rn->n;i++){
            const TradeRoute *t=&rn->route[i];
            if (!t->open) continue;
            int other=-1;
            if (t->ra==r) other=t->rb; else if (t->rb==r) other=t->ra;
            if (other<0||other>=SCPS_MAX_REG) continue;
            if (hop[other]<0){ hop[other]=h+1; queue[qt++]=other; }
        }
    }
    ev->last_id=EVID_PLAGUE; ev->last_name=EVENTS[EVID_PLAGUE].name;
    return infected;
}

/* ===================================================================== */
/* events_match_political — la VARIANTE par la fiche (priorité éthos)     */
/* ===================================================================== */
int events_match_political(const EventsState *ev, World *w, WorldEconomy *econ,
                           WorldLegitimacy *wl, Statecraft *sc, int region){
    EventCtx cx={ (EventsState*)ev, w, econ, wl, NULL, sc, NULL, NULL, NULL, NULL, -1, NULL };
    static const int order[4]={ EVID_INTEG_DOMINATEUR, EVID_INTEG_MERCANTILE,
                                EVID_INTEG_BUREAUCRATE, EVID_INTEG_ANCIEN };
    for (int i=0;i<4;i++){
        const EventDef *d=&EVENTS[order[i]];
        if (d->trigger && d->trigger(&cx, region)) return order[i];
    }
    return -1;
}

/* ===================================================================== */
/* ÂGES SANS ORDRE IMPOSÉ (2026-07-11, docs/AGES_FINS_2026-07-11.md)      */
/* ===================================================================== */
/* Verdict du moteur, mode « révolution » = SCPS_SUBMERGE_REVOLUTION (miroir, on
 * n'inclut PAS scps_core : on lit l'int déjà stocké dans CountryProsperity). */
enum { EV_MODE_CONSENTI=0, EV_MODE_COERC_FRAGILE, EV_MODE_REVOLUTION, EV_MODE_SECESSION };

static const char *AGE_NAMES[AGE_COUNT]={
    "L'Ère des Échanges","L'Âge des Découvertes","L'Âge des Empires","L'Âge des Héros",
    "L'Âge de la Brèche","L'Âge des Lumières","L'Âge des Soulèvements","L'Ère des Tyrans"
};
const char *age_name(AgeId a){ return (a>=0&&a<AGE_COUNT)?AGE_NAMES[a]:"?"; }

#define AUBE_CITATION "Il n'y a rien de nouveau sous le soleil. (Ecclésiaste 1:9)"
static const char *AGE_CITATIONS[AGE_COUNT]={
    "L'effet naturel du commerce est de porter à la paix. (Montesquieu)",
    "Si j'ai vu plus loin, c'est en me tenant sur les épaules de géants. (Newton)",
    "Ils font un désert et appellent cela la paix. (Tacite)",
    "L'histoire universelle est, au fond, l'histoire des grands hommes. (Carlyle)",
    "La science de l'homme est la mesure de sa puissance. (Bacon)",
    "Aie le courage de te servir de ton propre entendement. (Kant)",
    "De l'audace, encore de l'audace… (Danton)",
    "Le pouvoir tend à corrompre… (Lord Acton)",
};
const char *age_citation(int age){
    if (age<0) return AUBE_CITATION;
    return (age<AGE_COUNT) ? AGE_CITATIONS[age] : "";
}

/* ---- Lecteurs de l'état AGRÉGÉ du monde (sur les pays non-vierges) ------ */
static int nc_loop(const World *w, const WorldProsperity *wp){
    return (w->n_countries < wp->n_countries) ? w->n_countries : wp->n_countries;
}
static int w_living_count(const World *w, const WorldProsperity *wp){
    int n=0;
    for (int c=0;c<nc_loop(w,wp);c++) if (w->country[c].role!=POLITY_UNCLAIMED) n++;
    return n;
}
static float w_mean_savoir(const World *w, const WorldProsperity *wp){
    float s=0.f; int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ s+=wp->country[c].Lumiere; n++; }
    return n? s/(float)n : 0.f;
}
static float w_mean_C(const World *w, const WorldProsperity *wp){
    float s=0.f; int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ s+=wp->country[c].C; n++; }
    return n? s/(float)n : 0.f;
}
static float w_mean_fracture(const World *w, const WorldProsperity *wp){
    float s=0.f; int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ s+=wp->country[c].fracture; n++; }
    return n? s/(float)n : 0.f;
}
static float w_mean_dereal(const World *w, const WorldProsperity *wp){
    float s=0.f; int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ s+=wp->country[c].dereal; n++; }
    return n? s/(float)n : 0.f;
}
static float w_mean_SI(const World *w, const WorldProsperity *wp){
    float s=0.f; int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ s+=wp->country[c].SI; n++; }
    return n? s/(float)n : 10.f;
}
int events_count_revolutionary(const World *w, const WorldProsperity *wp){
    int n=0;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED && wp->country[c].mode==EV_MODE_REVOLUTION) n++;
    return n;
}
/* raccord 6 — le ratio de pays connus (façade + déclencheur des Découvertes) : un
 * simple relais de country_known_pair_share (scps_fog.c), qui NE dépend PAS des
 * Âges (fog n'a jamais besoin de connaître AgeId). */
float ages_known_pair_share(const World *w){ return country_known_pair_share(w); }
int ages_fog_radius_add(const EventsState *ev){
    return (ev && ages_dawned(ev,AGE_DISCOVERY)) ? (int)tune_f("AGE_DISCOVERY_FOG_RADIUS_ADD",1.0f) : 0;
}

/* ---- DÉCLENCHEURS MATÉRIELS (spec verbatim, docs/AGES_FINS_2026-07-11.md) ---- */
static bool age_trig_exchange(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)w;(void)wp;(void)ts;
    int rich=0, habited=0;
    for (int r=0;r<econ->n_regions;r++){
        if (econ->region[r].owner<0) continue;
        habited++;
        if (econ->region[r].route_pe > tune_f("AGE_EXCHANGE_NODE_VALUE",1.0f)) rich++;
    }
    if (rich < (int)tune_f("AGE_EXCHANGE_NODE_MIN",4.0f)) return false;
    return habited>0 && (float)rich/(float)habited >= tune_f("AGE_EXCHANGE_NODE_SHARE",0.08f);
}
static bool age_trig_discovery(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)econ;(void)wp;(void)ts;
    if (w_living_count(w,wp) < (int)tune_f("AGE_DISCOVERY_COUNTRY_MIN",6.0f)) return false;
    return ages_known_pair_share(w) >= tune_f("AGE_DISCOVERY_KNOWN_PAIR_SHARE",0.12f);
}
static bool age_trig_empires(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[],
                             WorldLegitimacy *wl){
    (void)wp;(void)ts; if(!wl) return false;
    int world_n=0; int per_country[SCPS_MAX_COUNTRY]={0};
    float held_min = tune_f("AGE_EMPIRES_HELD_YEARS",35.0f);
    for (int r=0;r<econ->n_regions;r++){
        int o=econ->region[r].owner;
        if (o<0 || o>=SCPS_MAX_COUNTRY || !econ->region[r].culture.settled) continue;
        if (r>=SCPS_MAX_REG || wl->years_held[r] <= held_min) continue;
        world_n++; per_country[o]++;
    }
    if (world_n < (int)tune_f("AGE_EMPIRES_REGIONS_WORLD",8.0f)) return false;
    int need_one = (int)tune_f("AGE_EMPIRES_REGIONS_ONE_COUNTRY",4.0f);
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++) if (per_country[c] >= need_one) return true;
    return false;
}
static bool age_trig_breach(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)w;(void)econ;(void)wp; if(!ts) return false;
    float charge_min = tune_f("AGE_BREACH_CHARGE",6.0f);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) if (ts[c].charge > charge_min) return true;
    return false;
}
static bool age_trig_lumieres(World *w, WorldProsperity *wp){
    if (!wp) return false;
    return w_mean_savoir(w,wp) > tune_f("AGE_LUMIERES_SAVOIR_MEAN",5.0f)
        && w_mean_C(w,wp)      > tune_f("AGE_LUMIERES_C_MEAN",4.5f);
}
/* Soulèvements ↔ Tyrans : paire MUTUELLEMENT EXCLUSIVE (qui advient ferme l'autre
 * pour toujours) — plus de précondition Lumières (aucun ordre imposé). Ces deux
 * triggers restent des lectures MATÉRIELLES PURES (pas de précondition dessus) :
 * l'exclusion elle-même est un GATE séparé, revérifié à CHAQUE tentative
 * d'avènement (pas seulement au moment de l'ÉLIGIBILITÉ) — sinon les deux
 * s'ACQUIÈRENT tous les deux au même instant (un monde qui bascule dans la
 * crise satisfait souvent les DEUX conditions à la fois), et l'éligibilité
 * ACQUISE de spec (« reste acquise même si la valeur redescend ») ferait
 * quand même avenir le second après coup. */
static bool age_trig_soulevements(const EventsState *ev, World *w, WorldProsperity *wp){
    (void)ev; if (!wp) return false;
    return events_count_revolutionary(w,wp) >= (int)tune_f("AGE_SOULEVEMENTS_MIN_COUNTRIES",2.0f);
}
static bool age_trig_tyrans(const EventsState *ev, World *w, WorldProsperity *wp){
    (void)ev; if (!wp) return false;
    return w_mean_fracture(w,wp) > tune_f("AGE_TYRANS_FRACTURE",3.0f)
        && w_mean_dereal(w,wp)   > tune_f("AGE_TYRANS_DEREAL",1.25f)
        && w_mean_SI(w,wp)       < tune_f("AGE_TYRANS_SI",5.0f);
}

/* ---- JITTER SANS CHRONOLOGIE CACHÉE (hash pur, aucun état de rng partagé) ---- */
static uint32_t age_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d){
    uint32_t h = a*2654435761u ^ b*40503u ^ c*2246822519u ^ d*3266489917u;
    h ^= h>>16; h *= 2246822519u; h ^= h>>13; h *= 3266489917u; h ^= h>>16;
    return h;
}
/* année d'avènement = éligible + hash(seed,âge,éligible) % (JITTER+1) — 0..4 ans
 * d'attente déterministe, sans rien de plus à sérialiser (dérivée à chaque tick). */
static int age_dawn_year(uint32_t seed, AgeId a, int eligible){
    int jitter = (int)tune_f("AGE_TRIGGER_JITTER_YEARS",4.0f);
    if (jitter<0) jitter=0;
    uint32_t h = age_hash(seed^0xA6E1F00Du, (uint32_t)a, (uint32_t)eligible, 0x0DE1F1E5u);
    return eligible + (int)(h % (uint32_t)(jitter+1));
}
/* Ordre de priorité (« un hash de la seed choisit le premier ») entre deux âges
 * candidats la MÊME année : rang STABLE (indépendant de l'année), pas un second tirage. */
static uint32_t age_priority(uint32_t seed, AgeId a){
    return age_hash(seed^0xC011DE01u, (uint32_t)a, 0, 0);
}

/* ⚠ SUPPRIMÉ (raccord Tyrans, spec verbatim) : « AUCUNE conversion automatique des
 * credos » — l'Ère des Tyrans faisait auparavant muter le credo de la capitale de
 * CHAQUE pays vers CREDO_PURIFICATEUR (spread_credo_purificateur). Retiré sans
 * remplacement (cf. TROUVAILLES.md). */

/* ---- LEVIERS DE FACTION SCOPÉS (« que dans les pays MATÉRIELLEMENT concernés ») */
static void age_lever_exchange(World *w, WorldEconomy *econ){
    float y = tune_f("AGE_EXCHANGE_NODE_VALUE",1.0f);
    float lv = tune_f("AGE_EXCHANGE_MERCHANT_LEVER",0.08f);
    bool hit[SCPS_MAX_COUNTRY]={0};
    for (int r=0;r<econ->n_regions;r++){
        int o=econ->region[r].owner;
        if (o>=0 && o<SCPS_MAX_COUNTRY && econ->region[r].route_pe>y) hit[o]=true;
    }
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        if (hit[c]) faction_lever_apply(c, FAC_MARCHAND, lv);
}
static void age_lever_discovery(World *w, WorldProsperity *wp){
    float share_min = tune_f("AGE_DISCOVERY_KNOWN_PAIR_SHARE",0.12f);
    float lv_t = tune_f("AGE_DISCOVERY_TRANSGRESSEUR_LEVER",0.06f);
    float lv_m = tune_f("AGE_DISCOVERY_MERCHANT_LEVER",0.04f);
    int nc = nc_loop(w,wp);
    for (int a=0;a<nc && a<SCPS_MAX_COUNTRY;a++){
        if (w->country[a].role==POLITY_UNCLAIMED) continue;
        int n=0, known=0;
        for (int b=0;b<nc && b<SCPS_MAX_COUNTRY;b++){
            if (b==a || w->country[b].role==POLITY_UNCLAIMED) continue;
            n++; if (country_knows(a,b)) known++;
        }
        if (n>0 && (float)known/(float)n >= share_min){
            faction_lever_apply(a, FAC_TRANSGRESSEUR, lv_t);
            faction_lever_apply(a, FAC_MARCHAND, lv_m);
        }
    }
}
static void age_lever_empires(World *w, WorldEconomy *econ, WorldLegitimacy *wl){
    if (!wl) return;
    float held_min = tune_f("AGE_EMPIRES_HELD_YEARS",35.0f);
    int need_one = (int)tune_f("AGE_EMPIRES_REGIONS_ONE_COUNTRY",4.0f);
    float lv = tune_f("AGE_EMPIRES_CONQUEROR_LEVER",0.10f);
    int per_country[SCPS_MAX_COUNTRY]={0};
    for (int r=0;r<econ->n_regions;r++){
        int o=econ->region[r].owner;
        if (o<0 || o>=SCPS_MAX_COUNTRY || !econ->region[r].culture.settled) continue;
        if (r<SCPS_MAX_REG && wl->years_held[r] > held_min) per_country[o]++;
    }
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        if (per_country[c] >= need_one) faction_lever_apply(c, FAC_CONQUERANT, lv);
}
static void age_lever_breach(const TechState ts[]){
    if (!ts) return;
    float charge_min = tune_f("AGE_BREACH_CHARGE",6.0f);
    float lv = tune_f("AGE_BREACH_TRANSGRESSEUR_LEVER",0.12f);
    for (int c=0;c<SCPS_MAX_COUNTRY;c++)
        if (ts[c].charge > charge_min) faction_lever_apply(c, FAC_TRANSGRESSEUR, lv);
}
/* Lumières/Soulèvements/Tyrans : aucune condition PAR PAYS n'est donnée par la
 * spec (contrairement à Échanges/Découvertes/Empires/Brèche) — la « matière » est
 * l'appartenance au monde vivant (tout pays non-UNCLAIMED), sauf Soulèvements qui
 * NOMME explicitement « les pays révolutionnaires ». */
static void age_lever_lumieres(World *w, WorldProsperity *wp){
    float lv_l = tune_f("AGE_LUMIERES_LEGISTE_LEVER",0.06f);
    float lv_c = tune_f("AGE_LUMIERES_COMMUNAUTAIRE_LEVER",0.04f);
    for (int c=0;c<nc_loop(w,wp) && c<SCPS_MAX_COUNTRY;c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){
            faction_lever_apply(c, FAC_LEGISTE, lv_l);
            faction_lever_apply(c, FAC_COMMUNAUTAIRE, lv_c);
        }
}
static void age_lever_soulevements(World *w, WorldProsperity *wp){
    float lv = tune_f("AGE_SOULEVEMENTS_COMMUNAUTAIRE_LEVER",0.12f);
    for (int c=0;c<nc_loop(w,wp) && c<SCPS_MAX_COUNTRY;c++)
        if (w->country[c].role!=POLITY_UNCLAIMED && wp->country[c].mode==EV_MODE_REVOLUTION)
            faction_lever_apply(c, FAC_COMMUNAUTAIRE, lv);
}
static void age_lever_tyrans(World *w, WorldProsperity *wp){
    float lv_c = tune_f("AGE_TYRANS_CONQUEROR_LEVER",0.08f);
    float lv_l = tune_f("AGE_TYRANS_LEGISTE_LEVER",0.04f);
    for (int c=0;c<nc_loop(w,wp) && c<SCPS_MAX_COUNTRY;c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){
            faction_lever_apply(c, FAC_CONQUERANT, lv_c);
            faction_lever_apply(c, FAC_LEGISTE, lv_l);
        }
}

static void age_dawn(EventsState *ev, uint32_t seed, AgeId a, World *w, WorldEconomy *econ,
                     WorldProsperity *wp, WorldLegitimacy *wl, const TechState ts[]){
    EventCtx cx={ev,w,econ,NULL,wp,NULL,NULL,NULL,NULL,NULL,-1,NULL};
    EvEffect e; memset(&e,0,sizeof e); e.pop_mult=1.f; e.unlock_branch=-1;
    switch(a){
        case AGE_EXCHANGE:
            e.d_C_global = tune_f("AGE_EXCHANGE_C",0.50f);            /* PERMANENT */
            if (wp) wp->age_P_bonus += tune_f("AGE_EXCHANGE_P",0.50f);/* TRANSITOIRE */
            if (wp) wp->age_mig_mult = tune_f("AGE_EXCHANGE_MIG_PACT_MULT",1.15f); /* TRANSITOIRE */
            e.unlock_branch=THM_SOCIETE; e.unlock_tier=3;
            if (w && econ) age_lever_exchange(w,econ);
            break;
        case AGE_DISCOVERY:
            e.d_C_global = tune_f("AGE_DISCOVERY_C",0.50f);           /* PERMANENT */
            if (wp) wp->age_research_mult = tune_f("AGE_DISCOVERY_RESEARCH_MULT",1.10f); /* TRANSITOIRE */
            e.unlock_branch=THM_SAVOIR; e.unlock_tier=4;
            if (w) age_lever_discovery(w,wp);
            break;
        case AGE_EMPIRES:
            if (wp) wp->age_integration_mult = tune_f("AGE_EMPIRES_INTEGRATION_MULT",1.20f); /* PERMANENT */
            e.unlock_branch=THM_SOCIETE; e.unlock_tier=5;
            if (w && econ) age_lever_empires(w,econ,wl);
            break;
        case AGE_HEROES:
            break;   /* aucun effet mondial — la mécanique vit dans ages_hero_fire */
        case AGE_BREACH:
            e.d_breach = tune_f("AGE_BREACH_FLUX",1.50f);
            e.unlock_branch=THM_SAVOIR; e.unlock_tier=5;
            age_lever_breach(ts);
            break;
        case AGE_LUMIERES:
            if (wp){ wp->age_I_bonus         += tune_f("AGE_LUMIERES_I",1.50f);       /* TRANSITOIRE */
                     wp->age_lumiere_solvent += tune_f("AGE_LUMIERES_SOLVENT",1.25f); }/* TRANSITOIRE */
            if (w) age_lever_lumieres(w,wp);
            break;
        case AGE_SOULEVEMENTS:
            if (wp) wp->age_L_penalty += tune_f("AGE_SOULEVEMENTS_L",1.50f);   /* TRANSITOIRE */
            if (w) age_lever_soulevements(w,wp);
            break;
        case AGE_TYRANS:
            if (wp){ wp->age_H_bonus      += tune_f("AGE_TYRANS_H",1.75f);        /* TRANSITOIRE */
                     wp->age_myth_homogen += tune_f("AGE_TYRANS_DIVERSITY",1.50f); }/* TRANSITOIRE */
            /* AUCUNE conversion automatique des credos (spec verbatim) : PAS
             * d'appel à spread_credo_purificateur (retirée ci-dessus). */
            if (w) age_lever_tyrans(w,wp);
            break;
        default: break;
    }
    apply_effect(&cx, EV_WORLD, 0, &e);   /* pousse age_C_bonus/age_breach_flux/age_tech_mask (EV_WORLD, ci-dessus) */
    ev->ages.dawned[a]=true; ev->ages.last_dawned=(int)a;
    ev->ages.last_dawn_year = ev->ages.days_elapsed/365;   /* throttle des collisions : 1/an */
    /* §G2 — un ÂGE est le fait le PLUS notable : MÉMOIRE durable de poids plein
     * (subject = -1, échelle monde) → un présage l'évoquera plus tard. */
    dir_remember(&ev->director, ev->ages.days_elapsed, DMEM_AGE, -1, 1.0f);
    (void)seed;
}

bool events_check_ages(EventsState *ev, World *w, WorldEconomy *econ,
                       WorldProsperity *wp, WorldLegitimacy *wl, const TechState ts[]){
    uint32_t seed = w ? w->seed : 0u;
    int year = ev->ages.days_elapsed/365;
    /* 1. ÉLIGIBILITÉ — ACQUISE la première fois que le déclencheur devient vrai,
     * pour chaque âge non encore avenu (les âges déjà avenus n'ont plus rien à
     * latcher). AGE_HEROES est HORS scan (déclenché par ages_hero_fire). */
    bool trig[AGE_COUNT] = {0};
    trig[AGE_EXCHANGE]     = age_trig_exchange(w,econ,wp,ts);
    trig[AGE_DISCOVERY]    = age_trig_discovery(w,econ,wp,ts);
    trig[AGE_EMPIRES]      = age_trig_empires(w,econ,wp,ts,wl);
    trig[AGE_BREACH]       = age_trig_breach(w,econ,wp,ts);
    trig[AGE_LUMIERES]     = age_trig_lumieres(w,wp);
    trig[AGE_SOULEVEMENTS] = age_trig_soulevements(ev,w,wp);
    trig[AGE_TYRANS]       = age_trig_tyrans(ev,w,wp);
    /* DIAG GATED SCPS_AGEDIAG (même motif que SCPS_FINDIAG/SCPS_CAPDIAG, OFF par
     * défaut, stderr → déterminisme intact) — investigation « TYRANS 0/200 » :
     * Soulèvements (≥2 pays en révolution, condition bon marché) tire-t-il TOUJOURS
     * avant que Tyrans (fracture/déréalisation/SI mondiaux) ne devienne ne serait-ce
     * qu'ATTEIGNABLE ? Throttlé à 1/an (days_elapsed%365==0, cf. la cadence de
     * world_events_tick, appelé quotidiennement). */
    if (getenv("SCPS_AGEDIAG") && (ev->ages.days_elapsed % 365) == 0) {
        /* Signaux ÉCHANGES/DÉCOUVERTES ajoutés à la ligne (2e passe, question de la
         * mission « Échanges an 3-6 partout = trop uniforme ? ») : le comptage des
         * régions riches en route_pe + la part de paires connues, pour juger si un
         * seuil créerait de la variance — proposé seulement si LA DONNÉE le montre. */
        int rich=0, habited=0;
        for (int r=0;r<econ->n_regions;r++){
            if (econ->region[r].owner<0) continue;
            habited++;
            if (econ->region[r].route_pe > tune_f("AGE_EXCHANGE_NODE_VALUE",1.0f)) rich++;
        }
        fprintf(stderr,
            "[AGEDIAG] an %d : revolutionnaires=%d (seuil %d) | fracture_moy=%.2f (seuil>%.2f) "
            "dereal_moy=%.2f (seuil>%.2f) SI_moy=%.2f (seuil<%.2f) | Soulevements=%s Tyrans=%s "
            "dawned{S=%d,T=%d} | exch rich=%d/%d | pairs=%.3f\n",
            year, events_count_revolutionary(w,wp), (int)tune_f("AGE_SOULEVEMENTS_MIN_COUNTRIES",2.0f),
            (double)w_mean_fracture(w,wp), (double)tune_f("AGE_TYRANS_FRACTURE",3.0f),
            (double)w_mean_dereal(w,wp),   (double)tune_f("AGE_TYRANS_DEREAL",1.25f),
            (double)w_mean_SI(w,wp),       (double)tune_f("AGE_TYRANS_SI",5.0f),
            trig[AGE_SOULEVEMENTS]?"VRAI":"faux", trig[AGE_TYRANS]?"VRAI":"faux",
            (int)ev->ages.dawned[AGE_SOULEVEMENTS], (int)ev->ages.dawned[AGE_TYRANS],
            rich, habited, (double)ages_known_pair_share(w));
    }
    for (int a=0;a<AGE_COUNT;a++){
        if (a==AGE_HEROES || ev->ages.dawned[a] || ev->ages.year_eligible[a]>=0) continue;
        if (trig[a]) ev->ages.year_eligible[a] = (int16_t)year;
    }
    /* 2. AVÈNEMENT — au plus UN âge par an (le throttle RÉSOUT les collisions :
     * « un hash de la seed choisit le premier, les autres suivent les années
     * suivantes », sans état de plus — le perdant reste candidat, retenté l'an
     * prochain). Priorité DÉTERMINISTE (age_priority), pas l'ordre de l'enum. */
    if (year <= ev->ages.last_dawn_year) return false;
    AgeId best=AGE_COUNT; uint32_t best_pr=0;
    for (int a=0;a<AGE_COUNT;a++){
        if (a==AGE_HEROES || ev->ages.dawned[a] || ev->ages.year_eligible[a]<0) continue;
        if (year < age_dawn_year(seed,(AgeId)a,ev->ages.year_eligible[a])) continue;
        /* EXCLUSION MUTUELLE Soulèvements↔Tyrans — revérifiée ICI (pas à l'éligibilité,
         * cf. commentaire d'age_trig_soulevements/_tyrans) : l'un des deux a pu avenir
         * ENTRE le moment où CET âge est devenu éligible et aujourd'hui. */
        if (a==AGE_SOULEVEMENTS && ev->ages.dawned[AGE_TYRANS]) continue;
        if (a==AGE_TYRANS && ev->ages.dawned[AGE_SOULEVEMENTS]) continue;
        uint32_t pr = age_priority(seed,(AgeId)a);
        if (best==AGE_COUNT || pr<best_pr){ best=(AgeId)a; best_pr=pr; }
    }
    if (best==AGE_COUNT) return false;
    age_dawn(ev,seed,best,w,econ,wp,wl,ts);
    return true;
}
bool  ages_dawned(const EventsState *ev, AgeId a){ return (a>=0&&a<AGE_COUNT)?ev->ages.dawned[a]:false; }
bool  ages_tier_open(const WorldProsperity *wp, TechTheme br, int tier){
    if (!wp || br<0 || br>=THM_COUNT || tier<0 || tier>=8) return false;
    unsigned bit = (unsigned)br*8u + (unsigned)tier;
    return bit<32 && (wp->age_tech_mask & (1u<<bit)) != 0;
}
bool  ages_tech_researchable(const WorldProsperity *wp, TechTheme br, int tier){
    bool gated = (br==THM_SOCIETE && (tier==3 || tier==5))
              || (br==THM_SAVOIR  && (tier==4 || tier==5));
    return !gated || ages_tier_open(wp,br,tier);
}
float ages_breach_pressure(const EventsState *ev){ return ev->ages.breach_pressure; }

/* raccord 7 — L'ÂGE DES HÉROS. Le TEST (rang III + efficacité + loyauté + encore
 * assis) vit dans scps_sim.c (accès direct à Statecraft/MissionsState, ce module
 * n'en a pas besoin) ; ici on se contente de faire advenir l'âge (une fois) et de
 * pousser « Le nom du siècle » — DÉTERMINISTE, pas de mtth (fire_event(subject)
 * sans roll, comme age_dawn : le fait est déjà avéré au moment de l'appel). */
void ages_hero_fire(EventsState *ev, World *w, WorldEconomy *econ, WorldLegitimacy *wl,
                    WorldProsperity *wp, Statecraft *sc, RouteNetwork *rn,
                    const TechState ts[], DiploState *dp, EndgameState *eg,
                    MissionsState *ms, int cid, int seat, int slot, int gen,
                    int human_player){
    if (!ev || !w || cid<0 || cid>=w->n_countries) return;
    (void)slot; (void)gen;   /* l'identité est RE-DÉRIVÉE à la résolution (statecraft_council_seated) */
    if (!ev->ages.dawned[AGE_HEROES])
        age_dawn(ev, w->seed, AGE_HEROES, w, econ, wp, wl, ts);
    int evid;
    switch (seat){
        case 0: evid=EVID_HERO_SAVOIR;    break;
        case 1: evid=EVID_HERO_SOCIETE;   break;
        case 2: evid=EVID_HERO_INDUSTRIE; break;
        default: return;
    }
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,human_player,ms};
    fire_event(&cx, evid, cid);
}

/* ===================================================================== */
/* LE DIRECTEUR D'ÉVÉNEMENTS (§F) — stabilise / déstabilise, sans s'acharner */
/* ===================================================================== */
/* Le principe (F0) : chaque événement DIRIGÉ est un CHOC sur des variables
 * EXISTANTES (L, agitation, K, trésor, fertilité, rancune…), appliqué par le
 * même apply_effect que tout le reste. Le DIRECTEUR (F1) lit la TEMPÉRATURE du
 * monde et tire dans le pool qui MANQUE : monde chaud (chaos) → stabilisateur ;
 * monde froid (ronron) → déstabilisateur. L'ANTI-ACHARNEMENT (F2) borne dur :
 * 15 ans de repos par province, 5 ans par pays, jamais deux négatifs d'affilée
 * sur la même cible, jamais plus de 3 négatifs par province et par siècle. */
#define DIR_CHECK_DAYS  365          /* le directeur scanne une fois l'an */
#define DIR_PROV_CD    (15*365)      /* repos de 15 ans par province */
#define DIR_PAYS_CD     (5*365)      /* repos de 5 ans par pays */
#define DIR_FAM_DAYS    (15*365)     /* G0.1 : même ÉVÉNEMENT (famille) ≥ 15 ans avant de rejouer (monde) */
#define DIR_CONT_CD     (25*365)     /* G0.1 : même CONTINENT, événement continent-large ≥ 25 ans */
#define DIR_SOFT_DAYS   (30*365)     /* G0.1 : tirage sans remise — fenêtre de pénalité après un tir */
#define DIR_SOFT_DIV    0.25f        /* G0.1 : … poids ÷4 dans cette fenêtre */
#define DIR_T_HOT       0.50f        /* T au-dessus → APAISER (abaissé : les stabilisateurs jouent dès la tension) */
#define DIR_T_COLD      0.32f        /* T en dessous → REMUER */
#define DIR_NEG_CAP     3            /* ≤ 3 événements négatifs / province / siècle */
#define DIR_CENTURY    (100*365)

const char *director_event_name(int id){
    static const char *N[DIR_EV_COUNT]={
        "La Mort du Charismatique","La Peste Fluviale","Les Enfants du Palais","Le Filon",
        "L'Année Sans Été","Le Schisme dirigé","La Débase",
        "Le Congrès","La Réformatrice","Le Cadastre","La Décennie des Moissons",
        "La Paix du Marchand","Le Héros Culturel","L'Amnistie" };
    return (id>=0&&id<DIR_EV_COUNT)?N[id]:"?";
}
bool  director_is_destab(int id){ return (id>=0&&id<DIR_EV_COUNT) && id<DIR_STAB_FIRST; }
int   director_fired(const EventsState *ev,int id){ return (id>=0&&id<DIR_EV_COUNT)?ev->director.fired[id]:0; }
float director_temperature(const EventsState *ev){ return ev->director.last_T; }

/* ===================================================================== */
/* §G2 — LE DIRECTEUR-AMPLITUDE (la boucle « tale »)                      */
/* ===================================================================== */
/* Le principe (§G2) : un DIRECTEUR NARRATIF déterministe par-dessus le directeur
 * d'événements. Quatre ressorts, tous SÉRIALISÉS (dans Director, donc l'EVNT blob) :
 *   1. adapt_days — un INTÉGRATEUR DE TRAUMATISME : il MONTE sous les chocs (la
 *      température T agrège DÉJÀ guerre/famine/révolte/fracture, §F1) et REDESCEND
 *      au calme par demi-vie. Pas de hasard : T est lue de l'état du monde.
 *   2. amplitude = f(adapt_days) — l'amplitude « dramatique » [0..1], HAUTE juste
 *      après un choc, BASSE au ronron (saturation douce de adapt_days).
 *   3. budget — des points de mise en scène ∝ POP·RICHESSE·TEMPS (un monde riche,
 *      peuplé, âgé en accumule plus) ; ils financent les présages.
 *   4. la boucle TALE — un fait NOTABLE (événement dirigé / âge) laisse une MÉMOIRE
 *      durable (anneau) qui, plus tard, RESSURGIT en AUGURE/PRÉSAGE quand budget &
 *      amplitude le permettent. Le présage dépense du budget : la trace nourrit le récit.
 *
 * Lectures des tunables (registre J, §G2) ; bornes dures partout (sérialisé ⇒ revalidé). */
static inline float ampl_charge_per_day(void){ return tune_f("AMPL_TRAUMA_CHARGE",180.f)/365.f; }
static inline float ampl_trauma_max(void){ return tune_f("AMPL_TRAUMA_MAX",2000.f); }

/* L'amplitude dérivée du traumatisme : saturation douce adapt/(adapt+SCALE) ∈ [0..1[.
 * adapt=0 → 0 (calme plat) ; adapt=SCALE → 0.5 ; adapt≫ → →1 (le monde vibre). */
static float ampl_from_adapt(float adapt){
    float scale = tune_f("AMPL_TRAUMA_SCALE",500.f); if (scale<1.f) scale=1.f;
    if (adapt<0.f) adapt=0.f;
    return clampf(adapt/(adapt+scale), 0.f, 1.f);
}

/* Inscrit un fait NOTABLE dans l'anneau de mémoire (NOTABLE → MÉMOIRE durable). */
static void dir_remember(Director *D, int day, DirMemKind kind, int subject, float weight){
    if (D->mem_head<0 || D->mem_head>=DIR_MEM_CAP) D->mem_head=0;   /* sérialisé : on borne */
    DirMemory *m=&D->mem[D->mem_head];
    m->day=day; m->kind=(int)kind; m->subject=subject; m->weight=weight;
    D->mem_head=(D->mem_head+1)%DIR_MEM_CAP;
}
int director_memories(const EventsState *ev){
    const Director *D=&ev->director; int n=0;
    for (int i=0;i<DIR_MEM_CAP;i++) if (D->mem[i].day>0 && D->mem[i].kind!=DMEM_NONE) n++;
    return n;
}
float director_amplitude(const EventsState *ev){ return ev->director.amplitude; }
float director_adapt_days(const EventsState *ev){ return ev->director.adapt_days; }
float director_budget(const EventsState *ev){ return ev->director.budget; }
int   director_omens(const EventsState *ev){ return ev->director.omens; }

/* Le PAS d'amplitude (§G2) — intègre le traumatisme depuis T, recalcule amplitude
 * & budget, et tente un présage. DÉTERMINISTE (aucune fonction du hasard). Exposé
 * au banc (director_amplitude_step) ; appelé en interne par le tick du directeur. */
static bool director_amplitude_advance(Director *D, int day, float T, double pop, double gold, int days){
    if (days<1) days=1;
    /* 1. INTÉGRATEUR DE TRAUMATISME : charge ∝ T (le choc), décharge par demi-vie. */
    float chg = ampl_charge_per_day()*clampf(T,0.f,1.f)*(float)days;        /* le choc REMPLIT */
    float half = tune_f("AMPL_TRAUMA_HALF",900.f); if (half<1.f) half=1.f;
    float keep = expf(-0.69314718f*(float)days/half);                       /* le calme VIDE (demi-vie) */
    D->adapt_days = clampf(D->adapt_days*keep + chg, 0.f, ampl_trauma_max());
    /* 2. AMPLITUDE = f(adapt_days). */
    D->amplitude = ampl_from_adapt(D->adapt_days);
    if (D->amplitude > D->max_amplitude) D->max_amplitude = D->amplitude;
    /* 3. BUDGET ∝ POP·RICHESSE·TEMPS (le monde riche/peuplé/âgé met plus en scène). */
    if (pop<0.0)  pop=0.0;
    if (gold<0.0) gold=0.0;
    float yfrac = (float)days/365.f;
    float gain = (tune_f("AMPL_BUDGET_POP",0.02f)*(float)(pop/1000.0)
                + tune_f("AMPL_BUDGET_GOLD",0.01f)*(float)(gold/1000.0)) * yfrac;
    D->budget = clampf(D->budget + gain, 0.f, tune_f("AMPL_BUDGET_CAP",400.f));
    /* 4. LA BOUCLE TALE : une MÉMOIRE durable RESSURGIT en PRÉSAGE quand le monde
     *    vibre (amplitude ≥ seuil) ET que le budget paie. On choisit le plus VIEUX
     *    fait encore en mémoire (un présage évoque le passé, pas l'écho immédiat) et
     *    on le « consume » (case vidée) : la trace a nourri le récit. Déterministe. */
    bool emitted=false;
    float cost = tune_f("AMPL_OMEN_COST",60.f); if (cost<1.f) cost=1.f;
    if (D->amplitude >= tune_f("AMPL_OMEN_AMPL",0.35f) && D->budget >= cost){
        int oldest=-1, oldest_day=0;
        for (int i=0;i<DIR_MEM_CAP;i++){
            const DirMemory *m=&D->mem[i];
            if (m->day<=0 || m->kind==DMEM_NONE) continue;
            if (m->day >= day) continue;                  /* le présage ÉVOQUE le passé révolu */
            if (oldest<0 || m->day<oldest_day){ oldest=i; oldest_day=m->day; }
        }
        if (oldest>=0){
            D->budget -= cost;                            /* le présage SE PAIE sur le budget */
            D->mem[oldest].day=0; D->mem[oldest].kind=DMEM_NONE;  /* consommé (la trace a parlé) */
            D->omens++;
            emitted=true;
        }
    }
    return emitted;
}
bool director_amplitude_step(EventsState *ev, float T, double pop, double gold, int days){
    return director_amplitude_advance(&ev->director, ev->ages.days_elapsed, T, pop, gold, days);
}

/* §G2 — REVALIDATION du directeur-amplitude désérialisé (la garde de save_sane). */
bool director_save_sane(const EventsState *ev, int max_subject){
    if (!ev) return false;
    const Director *D=&ev->director;
    if (D->mem_head < 0 || D->mem_head >= DIR_MEM_CAP) return false;   /* index d'écriture de l'anneau */
    for (int i=0;i<DIR_MEM_CAP;i++){
        if (D->mem[i].kind < 0 || D->mem[i].kind >= DMEM_KIND_COUNT) return false;
        if (D->mem[i].subject < -1 || D->mem[i].subject >= max_subject) return false;
    }
    if (D->omens < 0) return false;
    return true;
}

/* MEMBRANE DE DÉCISION — REVALIDATION des trois anneaux désérialisés (motif
 * director_save_sane) : subject/kind/evid bornés, jamais déréférencés hors-borne. */
bool decision_memory_save_sane(const EventsState *ev, int max_subject){
    if (!ev) return false;
    if (ev->scar_head < 0 || ev->scar_head >= DECISION_SCAR_CAP) return false;
    for (int i=0;i<DECISION_SCAR_CAP;i++){
        const DecisionScar *s=&ev->scars[i];
        if (s->kind < 0 || s->kind >= SCAR_KIND_COUNT) return false;
        if (s->subject < -1 || s->subject >= max_subject) return false;
    }
    if (ev->cd_head < 0 || ev->cd_head >= DECISION_CD_CAP) return false;
    for (int i=0;i<DECISION_CD_CAP;i++){
        const EvCooldown *c=&ev->cds[i];
        if (c->evid >= EVID_COUNT) return false;   /* uint8_t : jamais < 0 */
        if (c->subject < -1 || c->subject >= max_subject) return false;
    }
    if (ev->pending_n < 0 || ev->pending_n > PENDING_EVENT_CAP) return false;
    for (int i=0;i<ev->pending_n;i++){
        const PendingEvent *p=&ev->pending[i];
        if (p->evid >= EVID_COUNT) return false;
        if (p->subject < -1 || p->subject >= max_subject) return false;
    }
    return true;
}

/* ---- LA TEMPÉRATURE T (F1) : 0 = ronron, 1 = chaos ---------------------- */
static int dir_wars_active(const DiploState *dp, const World *w){
    if (!dp) return 0;
    int n=0;
    for (int a=0;a<w->n_countries;a++) for (int b=a+1;b<w->n_countries;b++)
        if (diplo_status(dp,a,b)==DIPLO_WAR) n++;
    return n;
}
/* G0.1 — T lit la CRISE, pas la moyenne séculaire : un seul hégémon qui craque
 * (worst élevé) ou une vague de révoltes (part en crise / révolution) doit la
 * faire MONTER — sinon T restait à 0.14 pendant l'effondrement d'un géant. Échelle
 * instantanée (le scan est annuel → fenêtre courte par construction). */
static float director_compute_T(const EventCtx *cx){
    World *w=cx->w; WorldProsperity *wp=cx->wp;
    int nc = (w->n_countries < wp->n_countries)? w->n_countries : wp->n_countries;
    int nliv=0, ncrisis=0; float worst=0.f, sumchaos=0.f, sumfrac=0.f;
    for (int c=0;c<nc;c++){
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        nliv++;
        float chaos = clampf((10.f - wp->country[c].SI)/10.f, 0.f, 1.f);   /* SI 0..10 */
        sumchaos += chaos; if (chaos>worst) worst=chaos;
        if (wp->country[c].SI < 5.f) ncrisis++;                            /* en difficulté */
        sumfrac  += wp->country[c].fracture;
    }
    if (nliv==0) return 0.f;
    float meanchaos  = sumchaos/(float)nliv;
    float crisisfrac = (float)ncrisis/(float)nliv;
    float rev  = clampf((float)events_count_revolutionary(w,wp)/(float)nliv, 0.f,1.f);
    float frac = clampf((sumfrac/(float)nliv)/10.f, 0.f, 1.f);
    float wars = clampf((float)dir_wars_active(cx->dp,w)/(float)nliv, 0.f, 1.f);
    float T = 0.35f*worst + 0.25f*crisisfrac + 0.15f*rev + 0.10f*meanchaos + 0.08f*frac + 0.07f*wars;
    return clampf(T, 0.f, 1.f);
}

/* ---- ANTI-ACHARNEMENT (F2) --------------------------------------------- */
static bool dir_ok_region(const Director *D, int day, int r, bool negative){
    if (r<0||r>=SCPS_MAX_REG) return false;
    if (D->prov_cd_until[r] > day) return false;                  /* la province se repose */
    if (negative && D->prov_last_neg[r]==1) return false;         /* jamais deux négatifs d'affilée */
    if (negative && D->prov_neg_century[r] >= DIR_NEG_CAP) return false; /* plafond du siècle */
    return true;
}
static bool dir_ok_pays(const Director *D, int day, int c){
    return c>=0 && c<SCPS_MAX_COUNTRY && D->pays_cd_until[c] <= day;
}
static void dir_touch(Director *D, int day, int r, int owner, bool negative){
    if (r>=0 && r<SCPS_MAX_REG){
        D->prov_cd_until[r]  = day + DIR_PROV_CD;
        D->prov_last_neg[r]  = negative ? 1 : 0;
        if (negative){
            if (D->prov_neg_century[r] < 255) D->prov_neg_century[r]++;
            if (D->prov_neg_century[r] > DIR_NEG_CAP) D->neg_over_cap++;   /* DOIT rester 0 (preuve F2) */
        }
    }
    if (owner>=0 && owner<SCPS_MAX_COUNTRY) D->pays_cd_until[owner] = day + DIR_PAYS_CD;
}

/* ---- Les CHOCS (F3/F4), via le même apply_effect que tout le module ---- */
static void dir_country_eff(EventCtx *cx, int cid, float dL, float dAgit, float dK, float dTreasury){
    EvEffect e; memset(&e,0,sizeof e); e.pop_mult=1.f;
    e.d_L=dL; e.d_agitation=dAgit; e.d_K_inst=dK; e.d_treasury=dTreasury;
    apply_effect(cx, EV_COUNTRY, cid, &e);
}
static void dir_region_eff(EventCtx *cx, int r, float dL, float dAgit, float dTreasury, float popMult){
    EvEffect e; memset(&e,0,sizeof e); e.pop_mult=(popMult<=0.f)?1.f:popMult;
    e.d_L=dL; e.d_agitation=dAgit; e.d_treasury=dTreasury;
    apply_effect(cx, EV_PROVINCE, r, &e);
}

/* Trouve un SUJET valide pour l'événement `id` (lit l'état du monde + anti-acharnement).
 * subject = pays (scope pays), région (scope région), continent (scope continent),
 * ou a·MAX+b (Amnistie). Renvoie false si rien d'éligible. */
static bool dir_eligible(EventCtx *cx, int id, int day, int *out){
    Director *D=&cx->ev->director; World *w=cx->w; WorldEconomy *econ=cx->econ; WorldProsperity *wp=cx->wp;
    uint32_t *rng=&cx->ev->rng;
    bool neg = director_is_destab(id);
    int nc=w->n_countries, nr=econ->n_regions;
    int s0;
    switch(id){
        case DIR_CHARISMA: case DIR_PALAIS: case DIR_SCHISME: case DIR_DEBASE:
        case DIR_CONCILE: case DIR_REFORME: case DIR_CADASTRE: case DIR_MARCHAND:
            s0=(int)(frand(rng)*(float)nc);
            for (int i=0;i<nc;i++){ int c=(s0+i)%nc;
                if (w->country[c].role==POLITY_UNCLAIMED) continue;
                if (!dir_ok_pays(D,day,c)) continue;
                /* H8 — fenêtres ÉLARGIES : chaque événement doit pouvoir se déclencher quelque part. */
                if (id==DIR_CHARISMA){ int rc=0; for (int r=0;r<nr;r++) if (econ->region[r].owner==c) rc++;
                                       if (!(wp->country[c].L>6.f || (wp->country[c].K<4.f && rc>=5))) continue; }  /* L haut OU (K bas ET ≥5 rég) */
                if (id==DIR_PALAIS   && !(wp->country[c].L<5.f)) continue;
                if (id==DIR_SCHISME  && !EVENTS[EVID_SCHISM].trigger(cx,c)) continue;
                if (id==DIR_DEBASE){ int cr=world_capital_region(w,c); if (cr<0||econ->region[cr].treasury>=2000.f) continue; }  /* trésor bas (élargi 200→2000) */
                if (id==DIR_CADASTRE && !(wp->country[c].K>=6.f)) continue;
                if (id==DIR_REFORME  && !(wp->country[c].L<5.f)) continue;
                if (id==DIR_MARCHAND && w->country[c].role!=POLITY_CITY_STATE) continue;
                /* anti-acharnement (F2) : un événement pays NÉGATIF frappe la capitale
                 * → il subit AUSSI le repos 15 ans + le plafond 3-négatifs/siècle de
                 * cette province (sinon la même capitale encaissait un coup tous les
                 * 5 ans — le cooldown pays — et dépassait le plafond). */
                if (neg){ int cr=world_capital_region(w,c); if (!dir_ok_region(D,day,cr,true)) continue; }
                *out=c; return true;
            }
            return false;
        case DIR_FILON:
            s0=(int)(frand(rng)*(float)nr);
            for (int i=0;i<nr;i++){ int r=(s0+i)%nr;
                if (!econ->region[r].colonized) continue;
                if (cx->ev->geo[r].relief < 0.25f) continue;      /* H8 : tout relief à métal/pierre (élargi 0.5→0.25) */
                if (!dir_ok_region(D,day,r,neg)) continue;
                *out=r; return true;
            }
            return false;
        case DIR_HEROS:
            s0=(int)(frand(rng)*(float)nr);
            for (int i=0;i<nr;i++){ int r=(s0+i)%nr;
                if (!econ->region[r].colonized) continue;
                /* RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un
                 * miroir de la province représentative. L'effet (dir_region_eff → apply_region_eff)
                 * atterrit déjà sur cette même province représentative : on y juge la composition
                 * pour rester cohérent avec ce qui recevra réellement l'effet. */
                { int hpid=econ_region_rep_province(econ,r);
                  if (hpid<0 || hpid>=econ->n_prov || econ->prov[hpid].pop.n_groups<2) continue; }  /* composite (syncrétique) */
                if (!dir_ok_region(D,day,r,false)) continue;
                *out=r; return true;
            }
            return false;
        case DIR_PESTE: {
            int best=-1; float bv=0.5f;                            /* l'estuaire-carrefour : meilleure route_pe */
            for (int r=0;r<nr;r++){ if (!econ->region[r].colonized) continue;
                if (!dir_ok_region(D,day,r,neg)) continue;
                if (econ->region[r].route_pe>bv){ bv=econ->region[r].route_pe; best=r; } }
            if (best<0) return false;
            *out=best; return true;
        }
        case DIR_ANNEE: case DIR_MOISSONS: {
            int ncont=w->n_continents; if (ncont<1) return false;
            s0=(int)(frand(rng)*(float)ncont);
            for (int i=0;i<ncont;i++){ int ct=(s0+i)%ncont;
                if (ct>=0 && ct<SCPS_MAX_CONTINENT && D->cont_cd_until[ct] > day) continue;  /* G0.1 : repos continent ≥25 ans */
                for (int r=0;r<nr && r<w->n_regions;r++)
                    if (w->region[r].continent==ct && econ->region[r].colonized && dir_ok_region(D,day,r,neg)){ *out=ct; return true; }
            }
            return false;
        }
        case DIR_AMNISTIE: {
            if (!cx->dp) return false;
            s0=(int)(frand(rng)*(float)nc);
            for (int i=0;i<nc;i++){ int a=(s0+i)%nc;
                if (w->country[a].role==POLITY_UNCLAIMED || !dir_ok_pays(D,day,a)) continue;
                for (int b=0;b<nc;b++){ if (b==a||w->country[b].role==POLITY_UNCLAIMED) continue;
                    if (diplo_status(cx->dp,a,b)==DIPLO_WAR) continue;   /* on éponge APRÈS la guerre, pas pendant */
                    if (cx->dp->rancor[a][b]>1.0f && dir_ok_pays(D,day,b)){ *out=a*SCPS_MAX_COUNTRY+b; return true; } }
            }
            return false;
        }
    }
    return false;
}

/* Applique l'effet de `id` sur `subject` et POSE l'anti-acharnement. */
static void dir_apply(EventCtx *cx, int id, int subject, int day){
    Director *D=&cx->ev->director; World *w=cx->w; WorldEconomy *econ=cx->econ;
    switch(id){
        case DIR_CHARISMA: dir_country_eff(cx,subject,-2.0f,20.f,0.f,0.f);    dir_touch(D,day,world_capital_region(w,subject),subject,true); break;
        case DIR_PALAIS:   dir_country_eff(cx,subject,-1.5f,25.f,0.f,0.f);    dir_touch(D,day,world_capital_region(w,subject),subject,true); break;
        case DIR_DEBASE:   dir_country_eff(cx,subject,-1.0f, 5.f,0.f,2000.f); dir_touch(D,day,world_capital_region(w,subject),subject,true); break;
        case DIR_SCHISME:  fire_event(cx,EVID_SCHISM,subject);                dir_touch(D,day,world_capital_region(w,subject),subject,true); break;
        case DIR_FILON:
            dir_region_eff(cx,subject,0.f,15.f,3000.f,1.f);   /* la ruée : or local + agitation */
            if (econ->region[subject].build.H_coerc<1.f){    /* camps sauvages si la poigne manque */
                /* RE-KEY PROVINCE : revolt_scar province-owned — route sur la représentative. */
                int fpid=econ_region_rep_province(econ,subject);
                if (fpid>=0 && fpid<econ->n_prov) econ->prov[fpid].revolt_scar=1.f;
            }
            dir_touch(D,day,subject,econ->region[subject].owner,true); break;
        case DIR_PESTE:
            events_plague_spread(cx->ev,w,econ,cx->wl,cx->sc,cx->rn,subject);  /* suit les ROUTES (C est le vecteur) */
            dir_touch(D,day,subject,econ->region[subject].owner,true); break;
        case DIR_HEROS:
            dir_region_eff(cx,subject,1.0f,-10.f,0.f,1.f);    /* cohésion + (L), grogne − */
            dir_touch(D,day,subject,econ->region[subject].owner,false); break;
        case DIR_ANNEE: { int ct=subject;
            if (ct>=0 && ct<SCPS_MAX_CONTINENT) D->cont_cd_until[ct]=day+DIR_CONT_CD;  /* G0.1 : repos continent 25 ans */
            for (int r=0;r<econ->n_regions && r<w->n_regions;r++)
                if (w->region[r].continent==ct && econ->region[r].colonized && dir_ok_region(D,day,r,true)){
                    dir_region_eff(cx,r,0.f,8.f,0.f,1.f);
                    econ->region[r].build.food_cap=fmaxf(0.f, econ->region[r].build.food_cap-2.f);  /* fertilité ↓ → famines */
                    dir_touch(D,day,r,econ->region[r].owner,true);
                } } break;
        case DIR_MOISSONS: { int ct=subject;
            if (ct>=0 && ct<SCPS_MAX_CONTINENT) D->cont_cd_until[ct]=day+DIR_CONT_CD;  /* G0.1 : repos continent 25 ans */
            for (int r=0;r<econ->n_regions && r<w->n_regions;r++)
                if (w->region[r].continent==ct && econ->region[r].colonized && dir_ok_region(D,day,r,false)){
                    econ->region[r].build.food_cap += 1.5f;          /* fertilité ↑ (IPM ↓ émergera en §C) */
                    dir_touch(D,day,r,econ->region[r].owner,false);
                } } break;
        case DIR_CONCILE:  dir_country_eff(cx,subject, 1.5f,-8.f,0.f,0.f);  dir_touch(D,day,world_capital_region(w,subject),subject,false); break;
        case DIR_REFORME:  dir_country_eff(cx,subject, 1.0f,-10.f,1.5f,0.f);dir_touch(D,day,world_capital_region(w,subject),subject,false); break;
        case DIR_CADASTRE: dir_country_eff(cx,subject, 0.5f,-5.f,0.5f,0.f); dir_touch(D,day,world_capital_region(w,subject),subject,false); break;
        case DIR_MARCHAND:
            dir_country_eff(cx,subject, 0.5f,-6.f,0.f,0.f);
            if (cx->wp) cx->wp->age_C_bonus=clampf(cx->wp->age_C_bonus+0.2f,0.f,5.f);   /* le négoce relie (C ↑) */
            dir_touch(D,day,world_capital_region(w,subject),subject,false); break;
        case DIR_AMNISTIE: { int a=subject/SCPS_MAX_COUNTRY, b=subject%SCPS_MAX_COUNTRY;
            if (cx->dp){ cx->dp->rancor[a][b]*=0.5f; cx->dp->rancor[b][a]*=0.5f; }   /* la rancune ÷2 */
            dir_country_eff(cx,a,1.f,0.f,0.f,0.f); dir_country_eff(cx,b,1.f,0.f,0.f,0.f);
            dir_touch(D,day,world_capital_region(w,a),a,false); dir_touch(D,day,world_capital_region(w,b),b,false); } break;
    }
}

/* §G2 — POP & RICHESSE du monde (les dimensions du budget de mise en scène).
 * pop = Σ strata sur les régions colonisées ; gold = Σ or des pays vivants. */
static void dir_world_scale(const World *w, const WorldEconomy *econ, double *out_pop, double *out_gold){
    double pop=0.0, gold=0.0;
    for (int r=0;r<econ->n_regions;r++){
        if (econ->region[r].owner<0) continue;
        for (int k=0;k<CLASS_COUNT;k++) pop += econ->region[r].strata[k].pop;
    }
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        if (w->country[c].role!=POLITY_UNCLAIMED){ double g=econ_country_gold(econ,c); if (g>0.0) gold+=g; }
    *out_pop=pop; *out_gold=gold;
}

/* La boucle du directeur : siècle glissant → cadence → T → AMPLITUDE → pool → tir. */
static void director_tick(EventCtx *cx, int days){
    (void)days;
    Director *D=&cx->ev->director; int day=cx->ev->ages.days_elapsed;
    if (day - D->century_base_day >= DIR_CENTURY){       /* le siècle tourne : on oublie les vieux coups */
        memset(D->prov_neg_century,0,sizeof D->prov_neg_century);
        D->century_base_day=day;
    }
    if (day < D->next_check_day) return;
    int scan_days = day - (D->next_check_day - DIR_CHECK_DAYS);   /* jours réels écoulés depuis le scan précédent */
    if (scan_days < 1) scan_days = DIR_CHECK_DAYS;
    D->next_check_day = day + DIR_CHECK_DAYS;
    float T = director_compute_T(cx); D->last_T=T; if (T>D->max_T) D->max_T=T;
    /* §G2 — L'AMPLITUDE AVANCE À CHAQUE SCAN (même au calme : l'intégrateur DÉCROÎT
     * sinon). Elle dimensionne aussi la trace : un fait NOTABLE pèse ∝ amplitude. */
    { double wpop=0.0, wgold=0.0; dir_world_scale(cx->w,cx->econ,&wpop,&wgold);
      director_amplitude_advance(D, day, T, wpop, wgold, scan_days); }
    int want;
    if      (T > tune_f("DIR_T_HOT",DIR_T_HOT))  want=+1;                    /* trop chaud → stabilisateur */
    else if (T < tune_f("DIR_T_COLD",DIR_T_COLD)) want=-1;                    /* trop froid → déstabilisateur */
    else return;                                         /* zone saine : on laisse le monde vivre */
    bool want_destab=(want<0);
    int lo = want_destab?0:DIR_STAB_FIRST;
    int hi = want_destab?DIR_STAB_FIRST:DIR_EV_COUNT;
    int span=hi-lo; if (span<1) return;
    int off=(int)(frand(&cx->ev->rng)*(float)span); if (off>=span) off=span-1;
    /* G0.1 — TIRAGE SANS REMISE : parmi les éligibles (qui passent déjà les cooldowns
     * durs famille 15 ans / continent 25 ans), tirage ALÉATOIRE PONDÉRÉ ; un événement
     * joué dans les 30 dernières années pèse ÷4. Aucun type ne peut donc monopoliser
     * (top ≤ 30 % des tirages). L'ordre de balayage tourne (off) → déterministe. */
    int   eid[DIR_EV_COUNT], esub[DIR_EV_COUNT], ncand=0;
    float ew[DIR_EV_COUNT], wsum=0.f;
    for (int k=0;k<span;k++){
        int id=lo + (off+k)%span;
        if (D->fam_active_until[id] > day) continue;     /* même événement : repos ≥15 ans */
        int subject=-1;
        if (!dir_eligible(cx,id,day,&subject)) continue;
        float wgt = 1.f;
        if (D->fired[id]>0 && day - D->last_fired_day[id] < DIR_SOFT_DAYS) wgt *= DIR_SOFT_DIV;  /* ÷4 pendant 30 ans après un tir */
        eid[ncand]=id; esub[ncand]=subject; ew[ncand]=wgt; wsum+=wgt; ncand++;
    }
    if (ncand<1 || wsum<=0.f) return;                    /* rien d'éligible ce scan */
    float r = frand(&cx->ev->rng)*wsum; int ci=0;
    for (; ci<ncand-1; ci++){ if (r < ew[ci]) break; r -= ew[ci]; }
    int pick=eid[ci], pick_subj=esub[ci];
    dir_apply(cx,pick,pick_subj,day);
    D->fam_active_until[pick] = day + DIR_FAM_DAYS;       /* repos famille 15 ans (continent : +25 ans via cont_cd) */
    D->last_fired_day[pick]   = day;                      /* … et pénalité ÷4 pour 30 ans */
    D->fired[pick]++; if (want_destab) D->fired_destab++; else D->fired_stab++;
    cx->ev->last_id=-1; cx->ev->last_name=director_event_name(pick); cx->ev->n_fired++;
    /* §G2 — la boucle TALE : l'événement dirigé est un fait NOTABLE → MÉMOIRE durable
     * (pondérée par l'amplitude du moment). Plus tard, au gré du budget, il RESSURGIT
     * en PRÉSAGE. Le sujet est gardé tel quel (pays/région/continent/Amnistie a·MAX+b). */
    dir_remember(D, day, want_destab?DMEM_DESTAB:DMEM_STAB, pick_subj, 0.5f + 0.5f*D->amplitude);
}

/* ===================================================================== */
/* LA BOUCLE (§5)                                                        */
/* ===================================================================== */
static float mtth_p(float mtth_days, int days){
    if (mtth_days<1.f) mtth_days=1.f;
    return 1.f - expf(-(float)days/mtth_days);
}
void world_events_tick(EventsState *ev, World *w, WorldEconomy *econ,
                       WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                       RouteNetwork *rn, const TechState ts[], DiploState *dp,
                       EndgameState *eg, int days,
                       int human_player){
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,human_player,NULL};
    g_title_sc = sc;   /* lot M : latch display-only pour event_title (noms de ministre) */
    ev->ages.days_elapsed += days;          /* horloge de jeu (rythme des âges) */

    /* LOT F (2026-07-08) — CATASTROPHES DU MONDE CALME : « si pas de fin, forcer des
     * catastrophes naturelles, ça devrait motiver l'IA à faire des trucs » (demande
     * joueur). Un monde SANS fin en vue — aucune fin latchée, passé un an-butoir,
     * entropie loin du seuil (le monde n'est pas simplement en train de charger une
     * fin qui va bientôt éclore) — reçoit une pression accrue de catastrophes GÉO,
     * réutilisant le motif EVENTS[] EXISTANT (quake/flood/drought/fire/peste) au lieu
     * d'un système neuf : `calm_mult` divise leur mtth (plus fréquent), jamais une
     * fin en soi — une PRESSION, pas un forçage narratif. */
    float calm_mult = 1.f;
    if (eg && wp && !eg->fired) {
        int yr = ev->ages.days_elapsed / 365;
        if (yr > (int)tune_f("CALM_DISASTER_YEAR", 200.f)
            && wp->entropy < tune_f("ENTROPY_FIN", 55.f) * tune_f("CALM_DISASTER_ENTFRAC", 0.5f))
            calm_mult = tune_f("CALM_DISASTER_MULT", 2.5f);
    }

    /* 1. CHOCS GÉO — à risque, par région, sur leur cadence (1/risk accélère). */
    static const int SHOCKS[4]={EVID_QUAKE,EVID_FLOOD,EVID_DROUGHT,EVID_FIRE};
    for (int r=0;r<econ->n_regions;r++){
        if (!econ->region[r].culture.settled) continue;
        for (int si=0;si<4;si++){
            int s=SHOCKS[si];
            float risk=shock_risk(ev,r,s);
            if (risk<0.06f) continue;                       /* la géo l'interdit ici */
            float mtth=EVENTS[s].mtth_days / risk / calm_mult;  /* fréquent là où le risque est haut — et sur un monde calme */
            if (frand(&ev->rng) < mtth_p(mtth,days)){
                events_strike(ev,w,econ,wl,wp,sc,r,s);
                if (calm_mult>1.f) g_calm_shocks_fired++;
            }
        }
    }
    /* Peste : foyer rare sur le plus grand carrefour ouvert, puis diffusion.
     * (NODE_VALUE_Y vivait autrefois à côté des seuils d'âge ; ce seuil « région
     * riche » est réutilisé ICI, sans rapport avec les Âges — cf. AGE_EXCHANGE_
     * NODE_VALUE pour l'équivalent tunable côté Échanges.) */
    {
        int hub=-1; float best=1.0f;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].route_pe>best){ best=econ->region[r].route_pe; hub=r; }
        if (hub>=0 && frand(&ev->rng) < mtth_p(EVENTS[EVID_PLAGUE].mtth_days / calm_mult, days)){
            events_plague_spread(ev,w,econ,wl,sc,rn,hub);
            if (calm_mult>1.f) g_calm_shocks_fired++;
        }
    }

    /* 2. ÉVÈNEMENTS POLITIQUES/CULTURELS — la fiche sélectionne la variante. */
    for (int r=0;r<econ->n_regions;r++){
        if (!econ->region[r].culture.settled) continue;
        int evid=events_match_political(ev,w,econ,wl,sc,r);
        if (evid<0) continue;
        if (frand(&ev->rng) < mtth_p(EVENTS[evid].mtth_days, days)) fire_event(&cx, evid, r);
    }
    for (int c=0;c<w->n_countries;c++){
        if (EVENTS[EVID_SUCCESSION].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_SUCCESSION].mtth_days,days)) fire_event(&cx,EVID_SUCCESSION,c);
        if (EVENTS[EVID_SCHISM].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_SCHISM].mtth_days,days)) fire_event(&cx,EVID_SCHISM,c);
    }
    /* 2ter. FLORAISON COSMOPOLITE — une province DIVERSE, accueillante et apaisée (les cultures
     * CONVERGENT) prospère. Court-circuit (trigger && frand) : aucun tirage tant qu'aucun creuset
     * n'existe → le déterminisme court terme ne bouge pas tant que la diversité n'a pas mûri. */
    for (int r=0;r<econ->n_regions;r++){
        if (EVENTS[EVID_XENOPHILE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_XENOPHILE].mtth_days,days)) fire_event(&cx,EVID_XENOPHILE,r);
        if (EVENTS[EVID_XENOPHOBE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_XENOPHOBE].mtth_days,days)) fire_event(&cx,EVID_XENOPHOBE,r);
    }

    /* 2quater. MEMBRANE DE DÉCISION — MARBRIVE (le contremaître réclame) + sa suite
     * CONSÉQUENTE (le sabotage mûrit en catastrophe, une cicatrice posée par le choix
     * « Envoyer les prévôts »). Même court-circuit (trigger && frand) que XENOPHILE. */
    for (int r=0;r<econ->n_regions;r++){
        if (EVENTS[EVID_MARBRIVE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_MARBRIVE].mtth_days,days)) fire_event(&cx,EVID_MARBRIVE,r);
        if (EVENTS[EVID_PONT_EFFONDRE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_PONT_EFFONDRE].mtth_days,days)) fire_event(&cx,EVID_PONT_EFFONDRE,r);
    }

    /* 2quinquies. CONTENU W1 — six évènements neufs. Même court-circuit (trigger &&
     * frand) que MARBRIVE/XENOPHILE ; les quatre EV_PROVINCE scannent les régions,
     * SALVE_RUNIQUE (EV_COUNTRY) scanne les pays (mécanisme « tech just-latched » :
     * le trigger lit unlocked[TECH_APEX_ARQUEBUSE], le cooldown 36500 j du hook fait
     * le déclenchement UNIQUE une fois qu'il a tiré). */
    for (int r=0;r<econ->n_regions;r++){
        if (EVENTS[EVID_CLOCHES].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_CLOCHES].mtth_days,days)) fire_event(&cx,EVID_CLOCHES,r);
        if (EVENTS[EVID_ENTREPOTS_FERMES].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_ENTREPOTS_FERMES].mtth_days,days)) fire_event(&cx,EVID_ENTREPOTS_FERMES,r);
        if (EVENTS[EVID_DEUX_CARTES].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_DEUX_CARTES].mtth_days,days)) fire_event(&cx,EVID_DEUX_CARTES,r);
        if (EVENTS[EVID_EAU_NOIRE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_EAU_NOIRE].mtth_days,days)) fire_event(&cx,EVID_EAU_NOIRE,r);
        if (EVENTS[EVID_DERNIERE_DECISION].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_DERNIERE_DECISION].mtth_days,days)) fire_event(&cx,EVID_DERNIERE_DECISION,r);
    }
    for (int c=0;c<w->n_countries;c++){
        if (EVENTS[EVID_SALVE_RUNIQUE].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_SALVE_RUNIQUE].mtth_days,days)) fire_event(&cx,EVID_SALVE_RUNIQUE,c);
    }

    /* 2sexies. CONTENU W2 (lot 2) — §A tech (déclenchement unique, motif SALVE_RUNIQUE) ·
     * §B culturel · §C religieux · §D chaînage. Même court-circuit (trigger && frand).
     * EV_PROVINCE (A6/C6/K2/K3/K7) scannent les régions ; EV_COUNTRY (A1-A5/B1/B4/C1/C5/
     * K5/K6) scannent les pays. */
    for (int r=0;r<econ->n_regions;r++){
        if (EVENTS[EVID_FOREUSE_SAIGNE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_FOREUSE_SAIGNE].mtth_days,days)) fire_event(&cx,EVID_FOREUSE_SAIGNE,r);
        if (EVENTS[EVID_RELIQUE_DOUTEUSE].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_RELIQUE_DOUTEUSE].mtth_days,days)) fire_event(&cx,EVID_RELIQUE_DOUTEUSE,r);
        if (EVENTS[EVID_REMEDE_MORTS].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_REMEDE_MORTS].mtth_days,days)) fire_event(&cx,EVID_REMEDE_MORTS,r);
        if (EVENTS[EVID_CELLULE_FAUBOURGS].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_CELLULE_FAUBOURGS].mtth_days,days)) fire_event(&cx,EVID_CELLULE_FAUBOURGS,r);
        if (EVENTS[EVID_TARIF_APPRIS].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_TARIF_APPRIS].mtth_days,days)) fire_event(&cx,EVID_TARIF_APPRIS,r);
    }
    for (int c=0;c<w->n_countries;c++){
        if (EVENTS[EVID_CHAINES_RAPPORTENT].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_CHAINES_RAPPORTENT].mtth_days,days)) fire_event(&cx,EVID_CHAINES_RAPPORTENT,c);
        if (EVENTS[EVID_OEUVRE_NOIRE].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_OEUVRE_NOIRE].mtth_days,days)) fire_event(&cx,EVID_OEUVRE_NOIRE,c);
        if (EVENTS[EVID_SAVOIR_INTERDIT].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_SAVOIR_INTERDIT].mtth_days,days)) fire_event(&cx,EVID_SAVOIR_INTERDIT,c);
        if (EVENTS[EVID_CULTE_IMPERIAL].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_CULTE_IMPERIAL].mtth_days,days)) fire_event(&cx,EVID_CULTE_IMPERIAL,c);
        if (EVENTS[EVID_EVEIL].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_EVEIL].mtth_days,days)) fire_event(&cx,EVID_EVEIL,c);
        if (EVENTS[EVID_DROIT_INTEGRATION].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_DROIT_INTEGRATION].mtth_days,days)) fire_event(&cx,EVID_DROIT_INTEGRATION,c);
        if (EVENTS[EVID_DIASPORA_COMPTOIRS].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_DIASPORA_COMPTOIRS].mtth_days,days)) fire_event(&cx,EVID_DIASPORA_COMPTOIRS,c);
        if (EVENTS[EVID_FOI_FENDRE].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_FOI_FENDRE].mtth_days,days)) fire_event(&cx,EVID_FOI_FENDRE,c);
        if (EVENTS[EVID_PROPHETE_BRECHE].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_PROPHETE_BRECHE].mtth_days,days)) fire_event(&cx,EVID_PROPHETE_BRECHE,c);
        if (EVENTS[EVID_FUSILS_REVIENNENT].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_FUSILS_REVIENNENT].mtth_days,days)) fire_event(&cx,EVID_FUSILS_REVIENNENT,c);
        if (EVENTS[EVID_SAVANTS_ENNEMI].trigger(&cx,c) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_SAVANTS_ENNEMI].mtth_days,days)) fire_event(&cx,EVID_SAVANTS_ENNEMI,c);
    }

    /* 2septies. V2b — Merveille (LOT 1) + Conseil (LOT 2) + contenu débloqué (LOT 3).
     * TOUS human-only par construction (leur trigger vérifie c==human_player) : sur
     * human_player=-1 (chronique) le trigger renvoie faux systématiquement — même
     * court-circuit (trigger && frand) que le reste du module, golden intact PAR
     * CONSTRUCTION (aucun tirage tant qu'aucun joueur n'existe). ⚠ EXEMPTÉS du
     * plafond mondial 3-5 (EV_MAX_FIRES=0 dans la table — cf. EV_CAPPED plus bas) :
     * personnels et récurrents par nature, leurs cooldowns propres (540-2555 j)
     * suffisent — un plafond mondial n'a pas de sens sur un dilemme qui ne concerne
     * QUE le joueur (les dilemmes du monde entier, eux, sont plafonnés). */
    if (EVENTS[EVID_MERV_FONDATION].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_MERV_FONDATION].mtth_days,days)) fire_event(&cx,EVID_MERV_FONDATION,human_player);
    if (EVENTS[EVID_MERV_SACRIFICE].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_MERV_SACRIFICE].mtth_days,days)) fire_event(&cx,EVID_MERV_SACRIFICE,human_player);
    if (EVENTS[EVID_MERV_ASCENSION].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_MERV_ASCENSION].mtth_days,days)) fire_event(&cx,EVID_MERV_ASCENSION,human_player);

    if (EVENTS[EVID_TRAHISON_SAVOIR].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_TRAHISON_SAVOIR].mtth_days,days)) fire_event(&cx,EVID_TRAHISON_SAVOIR,human_player);
    if (EVENTS[EVID_TRAHISON_SOCIETE].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_TRAHISON_SOCIETE].mtth_days,days)) fire_event(&cx,EVID_TRAHISON_SOCIETE,human_player);
    if (EVENTS[EVID_TRAHISON_INDUSTRIE].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_TRAHISON_INDUSTRIE].mtth_days,days)) fire_event(&cx,EVID_TRAHISON_INDUSTRIE,human_player);
    if (EVENTS[EVID_CONSEIL_SUCCESSION].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_SUCCESSION].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_SUCCESSION,human_player);
    if (EVENTS[EVID_CONSEIL_R1].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_R1].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_R1,human_player);
    if (EVENTS[EVID_CONSEIL_R2].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_R2].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_R2,human_player);
    if (EVENTS[EVID_CONSEIL_R3].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_R3].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_R3,human_player);
    if (EVENTS[EVID_CONSEIL_A1].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_A1].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_A1,human_player);
    if (EVENTS[EVID_CONSEIL_A2].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_A2].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_A2,human_player);
    if (EVENTS[EVID_CONSEIL_C1].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_CONSEIL_C1].mtth_days,days)) fire_event(&cx,EVID_CONSEIL_C1,human_player);

    if (EVENTS[EVID_RIVAUX_VOISINS].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_RIVAUX_VOISINS].mtth_days,days)) fire_event(&cx,EVID_RIVAUX_VOISINS,human_player);
    if (EVENTS[EVID_PARENTE_LOINTAINE].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_PARENTE_LOINTAINE].mtth_days,days)) fire_event(&cx,EVID_PARENTE_LOINTAINE,human_player);
    if (EVENTS[EVID_TOLERANCE_CREDO].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_TOLERANCE_CREDO].mtth_days,days)) fire_event(&cx,EVID_TOLERANCE_CREDO,human_player);
    if (EVENTS[EVID_LETTRE_PERIME].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_LETTRE_PERIME].mtth_days,days)) fire_event(&cx,EVID_LETTRE_PERIME,human_player);
    if (EVENTS[EVID_PRATIQUE_DERIVE].trigger(&cx,human_player) &&
        frand(&ev->rng) < mtth_p(EVENTS[EVID_PRATIQUE_DERIVE].mtth_days,days)) fire_event(&cx,EVID_PRATIQUE_DERIVE,human_player);
    /* B6 (province) : scanne les régions du JOUEUR seulement (le trigger vérifie
     * l'owner via owner_ethos ; on filtre ici pour ne pas scanner tout le monde
     * inutilement sur un évènement qui ne concernera jamais un pays IA). */
    if (human_player>=0) for (int r=0;r<econ->n_regions;r++){
        if (econ->region[r].owner!=human_player) continue;
        if (EVENTS[EVID_MARCHE_ETHOS].trigger(&cx,r) &&
            frand(&ev->rng) < mtth_p(EVENTS[EVID_MARCHE_ETHOS].mtth_days,days)) fire_event(&cx,EVID_MARCHE_ETHOS,r);
    }

    /* 2bis. LE DIRECTEUR (§F) — lit la TEMPÉRATURE du monde, puis stabilise ou
     * déstabilise (sans jamais s'acharner). Cadence ~annuelle (sa propre échéance). */
    director_tick(&cx, days);

    /* 3. ÂGES — scan d'interprétation du monde. */
    { int last_before = ev->ages.last_dawned;
      events_check_ages(ev,w,econ,wp,wl,ts);
      /* ANNALES : un ÂGE est advenu ce tick (last_dawned a changé) → un fait de poids
       * PLEIN (70), accroché au joueur seulement (le récit DU joueur, monde entier). */
      if (human_player>=0 && ev->ages.last_dawned!=last_before && ev->ages.last_dawned>=0)
          annal_push(ev, ev->ages.days_elapsed/365, ANNAL_AGE, ev->ages.last_dawned, 0, -1, 70, -1);
    }

    /* Résorption TRANSITOIRE de l'amplification structurelle : la crise aiguë
     * retombe à mesure que les pays tranchent (réforme / poigne / effondrement)
     * — résolution émergente, pas un minuteur. Les acquis (palier, credo) restent.
     * AGE_STRUCTURAL_DECAY_DAY (registre J, raccord « jitter ») remplace le 0,0004
     * codé en dur. age_P_bonus/age_mig_mult/age_research_mult (Échanges/Découvertes,
     * TRANSITOIRES) suivent le MÊME rythme ; age_integration_mult (Empires) et
     * age_tech_mask ne décroissent JAMAIS (PERMANENTS, spec verbatim). */
    if (wp){
        float k = clampf(tune_f("AGE_STRUCTURAL_DECAY_DAY",0.00015f)*(float)days, 0.f, 1.f);
        wp->age_I_bonus         -= wp->age_I_bonus         * k;
        wp->age_lumiere_solvent -= wp->age_lumiere_solvent * k;
        wp->age_L_penalty       -= wp->age_L_penalty       * k;
        wp->age_H_bonus         -= wp->age_H_bonus         * k;
        wp->age_myth_homogen    -= wp->age_myth_homogen    * k;
        wp->age_P_bonus         -= wp->age_P_bonus         * k;
        wp->age_mig_mult        -= (wp->age_mig_mult-1.f)  * k;   /* décroît VERS 1, pas vers 0 */
        wp->age_research_mult   -= (wp->age_research_mult-1.f) * k;
    }

    /* MEMBRANE DE DÉCISION — un pending qui a expiré (180 j sans choix du joueur)
     * s'auto-résout (ai_chance), comme l'IA l'aurait tranché. */
    pending_event_tick_expire(ev,w,econ,wl,wp,sc,rn,ts,dp,eg, ev->ages.days_elapsed);
}

/* ===================================================================== */
/* GARDE-FOU MEMBRANE — aucun nom SCPS dans les textes joueur            */
/* ===================================================================== */
bool events_text_clean(void){
    static const char *BANNED[]={ "SCPS","D∞","∞","K_inst","H_coerc","P_realise",
        "fragilit","fractur","flux_faustien","age_C","breach_flux","D_bar", NULL };
    /* CONTENU W2 (lot 2) : buffer DIMENSIONNÉ au pire cas — EVID_COUNT évènements,
     * chacun jusqu'à 1 nom + 4 options × 3 textes (label/blurb/flavor) + AGE_COUNT
     * noms d'âge. Le fixe 256 (legs W1) débordait dès EVID_COUNT≈20 (256/13≈19) —
     * ce lot ajoute 16 évènements de plus (37 au total), largement au-delà : un
     * dépassement RÉEL de tableau (stack smashing détecté par le canari), pas un
     * faux positif. */
    const char *texts[EVID_COUNT*(1+4*3) + AGE_COUNT]; int n=0;
    for (int i=0;i<EVID_COUNT;i++){
        texts[n++]=EVENTS[i].name;
        for (int o=0;o<EVENTS[i].n_options;o++){
            texts[n++]=EVENTS[i].options[o].label; texts[n++]=EVENTS[i].options[o].blurb;
            texts[n++]=EVENTS[i].options[o].flavor;   /* MEMBRANE DE DÉCISION : le tooltip du choix aussi */
        }
    }
    for (int a=0;a<AGE_COUNT;a++) texts[n++]=AGE_NAMES[a];
    for (int i=0;i<n;i++){
        const char *s=texts[i]; if(!s) continue;
        for (int b=0;BANNED[b];b++) if (strstr(s,BANNED[b])) return false;
    }
    return true;
}
