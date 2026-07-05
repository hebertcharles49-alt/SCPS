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

/* MEMBRANE DE DÉCISION — TÉLÉMÉTRIE (« la télémétrie est la preuve d'équilibre ») : combien
 * de fois la crise phare (et sa suite conséquente) ont tiré sur l'ensemble d'un run. Statics
 * de MODULE, RAZ à events_init (par partie/sim, comme g_wild_spawned), PAS sérialisés — la
 * chronique les lit pour son bilan (résolu par IA OU joueur, les deux passent par resolve_choice). */
static long g_marbrive_fired=0, g_pont_effondre_fired=0;
long events_marbrive_fired(void){ return g_marbrive_fired; }
long events_pont_effondre_fired(void){ return g_pont_effondre_fired; }

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

void events_init(EventsState *ev, const World *w, uint32_t seed){
    memset(ev,0,sizeof(*ev));
    ev->rng = seed ? seed : 0xA17F23C5u;
    ev->ages.research_mult = 1.f;
    ev->ages.integration_mult = 1.f;
    ev->ages.last_dawned = -1;
    /* L'Aube dure 20 ans : le 1er âge ne peut s'éveiller avant l'an 20
     * (gate : an ≥ last_dawn_year + 30, donc last init = 20 − 30 = −10). */
    ev->ages.last_dawn_year = AGE_DAWN_YEARS - AGE_MIN_YEARS;
    ev->last_id = -1; ev->last_name = NULL;
    g_marbrive_fired=0; g_pont_effondre_fired=0;   /* MEMBRANE DE DÉCISION : télémétrie RAZ par sim */

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
          { .d_L=1.2f, .d_food_cap=0.5f, .d_treasury=120.f, .d_influence=4.f, .pop_mult=1.02f, .unlock_branch=-1 }, 1.f } }, 1 },
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
};

const EventDef *event_def(int evid){ return (evid>=0&&evid<EVID_COUNT)?&EVENTS[evid]:NULL; }

/* TITRE PRÉSENTÉ (display-only) : le nom de la table est un GABARIT — s'il porte
 * un « %s », on y coud le NOM RÉEL du sujet (région en EV_PROVINCE, pays en
 * EV_COUNTRY) au moment de la PRÉSENTATION (« Marbrive » était un placeholder de
 * registre : le monde a ses propres noms). Assemblage MANUEL (pas de format non
 * littéral) ; buf est rendu pour chaîner les appels. */
const char *event_title(const World *w, int evid, int subject, char *buf, int n){
    const EventDef *d = event_def(evid);
    if (!buf || n<=0) return "";
    if (!d){ buf[0]='\0'; return buf; }
    const char *p = strstr(d->name, "%s");
    if (!p || !w){ snprintf(buf, (size_t)n, "%s", d->name); return buf; }
    const char *nom = "?";
    if (d->scope==EV_PROVINCE  && subject>=0 && subject<w->n_regions)   nom = w->region[subject].name;
    if (d->scope==EV_COUNTRY   && subject>=0 && subject<w->n_countries) nom = w->country[subject].name;
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
    if (e->pop_mult>0.f && e->pop_mult!=1.f)
        for (int k=0;k<CLASS_COUNT;k++) re->strata[k].pop *= e->pop_mult;
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
    if (h->scar_kind>SCAR_NONE && h->scar_kind<SCAR_KIND_COUNT)
        decision_memory_push(cx->ev, subject, (ScarKind)h->scar_kind, 180, 540);
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
static void resolve_choice(EventCtx *cx, int evid, int subject, int oi, int today){
    const EventDef *d=&EVENTS[evid];
    if (oi<0 || oi>=d->n_options) oi=0;
    const EvOption *opt=&d->options[oi];
    int cid = event_owner_of(cx, d->scope, subject);
    apply_effect(cx, d->scope, subject, &opt->eff);
    apply_choice_hook(cx, evid, subject, cid, &opt->hook, today);
    /* PONT EFFONDRÉ CONSOMME la cicatrice qui l'a fait mûrir (une cicatrice = un tir —
     * sinon le trigger la relirait ENCORE mûre au prochain scan et re-déclencherait). */
    if (evid==EVID_PONT_EFFONDRE) decision_memory_consume(cx->ev, subject, SCAR_SABOTAGE_CHANTIER, today);
    if (evid==EVID_MARBRIVE) g_marbrive_fired++; else if (evid==EVID_PONT_EFFONDRE) g_pont_effondre_fired++;
    cx->ev->last_id=evid; cx->ev->last_name=d->name; cx->ev->n_fired++;
    if (d->scope==EV_PROVINCE && subject>=0)
        provlog_push_event(subject, event_title_ring(cx->w, evid, subject),   /* nom RÉEL du lieu */
                           ev_sign(&opt->eff), ev_effdir(&opt->eff));
    /* FIL D'ÉVÈNEMENTS (alertes/popup du front) : write-only, jamais relu → déterminisme
     * intact. Le focus du fil (feed_set_focus) filtre à l'entrée, le front re-filtre en ceinture. */
    feed_push(FEED_DIRECTOR, cid, -1, (d->scope==EV_PROVINCE) ? subject : -1, evid);
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
static void fire_event(EventCtx *cx, int evid, int subject){
    const EventDef *d=&EVENTS[evid];
    /* RÉPIT (hook cooldown_days d'un choix précédent) : cet évènement ne retire pas
     * sur ce sujet tant qu'il n'a pas expiré — même acteur pour l'IA et le joueur. */
    if (decision_cooldown_active(cx->ev, evid, subject, cx->ev->ages.days_elapsed)) return;
    if (d->n_options>1 && cx->human_player>=0){
        int owner = event_owner_of(cx, d->scope, subject);
        if (owner==cx->human_player){
            if (!pending_event_has(cx->ev, evid, subject))
                pending_event_push(cx->ev, evid, subject, cx->ev->ages.days_elapsed);
            return;
        }
    }
    int best=0; float bw=-1.f;
    for (int i=0;i<d->n_options;i++) if (d->options[i].ai_chance>bw){ bw=d->options[i].ai_chance; best=i; }
    resolve_choice(cx, evid, subject, best, cx->ev->ages.days_elapsed);
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
                           int slot, int option, int today){
    if (!ev || slot<0 || slot>=ev->pending_n) return false;
    PendingEvent p = ev->pending[slot];
    if (p.evid>=EVID_COUNT) { pending_event_remove(ev,slot); return false; }
    const EventDef *d=&EVENTS[p.evid];
    if (option<0 || option>=d->n_options) return false;
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,-1};   /* la résolution elle-même n'enfile jamais (déjà tranchée) */
    resolve_choice(&cx, p.evid, p.subject, option, today);
    pending_event_remove(ev, slot);
    return true;
}
void pending_event_tick_expire(EventsState *ev, World *w, WorldEconomy *econ,
                               WorldLegitimacy *wl, WorldProsperity *wp, Statecraft *sc,
                               RouteNetwork *rn, const TechState ts[], DiploState *dp,
                               int today){
    if (!ev) return;
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,-1};
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
    EventCtx cx={ev,w,econ,wl,wp,sc,NULL,NULL,NULL,-1};
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
        EventCtx cx={ev,w,econ,wl,NULL,sc,rn,NULL,NULL,-1};
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
    EventCtx cx={ (EventsState*)ev, w, econ, wl, NULL, sc, NULL, NULL, NULL, -1 };
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
    EventCtx cx={ev,w,econ,NULL,wp,NULL,NULL,NULL,NULL,-1};
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
                       RouteNetwork *rn, const TechState ts[], DiploState *dp, int days,
                       int human_player){
    EventCtx cx={ev,w,econ,wl,wp,sc,rn,ts,dp,human_player};
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

    /* 2bis. LE DIRECTEUR (§F) — lit la TEMPÉRATURE du monde, puis stabilise ou
     * déstabilise (sans jamais s'acharner). Cadence ~annuelle (sa propre échéance). */
    director_tick(&cx, days);

    /* 3. ÂGES — scan d'interprétation du monde. */
    events_check_ages(ev,w,econ,wp,wl,ts);

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
    pending_event_tick_expire(ev,w,econ,wl,wp,sc,rn,ts,dp, ev->ages.days_elapsed);
}

/* ===================================================================== */
/* GARDE-FOU MEMBRANE — aucun nom SCPS dans les textes joueur            */
/* ===================================================================== */
bool events_text_clean(void){
    static const char *BANNED[]={ "SCPS","D∞","∞","K_inst","H_coerc","P_realise",
        "fragilit","fractur","flux_faustien","age_C","breach_flux","D_bar", NULL };
    const char *texts[256]; int n=0;
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
