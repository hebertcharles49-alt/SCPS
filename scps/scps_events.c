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
#include "scps_tune.h"
#include "scps_math.h"      /* clampf/absf/xs32/frand partagés */
#include "scps_provlog.h"   /* journal provincial : on POUSSE les évènements EV_PROVINCE (display) */
#include "scps_factions.h"  /* MEMBRANE DE DÉCISION : faction_lever_apply/faction_concede (hooks de choix) */
#include "scps_religion.h"  /* CONTENU W2 (lot 2) §C : religion_schism_eligible (C1) */
#include "scps_agency.h"    /* CONTENU W2 (lot 2) §C : Edifice (EDI_SANCTUAIRE/TEMPLE/CATHEDRALE, C6) */
#include "scps_demography.h" /* ESCLAVAGE (A1) : demography_manumit_country (choix « Abolir ») */
#include "scps_lang.h"      /* lot M : tr() — le NOM du ministre (maison V2a) dans les titres de trahison */
/* V2b LOT 2 relit statecraft_council_* — déjà visible via scps_statecraft.h (inclus
 * par scps_events.h) ; LOT 1 relit endgame_* — déjà visible via scps_endgame.h (idem). */

/* MEMBRANE DE DÉCISION — TÉLÉMÉTRIE (« la télémétrie est la preuve d'équilibre ») : combien
 * de fois la crise phare (et sa suite conséquente) ont tiré sur l'ensemble d'un run. Statics
 * de MODULE, RAZ à events_init (par partie/sim, comme g_wild_spawned), PAS sérialisés — la
 * chronique les lit pour son bilan (résolu par IA OU joueur, les deux passent par resolve_choice). */
static long g_marbrive_fired=0, g_pont_effondre_fired=0;
long events_marbrive_fired(void){ return g_marbrive_fired; }
long events_pont_effondre_fired(void){ return g_pont_effondre_fired; }

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

#define AGE_DAWN_YEARS 20   /* l'âge de l'Aube (base) dure les 20 premières années */
#define AGE_MIN_YEARS  30   /* puis un âge par génération (30 ans) entre avènements */

static void events_fire_caps_seed(EventsState *ev, uint32_t seed);  /* défini avec fire_event */
void events_init(EventsState *ev, const World *w, uint32_t seed){
    memset(ev,0,sizeof(*ev));
    ev->rng = seed ? seed : 0xA17F23C5u;
    events_fire_caps_seed(ev, ev->rng);   /* PLAFOND DE TIRS À VIE : 3-5 par évènement plafonné */
    ev->ages.research_mult = 1.f;
    ev->ages.integration_mult = 1.f;
    ev->ages.last_dawned = -1;
    /* L'Aube dure 20 ans : le 1er âge ne peut s'éveiller avant l'an 20
     * (gate : an ≥ last_dawn_year + 30, donc last init = 20 − 30 = −10). */
    ev->ages.last_dawn_year = AGE_DAWN_YEARS - AGE_MIN_YEARS;
    ev->last_id = -1; ev->last_name = NULL;
    g_marbrive_fired=0; g_pont_effondre_fired=0;   /* MEMBRANE DE DÉCISION : télémétrie RAZ par sim */
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
    [EVID_QUAKE] = { EVID_QUAKE, EV_PROVINCE, "La terre tremble", trig_quake, 8000.f, NULL,
        {{ "—", "Le sol se déchire : édifices effondrés, vies fauchées, ordre ébranlé.",
           { .d_K_inst=-1.5f, .d_agitation=12.f, .pop_mult=0.90f, .d_treasury=-20.f, .unlock_branch=-1 }, 1.f }}, 1 },
    [EVID_FLOOD] = { EVID_FLOOD, EV_PROVINCE, "Les eaux débordent", trig_flood, 5000.f, NULL,
        {{ "—", "La crue emporte des récoltes — mais le limon laisse une terre plus grasse.",
           { .d_food_cap=1.5f, .d_agitation=6.f, .pop_mult=0.97f, .d_treasury=-10.f, .unlock_branch=-1 }, 1.f }}, 1 },
    [EVID_DROUGHT] = { EVID_DROUGHT, EV_PROVINCE, "La sécheresse s'installe", trig_drought, 6000.f, NULL,
        {{ "—", "La pluie se refuse ; les greniers se vident et la colère monte.",
           { .d_food_cap=-1.5f, .d_agitation=15.f, .pop_mult=0.98f, .d_treasury=-10.f, .unlock_branch=-1 }, 1.f }}, 1 },
    [EVID_FIRE] = { EVID_FIRE, EV_PROVINCE, "Le feu court sur la forêt", trig_fire, 7000.f, NULL,
        {{ "—", "Les flammes ravagent les bois — et ouvrent, de force, des terres à la charrue.",
           { .d_food_cap=0.8f, .d_agitation=5.f, .pop_mult=0.97f, .unlock_branch=-1 }, 1.f }}, 1 },
    [EVID_PLAGUE] = { EVID_PLAGUE, EV_PROVINCE, "La peste remonte les routes", NULL, 12000.f, NULL,
        {{ "—", "Le mal voyage avec les marchands : il frappe d'autant plus fort les grands carrefours ouverts.",
           { .d_agitation=18.f, .pop_mult=0.78f, .d_treasury=-15.f, .unlock_branch=-1 }, 1.f }}, 1 },

    /* ---- Intégration des marches : MÊME état, quatre récits par la fiche ---- */
    [EVID_INTEG_DOMINATEUR] = { EVID_INTEG_DOMINATEUR, EV_PROVINCE, "Les marges osent gronder",
        trig_integ_dom, 900.f, NULL, {
        { "Mater dans le sang", "On brise la révolte par la terreur : l'ordre revient, mais la haine reste.",
          { .d_H_coerc=2.0f, .d_L=-1.0f, .d_coercion=0.5f, .d_agitation=-40.f, .unlock_branch=-1 }, 0.7f },
        { "Lever une horde punitive", "Une chevauchée de représailles ravage les insolents.",
          { .d_H_coerc=1.0f, .d_agitation=-22.f, .pop_mult=0.98f, .unlock_branch=-1 }, 0.3f } }, 2 },
    [EVID_INTEG_MERCANTILE] = { EVID_INTEG_MERCANTILE, EV_PROVINCE, "Le comptoir réclame son autonomie",
        trig_integ_merc, 900.f, NULL, {
        { "Affranchir le commerce", "On lâche du contrôle contre la paix : la cité respire et s'enrichit.",
          { .d_food_cap=0.6f, .d_L=0.5f, .d_coercion=-0.3f, .d_agitation=-30.f, .unlock_branch=-1 }, 0.6f },
        { "Acheter les meneurs", "Quelques bourses bien placées éteignent la fronde.",
          { .d_treasury=-50.f, .d_agitation=-25.f, .unlock_branch=-1 }, 0.4f } }, 2 },
    [EVID_INTEG_BUREAUCRATE] = { EVID_INTEG_BUREAUCRATE, EV_PROVINCE, "Une province mal arrimée",
        trig_integ_bur, 900.f, NULL, {
        { "Réforme d'intégration", "On dépêche des fonctionnaires : lent, mais le droit finit par lier.",
          { .d_L=1.5f, .d_agitation=-20.f, .unlock_branch=-1 }, 0.65f },
        { "Garnison", "Des baïonnettes tiennent ce que le droit n'a pas encore lié.",
          { .d_H_coerc=1.5f, .d_agitation=-15.f, .unlock_branch=-1 }, 0.35f } }, 2 },
    [EVID_INTEG_ANCIEN] = { EVID_INTEG_ANCIEN, EV_PROVINCE, "La longue mémoire des conquis",
        trig_integ_anc, 900.f, NULL, {
        { "Patience des siècles", "On ne fait rien d'éclatant : le temps, lentement, ploie les mémoires.",
          { .d_L=0.3f, .d_agitation=-10.f, .unlock_branch=-1 }, 0.6f },
        { "Concession rituelle", "Un geste sacré apaise — au prix d'un peu de prestige.",
          { .d_L=0.8f, .d_influence=-5.f, .d_agitation=-12.f, .unlock_branch=-1 }, 0.4f } }, 2 },

    /* ---- Génériques (légitimité / credo) ---- */
    [EVID_SUCCESSION] = { EVID_SUCCESSION, EV_COUNTRY, "La couronne vacante",
        trig_succession, 6000.f, NULL, {
        { "Transition ordonnée", "Les grands s'accordent : la passation se fait sans verser le sang.",
          { .d_L=0.6f, .d_influence=3.f, .unlock_branch=-1 }, 0.5f },
        { "Régence contestée", "Deux partis se disputent le trône ; le royaume retient son souffle.",
          { .d_L=-1.0f, .d_agitation=15.f, .unlock_branch=-1 }, 0.5f } }, 2 },
    [EVID_SCHISM] = { EVID_SCHISM, EV_COUNTRY, "Une idéologie se déchire",
        trig_schism, 7000.f, NULL, {
        { "Tolérer les deux courants", "On laisse coexister les doctrines : la paix, au prix de l'unité.",
          { .d_agitation=-10.f, .d_L=-0.3f, .unlock_branch=-1 }, 0.5f },
        { "Imposer l'orthodoxie", "Une seule doctrine, par la force s'il le faut — et la dissidence gronde.",
          { .d_H_coerc=1.0f, .d_coercion=0.3f, .d_agitation=10.f, .unlock_branch=-1 }, 0.5f } }, 2 },
    /* ---- Floraison cosmopolite : le creuset qui réussit (positif, par l'éthos) ---- */
    [EVID_XENOPHILE] = { EVID_XENOPHILE, EV_PROVINCE, "Le creuset des peuples",
        trig_xenophile, 2400.f, NULL, {
        { "Célébrer la concorde", "Tant de peuples sous un même toit, et la paix tient : les talents affluent, "
          "les comptoirs prospèrent, et le renom de la couronne tolérante porte au loin.",
          { .d_L=1.2f, .d_food_cap=0.5f, .d_treasury=120.f, .d_influence=4.f, .pop_mult=1.02f, .unlock_branch=-1 }, 1.f,
          /* V2b LOT 3 (hook Communautaire) : le peuple qu'on épargne — la concorde
           * COSMOPOLITE célébrée est, par nature, un vote pour le bien-commun. */
          { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.08f, .scar_kind=SCAR_NONE, .cooldown_days=0 } } }, 1 },
    /* ---- Miroir xénophobe : la cohésion du creuset DIGÉRÉ (positif pour l'éthos martial) ---- */
    [EVID_XENOPHOBE] = { EVID_XENOPHOBE, EV_PROVINCE, "Le creuset digéré",
        trig_xenophobe, 3000.f, NULL, {
        { "Sceller l'unité", "Les peuples conquis se sont fondus dans le creuset du vainqueur : un seul "
          "sang désormais, une seule loi — la cohésion farouche de qui a tout digéré tient sans effort.",
          { .d_L=1.0f, .d_H_coerc=0.5f, .d_agitation=-15.f, .d_influence=3.f, .unlock_branch=-1 }, 1.f } }, 1 },

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
          "Le fouet ne débat pas : il conclut.",
          { .d_H_coerc=0.6f, .d_L=-0.4f, .d_coercion=0.22f, .d_agitation=-12.f, .d_treasury_mois=-0.15f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.f, .scar_kind=SCAR_SABOTAGE_CHANTIER, .cooldown_days=540 },
          "Une garnison de prévôts mate la contestation — l'ordre revient, mais quelqu'un s'en souviendra." },
        { "Payer double jusqu'aux pluies",
          "L'or achète ce que la peur n'obtient pas : un peu de patience.",
          { .d_L=0.3f, .d_coercion=-0.10f, .d_agitation=-8.f, .d_treasury_mois=-0.6f, .unlock_branch=-1 },
          0.7f, { .faction=FAC_MARCHAND, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Une subvention généreuse — les Marchands s'en souviennent, et en tirent une dette d'obéissance." },
        { "Reporter et ouvrir une enquête",
          "Gagner du temps, en espérant qu'il joue pour soi.",
          { .d_K_inst=0.2f, .d_L=0.1f, .d_agitation=4.f, .d_treasury_mois=-0.25f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Une commission d'enquête — la contestation continue de couver, mais au moins on l'étudie." } }, 3 },

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
          "Trouver les coupables avant qu'ils ne recommencent.",
          { .d_H_coerc=0.4f, .d_coercion=0.15f, .d_agitation=-6.f, .d_treasury_mois=-0.2f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "L'enquête retrouve les responsables — un exemple est fait, la crainte s'installe." },
        { "Reconstruire, en payant cette fois",
          "Refaire à neuf, sans lésiner — la leçon a porté.",
          { .d_K_inst=0.5f, .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-0.7f, .unlock_branch=-1 },
          0.7f, { .faction=FAC_LEGISTE, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "Le chantier renaît, mieux bâti — coûteux, mais la province s'en souvient favorablement." },
        { "Abandonner la route",
          "Ce que le sabotage a pris, on ne le redonnera pas.",
          { .d_K_inst=-0.4f, .d_agitation=6.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=540 },
          "La route reste coupée — un renoncement qui se voit, et qui se paie en institutions perdues." } }, 3 },

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
          "L'impôt se lève, cloches ou pas.",
          { .d_L=-0.4f, .d_H_coerc=0.4f, .d_coercion=0.18f, .d_agitation=10.f, .d_treasury_mois=0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_DETTE_OBEISSANCE, .cooldown_days=720 },
          "Un impôt qu'on paie en silence se rembourse un jour en cris." },
        { "Accorder une remise d'une année",
          "Un an de grâce — le temps que la colère retombe.",
          { .d_L=0.5f, .d_coercion=-0.15f, .d_agitation=-10.f, .d_treasury_mois=-1.0f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : épargner le peuple d'une surtaxe — le vote
           * évident du bien-commun, cohérent avec l'ai_chance déjà la plus haute (0.7). */
          0.7f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.06f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Une remise généreuse — et peut-être la gratitude d'un peuple qui s'en souvient.",
          { .pop_mult=1.004f, .unlock_branch=-1 }, 0.3f },
        { "Acheter les notables",
          "Quelques bourses, discrètement, aux bonnes personnes.",
          { .d_L=0.2f, .d_agitation=-8.f, .d_treasury_mois=-0.6f, .d_influence=-2.f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Les notables se taisent, achetés — la paix a un prix, et il est discret." } }, 3 },

    /* ENTREPÔTS FERMÉS — le grief marchand ferme boutique. */
    [EVID_ENTREPOTS_FERMES] = { EVID_ENTREPOTS_FERMES, EV_PROVINCE, "Les entrepôts de %s ferment sans édit",
        trig_entrepots_fermes, 720.f, NULL, {
        { "Accorder une charte de transit",
          "Le passage s'ouvre, par écrit et pour de bon.",
          { .d_L=0.4f, .d_coercion=-0.12f, .d_agitation=-8.f, .d_treasury_mois=-0.45f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.6f, { .faction=FAC_MARCHAND, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Une charte de transit — les Marchands rouvrent, et n'oublient pas qui l'a signée." },
        { "Réquisitionner les entrepôts",
          "L'État prend ce que les marchands referment.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_coercion=0.2f, .d_agitation=6.f, .d_treasury_mois=0.3f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RANCUNE_MARCHANDE, .cooldown_days=720 },
          "L'État réquisitionne — les caisses se remplissent, la rancune marchande aussi." },
        { "Escorter les convois concurrents",
          "Une garde d'honneur pour qui ose encore commercer.",
          { .d_H_coerc=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.75f, .d_influence=2.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Une escorte armée rouvre la route — et si la ruse marchande perce, tant mieux.",
          { .d_C_global=0.08f, .unlock_branch=-1 }, 0.35f } }, 3 },

    /* DEUX CARTES — une conquête récente, deux frontières. */
    [EVID_DEUX_CARTES] = { EVID_DEUX_CARTES, EV_PROVINCE, "Deux cartes, deux frontières à %s",
        trig_deux_cartes, 720.f, NULL, {
        { "Imposer la carte de la capitale",
          "Une seule frontière compte : celle du trône.",
          { .d_K_inst=0.3f, .d_L=-0.4f, .d_coercion=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "La carte de la capitale s'impose — le terrain, lui, gardera sa propre mémoire." },
        { "Arbitrer sur place",
          "Un arbitre local tranche, au cas par cas.",
          { .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.5f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Un arbitrage patient — les deux cartes finissent, lentement, par se recouper." },
        { "Laisser l'ambiguïté rapporter",
          "Deux frontières, deux péages — pourquoi choisir ?",
          { .d_L=-0.3f, .d_agitation=8.f, .d_treasury_mois=0.35f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EXEMPTION_ACHETEE, .cooldown_days=720 },
          "Le double péage rapporte — tant que personne ne le remarque trop fort.",
          { .d_agitation=8.f, .unlock_branch=-1 }, 0.25f } }, 3 },

    /* EAU NOIRE — le puits vire noir, la Brèche s'invite. */
    [EVID_EAU_NOIRE] = { EVID_EAU_NOIRE, EV_PROVINCE, "Le puits de %s donne une eau noire",
        trig_eau_noire, 1095.f, NULL, {
        { "Fermer le puits",
          "On mure le puits — la prudence avant tout.",
          { .d_food_cap=-0.3f, .d_L=0.4f, .d_agitation=-6.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Le puits muré ne parlera plus — et ce qu'il portait reste, prudemment, sous terre." },
        { "Laisser les savants prélever",
          "La curiosité l'emporte sur la prudence.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.08f, .d_treasury_mois=-0.45f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Les savants prélèvent, fascinés — la Brèche s'invite un peu plus dans le grenier.",
          { .d_breach=-0.04f, .d_influence=1.f, .unlock_branch=-1 }, 0.3f },
        { "Vendre l'eau comme remède",
          "Un peu de mystère, beaucoup de bagout, et ça se vend.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_treasury_mois=0.5f, .d_breach=0.05f, .pop_mult=0.997f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1095 },
          "L'eau noire se vend comme remède — un scandale sanitaire qui couve, pour plus tard." } }, 3 },

    /* DERNIÈRE DÉCISION — le passé n'a pas fini d'arriver. */
    [EVID_DERNIERE_DECISION] = { EVID_DERNIERE_DECISION, EV_PROVINCE, "À %s, la dernière décision n'a pas fini d'arriver",
        trig_derniere_decision, 360.f, NULL, {
        { "Corriger publiquement",
          "On revient dessus, en public, avant que ça n'empire.",
          { .d_L=0.4f, .d_coercion=-0.15f, .d_agitation=-8.f, .d_treasury_mois=-0.75f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "Une correction publique — la décision passée n'ira pas plus loin." },
        { "Laisser les vagues mourir",
          "On ne touche à rien ; ça finira bien par se tasser.",
          { .d_L=-0.15f, .d_agitation=5.f, .pop_mult=0.999f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "On laisse porter — parfois les vagues meurent seules, sans qu'on lève un doigt.",
          { .d_agitation=-6.f, .unlock_branch=-1 }, 0.4f },
        { "Achever la décision par la force",
          "Ce qui couve, on l'écrase avant que ça n'éclate.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.2f, .d_agitation=-6.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_CONQUERANT, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=360 },
          "La force achève la décision passée — mais ce qu'elle couvait mûrira plus fort, pas plus doux." } }, 3 },

    /* SALVE RUNIQUE — la première fois que le feu runique parle (pays, unique). */
    [EVID_SALVE_RUNIQUE] = { EVID_SALVE_RUNIQUE, EV_COUNTRY, "La première salve runique",
        trig_salve_runique, 3650.f, NULL, {
        { "Doctriner l'usage",
          "On encadre l'arme neuve — un usage, pas un désordre.",
          { .d_K_inst=0.4f, .d_L=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.6f, .d_breach=-0.02f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_LEGISTE, .faction_strength=0.03f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Une doctrine d'emploi — l'arme runique sert l'ordre, encadrée." },
        { "Armer toutes les légions",
          "Chaque légion en reçoit — la gloire avant la prudence.",
          { .d_H_coerc=0.6f, .d_L=-0.4f, .d_agitation=8.f, .d_breach=0.10f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_CONQUERANT, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Toutes les légions s'arment de feu runique — la Brèche s'en trouve un peu plus proche." },
        { "Vendre le secret",
          "Un secret d'État se monnaie très cher, une seule fois.",
          { .d_agitation=6.f, .d_treasury_mois=2.0f, .d_influence=-3.f, .d_breach=0.15f, .unlock_branch=-1 },
          0.15f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_PROLIFERATION_ARME, .cooldown_days=36500 },
          "Le secret se vend, six mois de taxes d'un coup — mais un secret vendu s'ébruite.",
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
          "Une économie qui carbure aux chaînes, organisée, sans détour.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.18f, .d_treasury_mois=0.6f, .pop_mult=1.004f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_REVOLTE_SERVILE, .cooldown_days=1825 },
          "Institutionnalisée, l'économie servile rapporte — et couve sa propre révolte." },
        { "Limiter aux vaincus",
          "Seuls les vaincus d'hier — pas de zèle au-delà.",
          { .d_L=-0.15f, .d_coercion=0.10f, .d_treasury_mois=0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une pratique limitée, presque respectable — le trône s'en satisfait." },
        { "Abolir",
          "On referme ce que la tech a ouvert — la vertu, avant le profit.",
          { .d_L=0.5f, .d_agitation=-8.f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_CONQUERANT, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "L'abolition prive les Conquérants d'un levier qu'ils réclamaient.",
          { .d_influence=2.f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* A2 — L'ŒUVRE NOIRE NE S'ÉTEINT PAS LA NUIT (pays, L'Œuvre noire). */
    [EVID_OEUVRE_NOIRE] = { EVID_OEUVRE_NOIRE, EV_COUNTRY, "L'Œuvre noire ne s'éteint pas la nuit",
        trig_oeuvre_noire, 1825.f, NULL, {
        { "Sceller",
          "On scelle l'œuvre — l'effroi tenu sous clé.",
          { .d_L=0.5f, .d_agitation=-6.f, .d_treasury_mois=-0.6f, .d_breach=-0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Scellée, l'œuvre noire dort — pour l'instant." },
        { "Déployer aux frontières",
          "L'effroi tient l'ennemi à distance — un usage stratégique.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.10f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Déployée, l'œuvre glace les frontières — et rapproche la Brèche.",
          { .d_influence=2.f, .unlock_branch=-1 }, 0.4f },
        { "Disséminer",
          "Que chaque légion en porte un peu — la peur, partagée.",
          { .d_agitation=8.f, .d_L=-0.3f, .d_breach=0.16f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_PROLIFERATION_ARME, .cooldown_days=1825 },
          "Disséminée, l'œuvre noire échappe déjà à qui croyait la tenir." } }, 3 },

    /* A3 — LE SAVOIR INTERDIT TIENT SES PROMESSES (pays, Savoir interdit). */
    [EVID_SAVOIR_INTERDIT] = { EVID_SAVOIR_INTERDIT, EV_COUNTRY, "Le Savoir interdit tient ses promesses",
        trig_savoir_interdit, 1825.f, NULL, {
        { "École close",
          "Un cercle restreint, des murs épais — le savoir contenu.",
          { .d_K_inst=0.4f, .d_L=0.2f, .d_treasury_mois=-0.75f, .d_breach=0.04f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une école close — le savoir interdit sert, discrètement." },
        { "Exploiter sans le dire",
          "On s'en sert, en silence — le silence ne tient jamais longtemps.",
          { .d_L=-0.3f, .d_agitation=6.f, .d_breach=0.12f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.20f, .scar_kind=SCAR_FUITE_CERVEAUX, .cooldown_days=1825 },
          "Exploité sans le dire — et le secret fuit, comme toujours.",
          { .d_agitation=6.f, .unlock_branch=-1 }, 0.35f },
        { "Bannir et brûler",
          "Ce savoir-là ne servira personne — pas même nous.",
          { .d_L=-0.2f, .d_H_coerc=0.3f, .d_coercion=0.15f, .d_agitation=6.f, .d_breach=-0.06f, .unlock_branch=-1 },
          0.15f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_FUITE_CERVEAUX, .cooldown_days=1825 },
          "Banni et brûlé — mais des savants ont déjà fui avec ce qu'ils savaient." } }, 3 },

    /* A4 — LE TRÔNE EST DEVENU UN AUTEL (pays, Culte impérial). */
    [EVID_CULTE_IMPERIAL] = { EVID_CULTE_IMPERIAL, EV_COUNTRY, "Le trône est devenu un autel",
        trig_culte_imperial, 1825.f, NULL, {
        { "Imposer le culte",
          "Le trône ET l'autel, une seule voix — celle du souverain.",
          { .d_K_inst=0.3f, .d_H_coerc=0.4f, .d_L=0.4f, .d_coercion=0.15f, .d_agitation=-10.f, .unlock_branch=-1 },
          0.6f, { .faction=FAC_GARDIEN, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le culte impérial s'impose — les Gardiens y voient une victoire." },
        { "Cohabiter",
          "Le trône et l'autel, côte à côte, sans se fondre.",
          { .d_L=0.2f, .d_agitation=-6.f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Une cohabitation prudente — ni victoire, ni défaite pour personne." },
        { "Y renoncer",
          "Le trône reste un trône — l'autel, une affaire distincte.",
          { .d_L=-0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le renoncement surprend — l'humilité, parfois, désarme la critique.",
          { .d_L=0.3f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* A5 — QUELQUE CHOSE S'EST ÉVEILLÉ DANS LES GLYPHES (pays, L'Éveil). */
    [EVID_EVEIL] = { EVID_EVEIL, EV_COUNTRY, "Quelque chose s'est éveillé dans les glyphes",
        trig_eveil, 1825.f, NULL, {
        { "L'atteler à la guerre",
          "Une armée qui ne mange pas, ne dort pas — la tentation est trop grande.",
          { .d_H_coerc=0.5f, .d_L=-0.3f, .d_agitation=8.f, .d_breach=0.12f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Attelé à la guerre, l'Éveil sert — et la Brèche s'en rapproche." },
        { "Vase clos",
          "On l'étudie, on le contient — la curiosité, mesurée.",
          { .d_K_inst=0.4f, .d_treasury_mois=-0.75f, .d_breach=0.05f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "En vase clos, l'Éveil laisse parfois percer une découverte.",
          { .d_influence=2.f, .d_breach=-0.02f, .unlock_branch=-1 }, 0.3f },
        { "Refermer",
          "On referme ce qu'on n'aurait pas dû ouvrir.",
          { .d_L=0.3f, .d_agitation=-6.f, .d_treasury_mois=-0.5f, .d_breach=-0.08f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EVEIL_SOMMEIL, .cooldown_days=1825 },
          "Refermé — mais ce qui dort se souvient, et remuera un jour." } }, 3 },

    /* A6 — LA FOREUSE MORD DANS QUELQUE CHOSE QUI SAIGNE (province, Foreuse arcanique bâtie ICI). */
    [EVID_FOREUSE_SAIGNE] = { EVID_FOREUSE_SAIGNE, EV_PROVINCE, "À %s, la foreuse mord dans quelque chose qui saigne",
        trig_foreuse_saigne, 1825.f, NULL, {
        { "Plein régime",
          "On ne s'arrête pas pour si peu — la foreuse continue.",
          { .d_agitation=10.f, .d_breach=0.12f, .d_treasury_mois=0.8f, .pop_mult=0.998f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1825 },
          "Plein régime — la foreuse mord, rapporte, et couve un scandale sanitaire." },
        { "Sous relevés",
          "On note tout, discrètement — le sang est une donnée, aussi.",
          { .d_K_inst=0.3f, .d_treasury_mois=0.4f, .d_breach=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Sous relevés, la foreuse rapporte prudemment — et parfois révèle un filon.",
          { .d_treasury_mois=0.4f, .unlock_branch=-1 }, 0.35f },
        { "Reboucher",
          "On rebouche — ce que ça saignait ne regarde personne d'autre.",
          { .d_L=0.4f, .d_treasury_mois=-0.6f, .d_breach=-0.04f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Rebouchée, la foreuse se tait — la mine, elle, garde son secret." } }, 3 },

    /* B1 — LE DROIT D'INTÉGRATION DIVISE CEUX QU'IL UNIT (pays, off-culture > 25 %). */
    [EVID_DROIT_INTEGRATION] = { EVID_DROIT_INTEGRATION, EV_COUNTRY, "Le droit d'intégration divise ceux qu'il unit",
        trig_droit_integration, 1460.f, NULL, {
        { "Forcer l'assimilation",
          "Une langue, une loi, un peuple — de gré ou de force.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.18f, .d_agitation=8.f, .d_C_global=-0.02f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_FRACTURE_CULTURELLE, .cooldown_days=1460 },
          "L'assimilation forcée unit sur le papier — et divise en dessous." },
        { "Langue franque",
          "Un pont commun, sans effacer personne.",
          { .d_K_inst=0.4f, .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.75f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Une langue franque relie sans dissoudre — un pari mesuré." },
        { "Laisser les communautés",
          "Chacun sa loi, sa langue — l'unité viendra, ou pas.",
          { .d_L=0.2f, .d_agitation=-6.f, .d_C_global=0.03f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : le pluralisme qui laisse chacun chez soi —
           * le vote le plus évident du bien-commun de ce dilemme. */
          0.3f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Le pluralisme laisse chacun chez soi — et parfois, ça paie.",
          { .pop_mult=1.004f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* B4 — LA DIASPORA TIENT LES COMPTOIRS (pays, diaspora > 15 %, comptoirs actifs). */
    [EVID_DIASPORA_COMPTOIRS] = { EVID_DIASPORA_COMPTOIRS, EV_COUNTRY, "La diaspora tient les comptoirs",
        trig_diaspora_comptoirs, 1460.f, NULL, {
        { "Charte de protection",
          "On protège ceux qui font vivre le commerce.",
          { .d_L=0.2f, .d_agitation=6.f, .d_treasury_mois=0.3f, .d_C_global=0.05f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_MARCHAND, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Une charte de protection — les Marchands savent qui les a couverts." },
        { "Taxe spéciale",
          "Ils prospèrent ici : qu'ils paient un peu plus qu'ici.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_treasury_mois=0.5f, .pop_mult=0.998f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RANCUNE_MARCHANDE, .cooldown_days=1460 },
          "La taxe spéciale remplit les caisses — et sème une rancune marchande." },
        { "Expulser",
          "Le comptoir sans le comptoir : plus simple, en apparence.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_C_global=-0.10f, .pop_mult=0.985f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "L'expulsion vide les comptoirs — et le commerce, parfois, s'effondre avec eux.",
          { .d_treasury_mois=-1.0f, .unlock_branch=-1 }, 0.5f } }, 3 },

    /* C1 — LA FOI VA SE FENDRE (pays, schisme éligible : RUPTURE ou DÉRIVE). */
    [EVID_FOI_FENDRE] = { EVID_FOI_FENDRE, EV_COUNTRY, "La foi va se fendre",
        trig_foi_fendre, 1825.f, NULL, {
        { "Forcer l'unité",
          "Une seule foi, par la contrainte s'il le faut.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.15f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "L'unité forcée retient la brisure — pour un temps." },
        { "Laisser dériver",
          "Le courant divergent suivra sa pente, sans qu'on s'y oppose.",
          { .d_L=-0.15f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "On laisse dériver — la foi, elle, suivra son propre chemin." },
        { "Concile",
          "Réunir les doctrines autour d'une même table.",
          { .d_K_inst=0.4f, .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=-0.9f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_GARDIEN, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le concile cherche l'accord — et accouche parfois d'un dogme vivant.",
          { .d_L=0.3f, .unlock_branch=-1 }, 0.3f } }, 3 },

    /* C5 — LA BRÈCHE A TROUVÉ SON PROPHÈTE (pays, charge faustienne haute, Transgresseurs pas comblés). */
    [EVID_PROPHETE_BRECHE] = { EVID_PROPHETE_BRECHE, EV_COUNTRY, "La brèche a trouvé son prophète",
        trig_prophete_breche, 1460.f, NULL, {
        { "Livrer aux gardiens",
          "Le prophète appartient au bûcher, pas à la chaire.",
          { .d_L=0.2f, .d_H_coerc=0.3f, .d_coercion=0.15f, .d_agitation=8.f, .d_breach=-0.06f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=1460 },
          "Livré, le prophète se tait — mais ses fidèles, eux, se radicalisent." },
        { "L'écouter",
          "Ce qu'il dit de la Brèche mérite peut-être d'être entendu.",
          { .d_L=-0.3f, .d_agitation=8.f, .d_breach=0.14f, .unlock_branch=-1 },
          0.3f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "On l'écoute — et la Brèche se rapproche, un peu plus, à chaque mot." },
        { "Le récupérer",
          "Un prophète docile vaut mieux qu'un martyr.",
          { .d_K_inst=0.3f, .d_L=0.2f, .d_agitation=-6.f, .d_breach=0.05f, .d_influence=1.f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Récupéré, le prophète sert le trône — mais parfois déborde son rôle.",
          { .d_breach=0.06f, .unlock_branch=-1 }, 0.4f } }, 3 },

    /* C6 — LA RELIQUE FAIT DES MIRACLES DOUTEUX (province, édifice de foi + amplitude haute). */
    [EVID_RELIQUE_DOUTEUSE] = { EVID_RELIQUE_DOUTEUSE, EV_PROVINCE, "La relique de %s fait des miracles douteux",
        trig_relique_douteuse, 1095.f, NULL, {
        { "Authentifier",
          "L'Église tranche : le miracle est vrai, un point c'est tout.",
          { .d_L=0.4f, .d_agitation=-8.f, .d_treasury_mois=0.4f, .d_breach=0.06f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_SCANDALE_SANITAIRE, .cooldown_days=1095 },
          "Authentifiée, la relique attire les foules — et couve un scandale sanitaire." },
        { "Enquêter",
          "Avant de croire, on vérifie — la prudence d'abord.",
          { .d_K_inst=0.3f, .d_L=-0.15f, .d_treasury_mois=-0.4f, .d_breach=-0.03f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "L'enquête avance prudemment — et parfois prouve la fraude, noir sur blanc.",
          { .d_L=0.3f, .d_breach=-0.03f, .unlock_branch=-1 }, 0.35f },
        { "Interdire le pèlerinage",
          "On ferme la route avant que la foule ne s'y presse.",
          { .d_H_coerc=0.2f, .d_L=-0.3f, .d_coercion=0.12f, .d_agitation=8.f, .d_breach=-0.05f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1095 },
          "Le pèlerinage interdit déçoit — mais la relique cesse, au moins, de faire parler d'elle." } }, 3 },

    /* ═══ §D — CHAÎNAGE DE CICATRICES : les scars posées plus haut (§A/W1) mûrissent ici ═══ */

    /* K2 — LE REMÈDE FAIT DES MORTS (SCAR_SCANDALE_SANITAIRE mûrit — posée par EAU_NOIRE
     * "Vendre l'eau comme remède" ou RELIQUE_DOUTEUSE "Authentifier", délai 360-720 j
     * — scar_delay_range, voir apply_choice_hook plus bas dans le fichier). */
    [EVID_REMEDE_MORTS] = { EVID_REMEDE_MORTS, EV_PROVINCE, "Le remède de %s fait des morts",
        trig_remede_morts, 900.f, NULL, {
        { "Sacrifier les vendeurs",
          "Quelques têtes tombent — l'affaire est close, en apparence.",
          { .d_L=0.3f, .d_H_coerc=0.3f, .d_coercion=0.12f, .d_agitation=-8.f, .d_influence=-2.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Les vendeurs sacrifiés apaisent la foule — le trône, lui, perd un peu de lustre." },
        { "Indemniser",
          "Payer pour le mal fait — la seule réparation qui compte.",
          { .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-1.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "L'indemnisation coûte cher — et apaise, sincèrement, ceux qui restent." },
        { "Nier en bloc",
          "Il n'y a pas eu de remède, il n'y a pas eu de morts.",
          { .d_L=-0.3f, .d_agitation=8.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=720 },
          "Le déni tient, parfois — ailleurs, il nourrit une colère qui couve.",
          { .d_agitation=-6.f, .unlock_branch=-1 }, 0.5f } }, 3 },

    /* K3 — UNE CELLULE DANS LES FAUBOURGS (SCAR_RADICALISATION mûrit — posée par
     * PROPHETE_BRECHE "Livrer aux gardiens" ou REMEDE_MORTS "Nier en bloc"). */
    [EVID_CELLULE_FAUBOURGS] = { EVID_CELLULE_FAUBOURGS, EV_PROVINCE, "Une cellule dans les faubourgs de %s",
        trig_cellule_faubourgs, 900.f, NULL, {
        { "Rafle générale",
          "On ratisse large — la cellule tombe, avec du monde autour.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_coercion=0.20f, .d_agitation=-10.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_RADICALISATION, .cooldown_days=720 },
          "La rafle frappe fort — et sème, chez les innocents ratissés, une rancœur nouvelle." },
        { "Amnistie et emplois",
          "On offre une sortie plutôt qu'un cachot.",
          { .d_L=0.4f, .d_agitation=-10.f, .d_treasury_mois=-1.0f, .pop_mult=1.004f, .unlock_branch=-1 },
          /* V2b LOT 3 (hook Communautaire) : épargner des gens qu'on aurait pu rafler —
           * l'amnistie EST le vote du bien-commun de cet évènement. */
          0.5f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.07f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "L'amnistie désarme la colère mieux qu'un cachot ne l'aurait fait." },
        { "Infiltrer",
          "Patience — laisser le réseau se montrer avant de frapper.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "L'infiltration prend son temps — et parfois, le réseau entier tombe d'un coup.",
          { .d_agitation=-8.f, .unlock_branch=-1 }, 0.4f } }, 3 },

    /* K5 — NOS PROPRES FUSILS NOUS REVIENNENT (SCAR_PROLIFERATION_ARME mûrit — posée par
     * SALVE_RUNIQUE "Vendre le secret" ou OEUVRE_NOIRE "Disséminer", pays). */
    [EVID_FUSILS_REVIENNENT] = { EVID_FUSILS_REVIENNENT, EV_COUNTRY, "Nos propres fusils nous reviennent",
        trig_fusils_reviennent, 1460.f, NULL, {
        { "Surenchère",
          "On arme plus fort encore — la course continue.",
          { .d_agitation=8.f, .d_treasury_mois=-1.6f, .d_breach=0.08f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "La surenchère coûte cher — et rapproche, un peu plus, la Brèche." },
        { "Négocier le désarmement",
          "Une table plutôt qu'un champ de bataille.",
          { .d_L=-0.15f, .d_influence=-2.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Le désarmement négocié tient, parfois, mieux qu'on ne l'espérait.",
          { .d_influence=2.f, .d_breach=-0.05f, .unlock_branch=-1 }, 0.35f },
        { "Guerre préventive",
          "Frapper avant d'être frappé par sa propre arme.",
          { .d_H_coerc=0.5f, .d_L=-0.4f, .d_agitation=8.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 } } }, 3 },

    /* K6 — LES SAVANTS SONT PASSÉS À L'ENNEMI (SCAR_FUITE_CERVEAUX mûrit — posée par
     * SAVOIR_INTERDIT "Exploiter sans le dire" ou "Bannir et brûler", pays). */
    [EVID_SAVANTS_ENNEMI] = { EVID_SAVANTS_ENNEMI, EV_COUNTRY, "Les savants sont passés à l'ennemi",
        trig_savants_ennemi, 1095.f, NULL, {
        { "Rappeler à prix d'or",
          "L'or ramène ce que la fierté avait laissé filer.",
          { .d_K_inst=0.3f, .d_L=0.2f, .d_treasury_mois=-1.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Rappelés à prix d'or, les savants reviennent — la fierté un peu froissée." },
        { "Espionner",
          "Ce qu'on ne peut pas racheter, on le vole.",
          { .d_agitation=4.f, .d_treasury_mois=-0.6f, .d_breach=0.05f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "L'espionnage prend du temps — et parfois, le vol réussit pleinement.",
          { .d_K_inst=0.3f, .unlock_branch=-1 }, 0.4f },
        { "Renoncer à la branche",
          "Ce savoir-là, on ne le poursuivra plus.",
          { .d_L=0.2f, .d_C_global=-0.03f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 } } }, 3 },

    /* K7 — LES AUTRES VILLES ONT APPRIS LE TARIF (SCAR_EXEMPTION_ACHETEE mûrit — posée par
     * DEUX_CARTES "Laisser l'ambiguïté rapporter", PROVINCE (même clé de sujet que le
     * poseur — DEUX_CARTES est EV_PROVINCE, cf. note du trigger)). */
    [EVID_TARIF_APPRIS] = { EVID_TARIF_APPRIS, EV_PROVINCE, "Les autres villes ont appris le tarif de %s",
        trig_tarif_appris, 720.f, NULL, {
        { "Refuser toute exemption",
          "Plus personne ne passera entre les mailles.",
          { .d_L=0.2f, .d_H_coerc=0.3f, .d_coercion=0.12f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "Le refus ferme la faille — au prix d'un peu de grogne marchande." },
        { "Tarifer officiellement",
          "Ce qui se pratiquait sous le manteau devient la règle.",
          { .d_L=-0.3f, .d_agitation=-6.f, .d_treasury_mois=0.8f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_EXEMPTION_ACHETEE, .cooldown_days=720 },
          "Tarifée, l'exemption devient un revenu régulier — le marché s'auto-entretient." },
        { "Faire un exemple",
          "Un tarif exemplaire, appliqué à qui doutait encore.",
          { .d_H_coerc=0.4f, .d_L=-0.3f, .d_coercion=0.15f, .d_agitation=6.f, .d_treasury_mois=0.6f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=720 },
          "L'exemple fait grand bruit — et parfois, la leçon porte au-delà de l'espéré.",
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
    [EVID_MERV_FONDATION] = { EVID_MERV_FONDATION, EV_COUNTRY, "Le monde reconnaît une civilisation qui a tout digéré",
        trig_merv_fondation, 1200.f, NULL, {
        { "Fonder par la foi",
          "Ce que la conquête a pris, la foi le consacre — un temple à la mesure du monde entier.",
          { .d_K_inst=0.3f, .d_L=0.5f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "La foi fonde le chantier — les Gardiens s'en réjouissent, les Transgresseurs s'en méfient." },
        { "Fonder par la science",
          "Ce que la foi n'explique pas, la raison le mesure — et bâtit dessus.",
          { .d_K_inst=0.5f, .d_H_coerc=0.2f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "La science fonde le chantier — les Transgresseurs y voient leur heure, les Gardiens s'inquiètent." },
        { "Fonder par la force",
          "Ce qu'on n'a pas convaincu, on l'a soumis — le chantier se lève sur des fondations conquises.",
          { .d_H_coerc=0.5f, .d_L=-0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_CONQUERANT, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "La force fonde le chantier — les Conquérants exultent, les Communautaires grondent." } }, 3 },

    /* CONSTRUCTION — LES DILEMMES DE SACRIFICE : le chantier réclame, récurrent
     * tant que la Merveille est active. Trois voies : le trésor (mois), les
     * bras (pop+agitation), ou ralentir (petit malus, pas de sacrifice —
     * L descend un peu : un chantier qu'on freine perd de sa superbe). */
    [EVID_MERV_SACRIFICE] = { EVID_MERV_SACRIFICE, EV_COUNTRY, "Le chantier de la Merveille réclame",
        trig_merv_sacrifice, 1200.f, NULL, {
        { "Saigner le trésor",
          "L'or coule dans les fondations — le chantier avale sans rendre.",
          { .d_treasury_mois=-1.5f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Le trésor saigne — le chantier, lui, avance sans faiblir." },
        { "Saigner les bras",
          "On lève des corvées de plus — le peuple porte la pierre.",
          { .pop_mult=0.99f, .d_agitation=10.f, .unlock_branch=-1 },
          0.35f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Les bras saignent — le chantier avance sur le dos de ceux qui le portent." },
        { "Ralentir le chantier",
          "Mieux vaut un monument lent qu'un peuple épuisé.",
          { .d_L=-0.2f, .unlock_branch=-1 },
          0.25f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1200 },
          "Le chantier ralentit — la superbe en pâtit, mais personne n'en meurt." } }, 3 },

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
    [EVID_MERV_ASCENSION] = { EVID_MERV_ASCENSION, EV_COUNTRY, "Le dernier choix : ce que la Merveille achevée mérite",
        trig_merv_ascension, 720.f, NULL, {
        { "Activer",
          "La civilisation transcende — qu'il en soit ainsi.",
          { .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "L'Ascension s'active — le monde ne reverra plus cette civilisation, elle est passée ailleurs." },
        { "Refuser",
          "Le monument reste vide — mieux vaut rester mortel que de disparaître en dieu.",
          { .d_L=-0.6f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.20f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Le refus laisse le monument achevé, vide — un peuple qui a choisi de rester mortel." },
        { "Détruire",
          "Ce sommet-là, on ne le laissera à personne — on le brise soi-même.",
          { .d_L=-1.0f, .d_agitation=16.f, .unlock_branch=-1 },
          0.2f, { .faction=FAC_GARDIEN, .faction_strength=0.30f, .scar_kind=SCAR_NONE, .cooldown_days=36500 },
          "Ils ont touché le sommet et l'ont brisé — la cicatrice restera plus longtemps que le souvenir." } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * V2b LOT 2 — LE CONSEIL (signaux V2a). human gate (l'IA remplace déjà
     * son ministre au bord, cf. statecraft_council_ai_replace_count). Sujet =
     * le PAYS joueur (EV_COUNTRY) ; le siège concerné est retrouvé au moment
     * de la résolution (resolve_choice) par le même balayage que le trigger.
     * EXEMPTÉS du plafond mondial (personnels, cooldowns propres 1460-2555 j).
     * ═══════════════════════════════════════════════════════════════════ */

    /* Trahison — Savoir : le savant publie tes secrets. lot M : le %s de ces trois
     * gabarits = le MINISTRE assis (maison V2a, résolu par event_title — pas le pays). */
    [EVID_TRAHISON_SAVOIR] = { EVID_TRAHISON_SAVOIR, EV_COUNTRY, "%s publie les secrets du trône",
        trig_trahison_savoir, 1825.f, NULL, {
        { "Le faire taire",
          "Un procès discret, une plume qu'on brise avant qu'elle n'écrive davantage.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Fait taire, le savant se tait — mais ses pairs, eux, retiennent la leçon." },
        { "Le renvoyer sans bruit",
          "On referme le dossier — la trahison, elle, court toujours dans les gazettes.",
          { .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Renvoyé sans bruit — le silence coûte, mais évite le scandale d'un procès." },
        { "Faire un exemple public",
          "Toute la cour saura ce qu'il en coûte de vendre les secrets du trône.",
          { .d_L=0.3f, .d_agitation=8.f, .d_influence=-2.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_GARDIEN, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "L'exemple fait grand bruit — la cour retient la leçon, le monde retient le scandale." } }, 3 },

    /* Trahison — Société : le notable place ses familles. */
    [EVID_TRAHISON_SOCIETE] = { EVID_TRAHISON_SOCIETE, EV_COUNTRY, "%s a placé ses familles avant le trône",
        trig_trahison_societe, 1825.f, NULL, {
        { "Purger les places",
          "On reprend ce qui a été distribué — la loyauté se remérite.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La purge reprend les places — et laisse une cour rancunière." },
        { "Composer",
          "On laisse faire, contre un service rendu — la cour vit de ces arrangements.",
          { .d_L=-0.1f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "On compose — la faveur se rembourse un jour, d'une façon ou d'une autre." },
        { "Laisser faire",
          "Les familles s'installent — le trône regarde ailleurs.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le trône regarde ailleurs — les familles, elles, ne l'oublient pas." } }, 3 },

    /* Trahison — Industrie : le marchand détourne. */
    [EVID_TRAHISON_INDUSTRIE] = { EVID_TRAHISON_INDUSTRIE, EV_COUNTRY, "%s a détourné plus que sa part",
        trig_trahison_industrie, 1825.f, NULL, {
        { "Le poursuivre",
          "On récupère ce qu'on peut — le reste, la justice le tranchera.",
          { .d_treasury_mois=0.3f, .d_agitation=4.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_LEGISTE, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Poursuivi, le marchand rend une part — le reste a déjà changé de mains." },
        { "Négocier un remboursement",
          "Un accord discret vaut mieux qu'un procès qui traîne.",
          { .d_treasury_mois=0.15f, .unlock_branch=-1 },
          0.5f, { .faction=FAC_MARCHAND, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le remboursement discret arrange tout le monde — sauf ceux qui n'ont pas triché." },
        { "Fermer les yeux",
          "Le détournement continue — mais les routes, elles, restent ouvertes.",
          { .d_treasury_mois=-0.2f, .d_C_global=0.03f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_MARCHAND, .faction_strength=0.15f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Fermer les yeux coûte cher — mais le commerce, lui, ne s'en porte que mieux." } }, 3 },

    /* SUCCESSION — la retraite d'un loyal (>20 ans). Un moment, deux choix légers. */
    [EVID_CONSEIL_SUCCESSION] = { EVID_CONSEIL_SUCCESSION, EV_COUNTRY, "Un ministre loyal prend sa retraite",
        trig_conseil_succession, 1460.f, NULL, {
        { "Le remercier publiquement",
          "Une cérémonie, une pension — la loyauté se paie de reconnaissance.",
          { .d_L=0.3f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.6f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "La reconnaissance publique coûte peu — et se rappelle longtemps." },
        { "Le laisser partir sans bruit",
          "Une carrière s'achève — pas besoin d'en faire un événement.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1460 },
          "Il part sans bruit — le siège, lui, attend son successeur." } }, 2 },

    /* R1 — Savoir vs Société en RIVALITÉ : trancher pour l'un/l'autre/renvoyer les deux. */
    [EVID_CONSEIL_R1] = { EVID_CONSEIL_R1, EV_COUNTRY, "Le Savoir et la Société se disputent l'oreille du trône",
        trig_conseil_r1, 1825.f, NULL, {
        { "Trancher pour le Savoir",
          "L'érudition l'emporte — la Société ravale sa rancœur.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le Savoir gagne l'arbitrage — la Société retient, un peu plus fort, sa rancœur." },
        { "Trancher pour la Société",
          "Les familles l'emportent — le Savoir ravale son orgueil.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_CONQUERANT, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La Société gagne l'arbitrage — le Savoir retient, un peu plus fort, son orgueil." },
        { "Renvoyer les deux",
          "Si ni l'un ni l'autre ne sait se taire, qu'ils partent ensemble.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Deux sièges vacants d'un coup — le trône a montré qu'il ne penche pour personne." } }, 3 },

    /* R2 — Industrie vs Société : la route. */
    [EVID_CONSEIL_R2] = { EVID_CONSEIL_R2, EV_COUNTRY, "L'Industrie et la Société se disputent la route",
        trig_conseil_r2, 1825.f, NULL, {
        { "Trancher pour l'Industrie",
          "Le commerce ouvre sa route — la Société encaisse.",
          { .d_treasury_mois=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_MARCHAND, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La route s'ouvre au commerce — la Société retient qui l'a emporté." },
        { "Trancher pour la Société",
          "La route reste sous la coutume des familles.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_CONQUERANT, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La coutume l'emporte — l'Industrie retient qui l'a emporté." },
        { "Renvoyer les deux",
          "Ni l'un ni l'autre — la route attendra un arbitre moins intéressé.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Deux sièges vacants — la route, elle, attendra encore un peu." } }, 3 },

    /* R3 — Savoir vs Industrie : le cadastre. */
    [EVID_CONSEIL_R3] = { EVID_CONSEIL_R3, EV_COUNTRY, "Le Savoir et l'Industrie se disputent le cadastre",
        trig_conseil_r3, 1825.f, NULL, {
        { "Trancher pour le Savoir",
          "Le cadastre sert la mesure exacte — l'Industrie s'incline.",
          { .d_K_inst=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_TRANSGRESSEUR, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le cadastre sert la mesure — l'Industrie retient qui l'a emporté." },
        { "Trancher pour l'Industrie",
          "Le cadastre sert le commerce — le Savoir s'incline.",
          { .d_treasury_mois=0.2f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_MARCHAND, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le cadastre sert le commerce — le Savoir retient qui l'a emporté." },
        { "Renvoyer les deux",
          "Le cadastre attendra — aucun des deux ne le méritait tel qu'il l'a demandé.",
          { .d_H_coerc=0.3f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Deux sièges vacants — le cadastre, lui, reste à faire." } }, 3 },

    /* A1 — L'ALLIANCE de deux sièges : laisser la synergie ×rot / contrebalancer / séparer. */
    [EVID_CONSEIL_A1] = { EVID_CONSEIL_A1, EV_COUNTRY, "Deux sièges du Conseil s'entendent trop bien",
        trig_conseil_a1, 1825.f, NULL, {
        { "Laisser faire",
          "Une alliance de sièges qui tient, tant mieux pour l'efficacité — tant pis pour l'équilibre.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "L'alliance tient — l'efficacité y gagne, l'équilibre du Conseil un peu moins." },
        { "Contrebalancer",
          "On pousse discrètement un contrepoids ailleurs dans le Conseil.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le contrepoids apaise — sans briser ce qui, pour l'instant, fonctionne." },
        { "Séparer",
          "Deux voix trop accordées : on en écarte une, par prudence.",
          { .d_H_coerc=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La séparation coûte une efficacité gagnée — et achète une prudence retrouvée." } }, 3 },

    /* A2 — leur candidat au 3e siège. */
    [EVID_CONSEIL_A2] = { EVID_CONSEIL_A2, EV_COUNTRY, "L'alliance du Conseil propose son candidat au 3e siège",
        trig_conseil_a2, 1825.f, NULL, {
        { "Accepter leur candidat",
          "Le troisième siège complète l'accord — le Conseil parle d'une seule voix, désormais.",
          { .d_L=0.1f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le troisième siège complète l'accord — au risque d'un Conseil trop uni pour contredire." },
        { "Imposer son propre choix",
          "Le trône garde la main sur le dernier siège — l'alliance en prend note.",
          { .d_H_coerc=0.1f, .d_agitation=4.f, .unlock_branch=-1 },
          0.3f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le trône impose son choix — l'alliance en prend bonne note, pour plus tard." },
        { "Laisser le siège vacant",
          "Ni leur candidat, ni le sien — le siège attendra un jour plus clair.",
          { .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le siège reste vide — un vide qui, parfois, en dit plus qu'une nomination." } }, 3 },

    /* C1 — LA CONSPIRATION : les renvoyer les deux / en sacrifier un / céder. */
    [EVID_CONSEIL_C1] = { EVID_CONSEIL_C1, EV_COUNTRY, "Deux sièges aliénés complotent contre le trône",
        trig_conseil_c1, 2555.f, NULL, {
        { "Les renvoyer les deux",
          "On ne prend pas de risque — les deux sièges tombent d'un coup.",
          { .d_H_coerc=0.5f, .d_L=0.2f, .d_agitation=10.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Le trône plie deux sièges d'un coup — le Conseil, lui, retient la leçon de la peur." },
        { "En sacrifier un",
          "Un exemple suffit — l'autre reste, humilié, mais en place.",
          { .d_H_coerc=0.3f, .d_L=0.1f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Un siège tombe, l'autre reste — humilié, il n'oubliera pas non plus." },
        { "Céder",
          "Le trône plie devant la conspiration — pour cette fois.",
          { .d_L=-0.6f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2555 },
          "Le trône a plié — la conspiration l'a emporté, et le sait désormais." } }, 3 },

    /* ═══════════════════════════════════════════════════════════════════
     * V2b LOT 3 — LE CONTENU DÉBLOQUÉ (lecteurs P7a). ANCRES : même fourchette
     * que le reste du module. human gate (comme LOT 1/2, cf. les triggers).
     * EXEMPTÉS du plafond mondial (personnels, cooldowns 1460-2555 j).
     * ═══════════════════════════════════════════════════════════════════ */

    /* B2 — deux cultures voisines ne s'accordent plus (rivalité de voisinage). */
    [EVID_RIVAUX_VOISINS] = { EVID_RIVAUX_VOISINS, EV_COUNTRY, "Deux peuples voisins ne s'accordent plus sur rien",
        trig_rivaux_voisins, 1825.f, NULL, {
        { "Fermer la frontière",
          "Ce qui ne s'accorde plus, on cesse au moins de le voir.",
          { .d_H_coerc=0.2f, .d_C_global=-0.05f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_CONQUERANT, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La frontière se ferme — la rancune, elle, ne connaît pas de douane." },
        { "Ouvrir un dialogue",
          "Un émissaire, une table, et peut-être un peu moins de rancœur.",
          { .d_L=0.2f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le dialogue s'ouvre, patient — et parfois, la rancœur s'y use un peu." },
        { "Attiser la rivalité",
          "Un ennemi commode à la frontière vaut mieux qu'un allié encombrant.",
          { .d_agitation=6.f, .d_influence=1.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_CONQUERANT, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La rivalité attisée sert le trône — jusqu'au jour où elle se retourne." } }, 3 },

    /* B3 — une parenté lointaine se souvient. */
    [EVID_PARENTE_LOINTAINE] = { EVID_PARENTE_LOINTAINE, EV_COUNTRY, "Une parenté lointaine se souvient de nous",
        trig_parente_lointaine, 1825.f, NULL, {
        { "Renouer",
          "Des cousins d'outre-monde, jamais rencontrés — et pourtant, du même sang culturel.",
          { .d_L=0.2f, .d_influence=2.f, .d_treasury_mois=-0.3f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Les cousins renouent — une amitié lointaine, mais réelle." },
        { "Rester distant",
          "Le sang culturel ne fait pas une alliance — on garde ses distances.",
          { .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La distance se maintient — ni chaude, ni froide, simplement lointaine." },
        { "Exploiter la parenté",
          "Une parenté commode se monnaie — les cousins ne le savent pas encore.",
          { .d_treasury_mois=0.3f, .d_influence=-1.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_MARCHAND, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La parenté exploitée rapporte — et se découvre, tôt ou tard, exploitée." } }, 3 },

    /* B6 — une région marche loin de l'éthos régnant (province). */
    [EVID_MARCHE_ETHOS] = { EVID_MARCHE_ETHOS, EV_PROVINCE, "À %s, l'éthos ne ressemble plus à celui du trône",
        trig_marche_ethos, 1825.f, NULL, {
        { "Ramener dans le rang",
          "Une marche qui pense trop différemment finit par s'écarter pour de bon.",
          { .d_H_coerc=0.3f, .d_L=-0.2f, .d_coercion=0.12f, .d_agitation=8.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La marche rentre dans le rang — de force, et ça se sent." },
        { "Laisser sa différence",
          "Une marche qui pense autrement n'est pas encore une marche qui trahit.",
          { .d_L=0.2f, .d_agitation=-4.f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La différence reste — et la marche, curieusement, n'en est pas moins loyale." },
        { "En faire un exemple d'autonomie",
          "Ce que la marche pense, on le laisse même s'exprimer un peu.",
          { .d_K_inst=0.2f, .d_L=0.1f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_COMMUNAUTAIRE, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "L'autonomie affichée rassure les marches — et inquiète un peu la capitale." } }, 3 },

    /* C2 — le décret de tolérance (fracture religieuse notable). */
    [EVID_TOLERANCE_CREDO] = { EVID_TOLERANCE_CREDO, EV_COUNTRY, "Le décret de tolérance attend une signature",
        trig_tolerance_credo, 1825.f, NULL, {
        { "Signer le décret",
          "Chaque culte garde son temple — le trône n'impose plus une seule foi.",
          { .d_L=0.3f, .d_agitation=-8.f, .d_treasury_mois=-0.4f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La tolérance signée apaise les cultes minoritaires — et froisse les zélotes de la foi d'État." },
        { "Refuser",
          "Une seule foi, une seule loi — le trône ne transige pas.",
          { .d_H_coerc=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le refus tient l'orthodoxie — et laisse la scission religieuse couver." },
        { "Tolérance limitée",
          "On tolère, mais on surveille — un compromis qui ne satisfait personne tout à fait.",
          { .d_L=0.1f, .d_agitation=-3.f, .unlock_branch=-1 },
          0.1f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le compromis tient, tiède — ni la ferveur ni la scission n'y gagnent grand-chose." } }, 3 },

    /* C3 — le lettré porte une face périmée. */
    [EVID_LETTRE_PERIME] = { EVID_LETTRE_PERIME, EV_COUNTRY, "Le lettré du trône prêche une foi d'hier",
        trig_lettre_perime, 1825.f, NULL, {
        { "Le remplacer",
          "Un lettré neuf, à jour de la doctrine du trône.",
          { .d_L=0.2f, .d_treasury_mois=-0.5f, .unlock_branch=-1 },
          0.5f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le lettré remplacé prêche enfin la doctrine du jour — l'ancien s'en va, amer." },
        { "Le laisser prêcher",
          "Une face périmée n'est pas encore une hérésie — on ferme les yeux.",
          { .d_agitation=4.f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "Le lettré périmé continue de prêcher — les fidèles, eux, s'y perdent un peu." },
        { "Le corriger publiquement",
          "On l'expose, doctrine face à doctrine — et on le laisse en poste, humilié.",
          { .d_L=0.1f, .d_agitation=-2.f, .unlock_branch=-1 },
          0.1f, { .faction=FAC_GARDIEN, .faction_strength=0.05f, .scar_kind=SCAR_NONE, .cooldown_days=1825 },
          "La correction publique tient la doctrine — et laisse un lettré humilié à son pupitre." } }, 3 },

    /* C4 — la pratique dérive de la foi professée (crise plus profonde que C2). */
    [EVID_PRATIQUE_DERIVE] = { EVID_PRATIQUE_DERIVE, EV_COUNTRY, "La pratique du peuple ne ressemble plus à la foi qu'il professe",
        trig_pratique_derive, 2190.f, NULL, {
        { "Réaffirmer la doctrine",
          "On rappelle ce que la foi d'État exige vraiment — par le sermon, sinon par la loi.",
          { .d_H_coerc=0.3f, .d_L=0.2f, .d_agitation=6.f, .unlock_branch=-1 },
          0.4f, { .faction=FAC_GARDIEN, .faction_strength=0.10f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "La doctrine réaffirmée retient la lettre — la pratique, elle, a déjà dérivé plus loin." },
        { "Laisser la pratique dériver",
          "Une foi qui vit se transforme — on ne la fige pas de force.",
          { .d_L=-0.1f, .unlock_branch=-1 },
          0.4f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "La dérive continue, tranquille — la foi d'hier n'est déjà plus tout à fait celle d'aujourd'hui." },
        { "Codifier la nouvelle pratique",
          "Ce que le peuple pratique déjà, on l'écrit — la doctrine suit, pour une fois.",
          { .d_K_inst=0.3f, .d_L=0.3f, .d_agitation=-6.f, .unlock_branch=-1 },
          0.2f, { .faction=-1, .faction_strength=0.f, .scar_kind=SCAR_NONE, .cooldown_days=2190 },
          "La nouvelle pratique, codifiée, devient la doctrine — le trône a suivi son peuple, pour une fois." } }, 3 },
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
static const char *TREASON_FALLBACK[3] = { "Le savant", "Le notable", "Le marchand" };

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
    int seat = treason_seat_of(evid);
    if (seat >= 0){
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
        }
        cx->ev->ages.breach_pressure = clampf(cx->ev->ages.breach_pressure + e->d_breach, 0.f, 10.f);
        if (e->unlock_branch>=0 && e->unlock_branch<THM_COUNT && e->unlock_tier>=0 && e->unlock_tier<8)
            cx->ev->ages.tier_open[e->unlock_branch][e->unlock_tier]=true;
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

    /* ═══ V2b LOT 2 — LE CONSEIL : les actions qui MUTENT le Conseil (renvoi/
     * remplacement) passent par les VERBES existants (statecraft_council_dismiss),
     * jamais un accès direct aux tableaux — même motif que demography_manumit_
     * country pour EVID_CHAINES_RAPPORTENT ci-dessus. */
    if (cx->sc && cx->w){
        uint32_t seed = cx->w->seed;
        if (evid==EVID_CONSEIL_SUCCESSION && oi==0 && cid>=0){
            /* « Le remercier publiquement » ET « le laisser partir » vident TOUS
             * DEUX le siège (la retraite a lieu quoi qu'on choisisse — seul le
             * ton diffère, cf. les deltas L/or de la table) — statecraft_council_
             * age_tick (annuel, appelé par sim_day) videra le siège de toute façon
             * dès que l'âge dépasse le seuil de retraite ; on ne duplique PAS ce
             * mécanisme ici (rien à muter côté Conseil, le siège se videra tout
             * seul au prochain passage annuel — cette option est un TON, pas un acte). */
        }
        else if (evid==EVID_CONSEIL_R1 && oi==2 && cid>=0)      { statecraft_council_dismiss(cx->sc,seed,cid,0); statecraft_council_dismiss(cx->sc,seed,cid,1); }
        else if (evid==EVID_CONSEIL_R2 && oi==2 && cid>=0)      { statecraft_council_dismiss(cx->sc,seed,cid,2); statecraft_council_dismiss(cx->sc,seed,cid,1); }
        else if (evid==EVID_CONSEIL_R3 && oi==2 && cid>=0)      { statecraft_council_dismiss(cx->sc,seed,cid,0); statecraft_council_dismiss(cx->sc,seed,cid,2); }
        else if (evid==EVID_CONSEIL_A1 && oi==2 && cid>=0){
            /* Séparer : renvoie le siège B de la paire alliée retrouvée (le trigger
             * a déjà prouvé qu'une telle paire existe — même balayage déterministe). */
            int a=-1,b=-1;
            if (conseil_pair_find(cx, cid, COUNCIL_PAIR_ALLIANCE, &a, &b)) statecraft_council_dismiss(cx->sc,seed,cid,b);
        }
        else if (evid==EVID_TRAHISON_SAVOIR    && oi!=1 && cid>=0) statecraft_council_dismiss(cx->sc,seed,cid,0);
        else if (evid==EVID_TRAHISON_SOCIETE   && oi==0 && cid>=0) statecraft_council_dismiss(cx->sc,seed,cid,1);
        else if (evid==EVID_TRAHISON_INDUSTRIE && oi==0 && cid>=0) statecraft_council_dismiss(cx->sc,seed,cid,2);
        else if (evid==EVID_CONSEIL_C1 && cid>=0){
            /* La conspiration porte sur une paire ALIÉNÉE (COUNCIL_PAIR_CONSPIRATION) —
             * « Les renvoyer les deux » vide les deux sièges ; « En sacrifier un » n'en
             * vide qu'un (le premier de la paire, déterministe) ; « Céder » ne renvoie
             * personne (le trône a plié — les deux restent, plus forts). */
            int a=-1,b=-1;
            if (conseil_pair_find(cx, cid, COUNCIL_PAIR_CONSPIRATION, &a, &b)){
                if (oi==0){ statecraft_council_dismiss(cx->sc,seed,cid,a); statecraft_council_dismiss(cx->sc,seed,cid,b); }
                else if (oi==1) statecraft_council_dismiss(cx->sc,seed,cid,a);
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
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,human_player};
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
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,-1};
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
    EventCtx cx={ev,w,econ,wl,wp,sc,NULL,NULL,NULL,NULL,-1};
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
        EventCtx cx={ev,w,econ,wl,NULL,sc,rn,NULL,NULL,NULL,-1};
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
    EventCtx cx={ (EventsState*)ev, w, econ, wl, NULL, sc, NULL, NULL, NULL, NULL, -1 };
    static const int order[4]={ EVID_INTEG_DOMINATEUR, EVID_INTEG_MERCANTILE,
                                EVID_INTEG_BUREAUCRATE, EVID_INTEG_ANCIEN };
    for (int i=0;i<4;i++){
        const EventDef *d=&EVENTS[order[i]];
        if (d->trigger && d->trigger(&cx, region)) return order[i];
    }
    return -1;
}

/* ===================================================================== */
/* ÂGES — une LECTURE du monde fait advenir l'ère                        */
/* ===================================================================== */
#define NODE_VALUE_Y   1.0f    /* une région « riche » : route_pe au-dessus de ça */
#define X_NODES        4       /* … et il en faut X pour l'Âge du Commerce         */
#define LUM_TOTAL_Y   25.f     /* Lumière mondiale cumulée → Âge de la Raison      */
#define EMPIRES_INTEG  10      /* régions bien intégrées → Âge des Empires         */
#define BREACH_CHARGE  5.f     /* charge faustienne quelque part → Âge de la Brèche*/
/* ---- Âges structurels : seuils & poussées (la surface d'équilibrage) ---- */
#define LUM_SAVOIR_Y  40.f     /* société de MASSE : savoir mondial cumulé          */
#define LUM_C_Y        4.f     /* … ET connectée (les idées circulent)              */
#define MASSE_CRITIQUE 2       /* pays en révolution → contagion des Soulèvements   */
#define OF_FRAC_Y      3.0f    /* monde qui se fracture …                            */
#define OF_DEREAL_Y    1.0f    /* … sous charge faustienne …                         */
#define OF_SI_CRISE    5.0f    /* … en crise ouverte → l'Ordre de Fer répond         */
#define AGE_DELTA_I    2.0f    /* Lumières : surgissement des idées (+ I)            */
#define AGE_DELTA_SOLV 2.0f    /* Lumières : dissolution de la légitimité coercitive */
#define AGE_DELTA_L    2.0f    /* Soulèvements : la légitimité ne porte plus (− L)   */
#define AGE_DELTA_H    2.5f    /* Ordre de Fer : la poigne (+ H)                     */
#define AGE_DELTA_MYTH 3.0f    /* Ordre de Fer : le mythe nie la diversité (− D̄)     */

/* Verdict du moteur, mode « révolution » = SCPS_SUBMERGE_REVOLUTION (miroir, on
 * n'inclut PAS scps_core : on lit l'int déjà stocké dans CountryProsperity). */
enum { EV_MODE_CONSENTI=0, EV_MODE_COERC_FRAGILE, EV_MODE_REVOLUTION, EV_MODE_SECESSION };

static const char *AGE_NAMES[AGE_COUNT]={
    "Âge du Commerce Mondial","Âge de la Raison","Âge des Empires","Âge de la Brèche",
    "Âge des Lumières","Âge des Soulèvements","Âge de l'Ordre de Fer"
};
const char *age_name(AgeId a){ return (a>=0&&a<AGE_COUNT)?AGE_NAMES[a]:"?"; }

/* ---- Lecteurs de l'état AGRÉGÉ du monde (sur les pays non-vierges) ------ */
static int nc_loop(const World *w, const WorldProsperity *wp){
    return (w->n_countries < wp->n_countries) ? w->n_countries : wp->n_countries;
}
static float w_total_savoir(const World *w, const WorldProsperity *wp){
    float s=0.f;
    for (int c=0;c<nc_loop(w,wp);c++)
        if (w->country[c].role!=POLITY_UNCLAIMED) s+=wp->country[c].Lumiere;
    return s;
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
/* Le mythe homogénéisant se diffuse : la foi du trône devient exclusive. */
static void spread_credo_purificateur(World *w, WorldEconomy *econ){
    for (int c=0;c<w->n_countries;c++){
        int cr=world_capital_region(w,c);
        if (cr>=0 && cr<econ->n_regions && econ->region[cr].culture.settled
            && econ->region[cr].culture.credo==CREDO_PLURALISTE)
            econ->region[cr].culture.credo=CREDO_PURIFICATEUR;
    }
}

static bool age_trig_commerce(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)w;(void)wp;(void)ts; int rich=0;
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].route_pe > NODE_VALUE_Y) rich++;
    return rich >= X_NODES;
}
static bool age_trig_reason(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)w;(void)econ;(void)ts; if(!wp) return false; float lum=0.f;
    for (int c=0;c<wp->n_countries;c++) lum += wp->country[c].Lumiere;
    return lum > LUM_TOTAL_Y;
}
static bool age_trig_empires(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[],
                             WorldLegitimacy *wl){
    (void)w;(void)wp;(void)ts; if(!wl) return false; int integ=0;
    for (int r=0;r<econ->n_regions;r++)
        if (econ->region[r].owner>=0 && econ->region[r].culture.settled
            && r<SCPS_MAX_REG && wl->years_held[r] > 40.f) integ++;
    return integ >= EMPIRES_INTEG;
}
static bool age_trig_breach(World *w, WorldEconomy *econ, WorldProsperity *wp, const TechState ts[]){
    (void)w;(void)econ;(void)wp; if(!ts) return false;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) if (ts[c].charge > BREACH_CHARGE) return true;
    return false;
}
/* ---- Âges STRUCTURELS (chaîne causale : Lumières d'abord) --------------- */
static bool age_trig_lumieres(World *w, WorldProsperity *wp){
    if (!wp) return false;
    return w_total_savoir(w,wp) > LUM_SAVOIR_Y && w_mean_C(w,wp) > LUM_C_Y;
}
static bool age_trig_soulevements(const EventsState *ev, World *w, WorldProsperity *wp){
    if (!ev->ages.dawned[AGE_LUMIERES] || !wp) return false;       /* précondition causale */
    return events_count_revolutionary(w,wp) >= MASSE_CRITIQUE;     /* verdict du moteur */
}
static bool age_trig_ordrefer(const EventsState *ev, World *w, WorldProsperity *wp){
    if (!ev->ages.dawned[AGE_LUMIERES] || !wp) return false;       /* société de masse */
    return w_mean_fracture(w,wp) > OF_FRAC_Y
        && w_mean_dereal(w,wp)   > OF_DEREAL_Y
        && w_mean_SI(w,wp)       < OF_SI_CRISE;                    /* crise ouverte */
}

static void age_dawn(EventsState *ev, AgeId a, World *w, WorldEconomy *econ, WorldProsperity *wp){
    EventCtx cx={ev,w,econ,NULL,wp,NULL,NULL,NULL,NULL,NULL,-1};
    EvEffect e; memset(&e,0,sizeof e); e.pop_mult=1.f; e.unlock_branch=-1;
    switch(a){
        case AGE_COMMERCE: e.d_C_global=1.0f; e.unlock_branch=THM_SOCIETE; e.unlock_tier=3; break;
        case AGE_REASON:   ev->ages.research_mult += 0.5f; e.unlock_branch=THM_SOCIETE; e.unlock_tier=4; break;
        case AGE_EMPIRES:  ev->ages.integration_mult += 0.5f; e.unlock_branch=THM_SOCIETE; e.unlock_tier=5; break;
        case AGE_BREACH:   e.d_breach=2.0f; e.unlock_branch=THM_SAVOIR; e.unlock_tier=5; break;
        /* Structurels : on déplace une ENTRÉE GLOBALE du moteur (le verdict suit). */
        case AGE_LUMIERES:
            if (wp){ wp->age_I_bonus += AGE_DELTA_I;            /* les idées surgissent */
                     wp->age_lumiere_solvent += AGE_DELTA_SOLV; }/* la légitimité coercitive se dissout */
            e.unlock_branch=THM_SOCIETE; e.unlock_tier=4;       /* le boon : le savoir */
            break;
        case AGE_SOULEVEMENTS: if(wp) wp->age_L_penalty += AGE_DELTA_L; break;   /* L ↓ partout (contagion) */
        case AGE_ORDRE_FER:
            if (wp){ wp->age_H_bonus      += AGE_DELTA_H;          /* la poigne */
                     wp->age_myth_homogen += AGE_DELTA_MYTH; }     /* le mythe nie la diversité */
            if (w && econ) spread_credo_purificateur(w,econ);
            break;
        default: break;
    }
    apply_effect(&cx, EV_WORLD, 0, &e);
    ev->ages.dawned[a]=true; ev->ages.last_dawned=(int)a;
    ev->ages.last_dawn_year = ev->ages.days_elapsed/365;   /* horodate l'avènement */
    /* §G2 — un ÂGE est le fait le PLUS notable : MÉMOIRE durable de poids plein
     * (subject = -1, échelle monde) → un présage l'évoquera plus tard. */
    dir_remember(&ev->director, ev->ages.days_elapsed, DMEM_AGE, -1, 1.0f);
}

bool events_check_ages(EventsState *ev, World *w, WorldEconomy *econ,
                       WorldProsperity *wp, WorldLegitimacy *wl, const TechState ts[]){
    bool any=false;
    /* Rythme : UN SEUL âge par fenêtre de 30 ans, rien avant l'an 30. La barrière
     * est RÉÉVALUÉE après chaque avènement (last_dawn_year bouge) → les âges
     * s'égrènent une génération à la fois, jamais en grappe. */
    #define AGE_GATE (ev->ages.days_elapsed/365 >= ev->ages.last_dawn_year + AGE_MIN_YEARS)
    if (AGE_GATE && !ev->ages.dawned[AGE_COMMERCE] && age_trig_commerce(w,econ,wp,ts)){ age_dawn(ev,AGE_COMMERCE,w,econ,wp); any=true; }
    if (AGE_GATE && !ev->ages.dawned[AGE_REASON]   && age_trig_reason  (w,econ,wp,ts)){ age_dawn(ev,AGE_REASON,w,econ,wp);   any=true; }
    /* Lumières AVANT les âges politiques (la société de masse d'abord). */
    if (AGE_GATE && !ev->ages.dawned[AGE_LUMIERES] && age_trig_lumieres(w,wp))        { age_dawn(ev,AGE_LUMIERES,w,econ,wp); any=true; }
    if (AGE_GATE && !ev->ages.dawned[AGE_EMPIRES]  && age_trig_empires (w,econ,wp,ts,wl)){ age_dawn(ev,AGE_EMPIRES,w,econ,wp);any=true; }
    if (AGE_GATE && !ev->ages.dawned[AGE_BREACH]   && age_trig_breach  (w,econ,wp,ts)){ age_dawn(ev,AGE_BREACH,w,econ,wp);   any=true; }
    /* Politiques : exigent que les Lumières aient eu lieu (causalité). */
    if (AGE_GATE && !ev->ages.dawned[AGE_SOULEVEMENTS] && age_trig_soulevements(ev,w,wp)){ age_dawn(ev,AGE_SOULEVEMENTS,w,econ,wp); any=true; }
    if (AGE_GATE && !ev->ages.dawned[AGE_ORDRE_FER]    && age_trig_ordrefer   (ev,w,wp)){ age_dawn(ev,AGE_ORDRE_FER,w,econ,wp);    any=true; }
    #undef AGE_GATE
    return any;
}
bool  ages_dawned(const EventsState *ev, AgeId a){ return (a>=0&&a<AGE_COUNT)?ev->ages.dawned[a]:false; }
bool  ages_tier_open(const EventsState *ev, TechTheme br, int tier){
    return (br>=0&&br<THM_COUNT&&tier>=0&&tier<8)?ev->ages.tier_open[br][tier]:false;
}
float ages_breach_pressure(const EventsState *ev){ return ev->ages.breach_pressure; }

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
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,eg,human_player};
    g_title_sc = sc;   /* lot M : latch display-only pour event_title (noms de ministre) */
    ev->ages.days_elapsed += days;          /* horloge de jeu (rythme des âges) */

    /* 1. CHOCS GÉO — à risque, par région, sur leur cadence (1/risk accélère). */
    static const int SHOCKS[4]={EVID_QUAKE,EVID_FLOOD,EVID_DROUGHT,EVID_FIRE};
    for (int r=0;r<econ->n_regions;r++){
        if (!econ->region[r].culture.settled) continue;
        for (int si=0;si<4;si++){
            int s=SHOCKS[si];
            float risk=shock_risk(ev,r,s);
            if (risk<0.06f) continue;                       /* la géo l'interdit ici */
            float mtth=EVENTS[s].mtth_days / risk;          /* fréquent là où le risque est haut */
            if (frand(&ev->rng) < mtth_p(mtth,days)) events_strike(ev,w,econ,wl,wp,sc,r,s);
        }
    }
    /* Peste : foyer rare sur le plus grand carrefour ouvert, puis diffusion. */
    {
        int hub=-1; float best=NODE_VALUE_Y;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].route_pe>best){ best=econ->region[r].route_pe; hub=r; }
        if (hub>=0 && frand(&ev->rng) < mtth_p(EVENTS[EVID_PLAGUE].mtth_days, days))
            events_plague_spread(ev,w,econ,wl,sc,rn,hub);
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
     * — résolution émergente, pas un minuteur. Les acquis (palier, credo) restent. */
    if (wp){
        float k = clampf(0.0004f*(float)days, 0.f, 1.f);
        wp->age_I_bonus         -= wp->age_I_bonus         * k;
        wp->age_lumiere_solvent -= wp->age_lumiere_solvent * k;
        wp->age_L_penalty       -= wp->age_L_penalty       * k;
        wp->age_H_bonus         -= wp->age_H_bonus         * k;
        wp->age_myth_homogen    -= wp->age_myth_homogen    * k;
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
