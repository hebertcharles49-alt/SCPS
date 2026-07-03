/*
 * scps_ai.c — la boucle de décision IA (voir scps_ai.h)
 *
 * Tout ce que fait l'IA passe par les verbes du JOUEUR :
 *   agency_order_build   (bâtir K/H/food/PE)
 *   routes_order         (ouvrir une route — la cloche f(D̄))
 *   diplo_declare_war / diplo_settle (la paix transfère l'occupé) / diplo_make_peace
 * Elle ne touche jamais l'état directement : elle pousse des leviers, le moteur
 * d'ordre fait le reste. La personnalité sort de la fiche ; le frein sort de la
 * coordonnée de consolidation. Aucune branche « si pays==X ».
 */
#include "scps_ai.h"
#include "scps_religion.h"   /* P7 : schisme IA + emploi du lettré (gated) */
#include "scps_tune.h"   /* Arc J : calibrage */
#include "scps_tech.h"
#include "scps_heritage.h"
#include "scps_factions.h"   /* l'éthos effectif + la fracture de valeurs (frein interne §6) */
#include "scps_intertrade.h" /* §leviers : l'embargo — la guerre commerciale du Mercantile */
#include "scps_labor.h"      /* F-arc : capitale_max_tier (le tier de capitale qui gate les manufactures) */
#include "scps_credit.h"     /* dette : les gardes « can't afford » deviennent « tu t'endettes » */
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define AI_ETHOS_FRACTURE_W     0.7f  /* poids du frein INTERNE : une politique déchirée se consolide (§6) */
#define AI_ETHOS_FRACTURE_FLOOR 0.38f /* socle : un mono-éthos a une fracture résiduelle — on ne freine qu'AU-DELÀ */

/* ---- Cadences & calibrage --------------------------------------------- */
#define AI_ECON_CADENCE   550    /* ~1.5 an entre décisions éco/bâti        */
#define AI_STRAT_CADENCE  1100   /* ~3 ans entre décisions stratégiques     */
#define AI_PEACE_LOCK     1825   /* 5 ans de consolidation forcée (hystérésis) */
#define AI_ARMY_MARGIN    0.75f  /* n'attaque que si armée ≥ 0.75× la cible  */
#define AI_WIDEN_W        0.5f    /* friction : poids du coût d'élargissement (alliés de la cible) */
#define AI_SURRENDER      55.f    /* score de guerre adverse au-delà duquel un défenseur sans espoir capitule */
#define AI_ALLY_SEUIL     6.0f    /* score d'alliance au-delà duquel on PROPOSE l'alliance */
#define AI_ALLY_DISSOLVE  3.0f    /* §D1 (ancien) : seuil de score — conservé pour mémoire, la
                                   * dissolution lit désormais la MENACE (shared_rel), pas le score */
/* §D-sat : le plafond de slots vit dans scps_diplo.h (DIPLO_ALLY_SLOTS) — partagé avec
 * le statecraft pour que « 2 alliances max » soit un invariant GLOBAL. */
#define AI_ALLY_SWAP_MARGIN 1.5f  /* slots pleins : on n'évince le plus faible que si le nouveau
                                   * le dépasse de cette marge → l'alliance est une ressource arbitrée */
#define ALLY_THREAT_KEEP  0.6f    /* §D1 : sous ce niveau de menace COMMUNE relative (shared_rel),
                                   * l'alliance n'a plus de raison → elle lâche (le slot se libère) */
#define AI_CONQUEROR_W    0.60f   /* appétit de conquête au-delà duquel on SAISIT une proie faible
                                   * AVANT de songer à s'allier (sinon, en monde calme §D2, le pacte
                                   * facile fige le conquérant — il ne prédate jamais) */
#define AI_FOOD_FLOOR     1.5f   /* sous ce seuil de marge : grenier d'abord */
#define AI_BRAKE_HARD     0.6f   /* frein dur : consolidation impérative     */
#define AI_RANCOR_W       3.0f   /* §6 biais de RECONQUÊTE : on vise qui nous a pris nos terres */
#define AI_CRUSADE_W      4.0f   /* croisade : l'orthodoxe vise qui développe le faustien (chance ∝ ferveur) */
#define AI_ANNEX_FRAC     0.6f   /* §5 : un budget ≥ 60 % de la valeur du pays = victoire décisive → annexion */
#define AI_WAR_EXHAUST   10.0f   /* 10 ANS de guerre → la paix est déclarée (paix BLANCHE si le score n'a pas tranché) */
#define AI_WAR_DECISIVE   50.f   /* score de guerre ≥ 50 (terrain gagné/occupé) → paix DÉCISIVE : on encaisse l'occupé */
/* §H3 — guerre outre-mer : portée = tout chemin de courants existant (campaign_order_sea
 * ne plafonne pas la distance — c'est la pénalité de score qui rend la traversée longue
 * moins attractive). Seuil large : world_sea_days < 0 = bassins séparés = injoignable. */
#define AI_SEA_WAR_MAX_DAYS 400.f /* portée max de la guerre trans-mer (toute mer accessible) */
#define AI_SEA_WAR_PENALTY  0.60f /* pénalité logistique : une cible outre-mer vaut 60 % d'une cible terrestre */
/* §4 — leviers : chaque ACTE est un vote. Une politique tenue accumule vers le cap. */
#define AI_LEVER_TECH     0.05f  /* franchir l'interdit (tech faustienne) → Transgresseurs */
#define AI_LEVER_WAR      0.05f  /* conquérir → Conquérants */
#define AI_LEVER_BUILD    0.035f /* bâtir une famille d'édifices → la faction afférente */
/* ---- Recherche (l'arbre de tech vivant) ------------------------------- */
#define AI_RESEARCH_CADENCE 365  /* ~1 an entre déverrouillages potentiels */
#define AI_RESEARCH_RATE    42.f /* P5.29 : ×3 pour suivre le coût ×3 → rythme IA inchangé */
#define AI_RESEARCH_POPREF  8000.f /* population qui DOUBLE l'assiette de recherche */
#define AI_TECH_PENCHANT    2.0f  /* biais vers le thème de SA heritage (penchant, pas « si ») */
/* ── M1 (design §6, VERBATIM) — ETHOS_FN[ETHOS][FN] : l'éthos pèse la FONCTION.
 * Indexée sur l'enum réel TechFunction (FN_PRODUCTION=0, FN_ARMEE, FN_RENFORCEMENT)
 * par initialiseurs désignés — l'inversion de colonnes est impossible. */
static const float ETHOS_FN[ETHOS_COUNT][FN_COUNT] = {
    [ETHOS_DOMINATEUR]  = { [FN_ARMEE]=2.5f, [FN_RENFORCEMENT]=0.8f, [FN_PRODUCTION]=0.6f },
    [ETHOS_HONNEUR]     = { [FN_ARMEE]=2.2f, [FN_RENFORCEMENT]=1.0f, [FN_PRODUCTION]=0.6f },
    [ETHOS_ORDRE]       = { [FN_ARMEE]=1.2f, [FN_RENFORCEMENT]=2.0f, [FN_PRODUCTION]=1.0f },
    [ETHOS_BUREAUCRATE] = { [FN_ARMEE]=0.7f, [FN_RENFORCEMENT]=2.2f, [FN_PRODUCTION]=1.3f },
    [ETHOS_MERCANTILE]  = { [FN_ARMEE]=0.7f, [FN_RENFORCEMENT]=1.0f, [FN_PRODUCTION]=2.3f },
    [ETHOS_PACIFISTE]   = { [FN_ARMEE]=0.4f, [FN_RENFORCEMENT]=1.2f, [FN_PRODUCTION]=2.4f },
};
/* M1/M4 — l'argmax de la ligne d'éthos (banc §24 : ai_prefers_func). */
TechFunction ai_ethos_pref_func(Ethos e){
    if (e<0||e>=ETHOS_COUNT) return FN_RENFORCEMENT;
    TechFunction best=FN_PRODUCTION;
    for (int f=1;f<FN_COUNT;f++) if (ETHOS_FN[e][f]>ETHOS_FN[e][best]) best=(TechFunction)f;
    return best;
}
/* M4 — l'appétit faustien du CREDO (design §3) : CREDO_FAUST[credo]·(0.6+0.08·valeurs).
 * Pluraliste tolère (1.0) ; Évangéliste se méfie (0.7) ; Purificateur abhorre (0.4). */
static float ai_faustian_appetite(Credo cr, float valeurs){
    static const float CREDO_FAUST[CREDO_COUNT]={ [CREDO_PLURALISTE]=1.0f,
        [CREDO_EVANGELISTE]=0.7f, [CREDO_PURIFICATEUR]=0.4f };
    float k=(cr>=0&&cr<CREDO_COUNT)?CREDO_FAUST[cr]:1.f;
    return k*(0.6f+0.08f*valeurs);
}
#define AI_TECH_SIGNATURE   1.5f  /* prime à une signature accessible (la sienne / greffée) */
#define AI_FAUST_QUEST      0.80f /* S3 : SEUIL d'appétit pour QUÊTER l'emblème faustien (forge runique ×
                                   * arcane) — l'empire le PLUS faustien-enclin du monde (l'appétit plafonne
                                   * ~0.84 sans Dominateur pluraliste) ; le filtre S1 écarte les marchands */
#define AI_TECH_FAUSTIAN    1.2f  /* S3 : tolérance faustienne RÉCONCILIÉE (2.5→1.2) — l'appétit d'un
                                   * Transgresseur/culte (×1.4 + bénédiction) peut enfin RENCONTRER le
                                   * frein ; la charge → Brèche garde le franchissement RARE et coûteux */
#define AI_FOREUSE_HUNGER   5.0f  /* §4 : la FAMINE DE FER rend la foreuse arcanique irrésistible (surpasse le frein faustien) */
#define AI_RELOC_SEED      300.f  /* §reloc : ensemencement type (~EXTRACT_POP_REF) — pousse la cible d'une bande d'intensité */
#define AI_RELOC_FLOOR     100.f  /* pop minimale pour qualifier source/cible (jamais peupler le vide) */
#define AI_RELOC_COOLDOWN  1300   /* ~3.5 ans entre deux ensemencements (ENSEMENCER, pas pomper) */
#define AI_COLONY_FLOOR   1000.f  /* §dév : palier de pop visé par colonie (remplir les jobs → optimum démographique) */
#define AI_STAFF_PER_MANUF 250.f  /* §dév : pop MINIMALE par manufacture pour BÂTIR — sinon on pose dans le VIDE
                                   * (la fabrique L5 a 2200 jobs ; sous ce socle, le raw dort, les jobs vides) */
#define AI_FAITH_FAUSTIAN   3.0f  /* §4 : l'orthodoxie INTERDIT le faustien, le culte le SACRALISE */

/* ---- Utilitaires ------------------------------------------------------ */
static inline float clampf(float v, float lo, float hi){ return v!=v?lo:(v<lo?lo:(v>hi?hi:v)); }
static uint32_t xs32(uint32_t *s){ uint32_t x=*s; x^=x<<13; x^=x>>17; x^=x<<5; return *s=x?x:1u; }
static float frand(uint32_t *s){ return (float)(xs32(s)&0xffffffu) / (float)0x1000000u; }

/* ===================================================================== */
/* PERSONNALITÉ — dérivée de la fiche (aucun code par-faction)            */
/* ===================================================================== */
static float norm01(float x){ return clampf(x/10.f, 0.f, 1.f); }

void ai_derive_weights(AiActor *a, const PopCulture *self){
    if (!self){ a->w_expand=a->w_trade=a->w_build=a->w_faith=0.f; a->w_faustian=0.2f; return; }
    float v = self->valeurs;                       /* 0..10 ; haut = Dominateur/martial */

    /* Conquête : l'appétit suit les VALEURS (Dominateur ~9 … Pacifiste ~1.5). */
    a->w_expand = norm01(v);

    /* Commerce : l'inverse des valeurs, amplifié par un trait éco mercantile,
     * étouffé par un trait prédateur (le tribut n'ouvre pas de routes). */
    float econ_f = (self->econ==ECON_CARAVANE || self->econ==ECON_GUILDE) ? 1.5f
                 : (self->econ==ECON_TRIBUT    || self->econ==ECON_PILLAGE_RUINES) ? 0.4f
                 : 1.0f;
    a->w_trade = norm01(10.f - v) * econ_f;

    /* Bâtir du K : l'éthos qui TIENT la diversité (Bureaucrate > Ordre > reste ;
     * le Dominateur/Honneur bâtit peu — il tient par la force, pas par le droit). */
    float build_f = (self->ethos==ETHOS_BUREAUCRATE) ? 1.6f
                  : (self->ethos==ETHOS_ORDRE)        ? 1.2f
                  : (self->ethos==ETHOS_DOMINATEUR || self->ethos==ETHOS_HONNEUR) ? 0.6f
                  : 1.0f;
    a->w_build = 0.85f * build_f;   /* volonté de BÂTIR relevée (0.5→0.85) : l'IA développe
                                     * plus volontiers ses institutions — désormais nourrie
                                     * par les matériaux de la 2e ressource (§6b). */

    /* Foi : le prosélytisme pousse la guerre sainte / l'homogénéisation forcée. */
    a->w_faith = (self->credo==CREDO_PURIFICATEUR) ? 1.0f
               : (self->credo==CREDO_EVANGELISTE)  ? 0.6f
               : 0.1f;

    /* Pente faustienne : appétit d'arcane (de base ; la heritage Arcanique l'amplifie
     * via ses leviers, lus ailleurs). */
    a->w_faustian = 0.2f;

    /* Jitter : ±12 % par poids — deux Dominateurs ne conquièrent pas en phase. */
    a->w_expand   *= 0.88f + 0.24f*frand(&a->rng);
    a->w_trade    *= 0.88f + 0.24f*frand(&a->rng);
    a->w_build    *= 0.88f + 0.24f*frand(&a->rng);
    a->w_faith    *= 0.88f + 0.24f*frand(&a->rng);
    a->w_faustian *= 0.88f + 0.24f*frand(&a->rng);
    /* Socle figé : la résultante des factions le MODULERA (ai_refresh_ethos, §3). */
    a->w_base[0]=a->w_expand; a->w_base[1]=a->w_trade; a->w_base[2]=a->w_build;
    a->w_base[3]=a->w_faith;  a->w_base[4]=a->w_faustian;
}

/* §war — GARANTIR UN DOMINATEUR par monde : si AUCUN empire piloté n'a l'appétit de conquête
 * (w_expand ≥ AI_CONQUEROR_W), on HISSE le plus belliqueux au rang de Dominateur (socle ET courant,
 * pour que la glisse §3 le TIENNE) — sinon un monde tout en alliances reste atone (0 guerre). Un
 * seul suffit à amorcer la dynamique ; les mondes fendus en ont déjà (hautes valeurs) → intacts. */
void ai_ensure_dominator(AiActor *ai, const bool *ai_on, int n){
    if (!ai||!ai_on) return;
    int best=-1; float bw=-1.f; bool have=false;
    for (int c=0;c<n && c<SCPS_MAX_COUNTRY;c++){
        if (!ai_on[c]) continue;
        if (ai[c].w_expand >= AI_CONQUEROR_W){ have=true; break; }
        if (ai[c].w_expand > bw){ bw=ai[c].w_expand; best=c; }
    }
    if (!have && best>=0){
        float d=AI_CONQUEROR_W+0.12f;            /* franchement Dominateur (au-dessus du seuil de proie) */
        ai[best].w_expand=d; ai[best].w_base[0]=d;
    }
}

/* §3 — L'ÉTHOS EFFECTIF GLISSE : la personnalité du pays n'est plus figée à sa
 * culture de trône, elle suit la RÉSULTANTE de ses factions. On module le socle
 * par l'ÉCART entre le penchant du PEUPLE (distribution enracinée) et celui du
 * TRÔNE (culture régnante) : un empire homogène ne bouge pas (écart nul, équilibre
 * préservé) ; un empire qui a avalé des claniques voit sa conquête (et son faustien)
 * MONTER, son commerce baisser — « un empire change d'éthos quand qui le compose
 * change ». Borné : la résultante infléchit, elle ne renverse pas le socle. */
#define AI_ETHOS_GLIDE 1.0f
static float glide_axis(float base, float pop_share, float crown_share){
    float f = 1.f + AI_ETHOS_GLIDE*(pop_share - crown_share);
    return base * clampf(f, 0.3f, 2.0f);
}
static void ai_refresh_ethos(AiActor *a, const World *w, const WorldEconomy *econ){
    int cp = (a->cid>=0 && a->cid<w->n_countries) ? w->country[a->cid].capital_prov : -1;
    if (cp<0 || cp>=w->n_provinces || cp>=econ->n_prov) return;
    /* Le penchant du TRÔNE = celui de sa capitale (le siège du pouvoir) ; celui du
     * PEUPLE = la distribution de tout l'empire. Même source (les groupes) → un
     * empire homogène ne glisse PAS (capitale == empire), seule la diversité conquise
     * écarte les deux.
     * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir
     * de la province représentative (à jour seulement JUSQU'AU dernier econ_tick ; ai_step
     * s'exécute AVANT econ_tick dans sim_day, donc lire le miroir serait un tick EN RETARD).
     * On lit directement econ->prov[cp] (LA capitale, déjà en main ci-dessus) — précis et
     * sans latence, plutôt que de traverser econ->region[cr] pour retomber sur la même
     * province via rep_pid. */
    float crownlean[FAC_COUNT]; faction_weights_of(&econ->prov[cp].pop, 1, crownlean);
    float pop[FAC_COUNT];       faction_effective_distribution(w, econ, a->cid, pop);  /* base + leviers (§4) */
    a->w_expand   = glide_axis(a->w_base[0], pop[FAC_CONQUERANT],    crownlean[FAC_CONQUERANT]);
    a->w_trade    = glide_axis(a->w_base[1], pop[FAC_MARCHAND],      crownlean[FAC_MARCHAND]);
    a->w_build    = glide_axis(a->w_base[2], pop[FAC_LEGISTE],       crownlean[FAC_LEGISTE]);
    a->w_faith    = glide_axis(a->w_base[3], pop[FAC_GARDIEN],       crownlean[FAC_GARDIEN]);
    a->w_faustian = glide_axis(a->w_base[4], pop[FAC_TRANSGRESSEUR], crownlean[FAC_TRANSGRESSEUR]);
}

void ai_actor_init(AiActor *a, const World *w, const WorldEconomy *econ,
                   int cid, uint32_t seed){
    memset(a, 0, sizeof(*a));
    a->cid = cid;
    a->rng = (seed ? seed : 0x9e3779b9u) ^ (uint32_t)(cid*2654435761u);
    if (a->rng==0) a->rng=1u;

    int cp = (cid>=0 && cid<w->n_countries) ? w->country[cid].capital_prov : -1;
    a->home_region = (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;

    const PopCulture *self = (a->home_region>=0 && a->home_region<econ->n_regions)
                           ? &econ->region[a->home_region].culture : NULL;
    ai_derive_weights(a, self);

    /* Cadences DÉCALÉES : chacun se réveille à un moment propre (pas de lockstep). */
    a->next_econ_day     = (int)(frand(&a->rng) * AI_ECON_CADENCE);
    a->next_strat_day    = (int)(frand(&a->rng) * AI_STRAT_CADENCE);
    a->next_research_day = (int)(frand(&a->rng) * AI_RESEARCH_CADENCE);
    a->next_reloc_day    = (int)(frand(&a->rng) * AI_RELOC_COOLDOWN);   /* §reloc : décalé, puis cooldown */
}

/* ===================================================================== */
/* OBSERVATION — l'IA lit les mêmes coordonnées que la membrane           */
/* ===================================================================== */
AiView ai_observe(const WorldProsperity *wp, const World *w,
                  const WorldEconomy *econ, int cid){
    AiView v; memset(&v, 0, sizeof v);
    if (cid<0 || cid>=wp->n_countries) return v;
    const CountryProsperity *cp = &wp->country[cid];
    v.SI=cp->SI; v.fragilite=cp->fragilite; v.fracture=cp->fracture;
    v.L=cp->L; v.K=cp->K; v.Dinf_interne=cp->profile.D_inf_int; v.PE=cp->P_realise;
    for (int r=0; r<econ->n_regions; r++) if (econ->region[r].owner==cid){
        v.tresor += econ->region[r].treasury;
        v.food   += econ->region[r].build.food_cap;
    }
    v.armee = diplo_mil_power(w, econ, cid);

    /* Fracture de VALEURS (§6) : si deux factions-éthos opposées se disputent la
     * direction du pays, la politique se déchire (paralysie interne). Lu de la
     * distribution de factions enracinée dans les peuples du pays. */
    { float fw[FAC_COUNT]; country_faction_weights(w, econ, cid, fw);
      v.ethos_fracture = faction_fracture(fw); }

    /* ── PERCEPTION DES BESOINS — ce qui MANQUE (l'IA était aveugle à tout ça) ──
     * Lu des MÊMES données que la membrane montre au joueur : capacités d'extraction,
     * stocks, demande/offre agrégées du pays. Aucune omniscience sur l'ennemi. */
    {
        static const Resource STRAT[3] = { RES_SALTPETER, RES_CELESTIAL_IRON, RES_ARCANE_CRYSTAL };
        float rawcap[RES_COUNT], stock[RES_COUNT], demand[RES_COUNT], supply[RES_COUNT];
        for (int g=0; g<RES_COUNT; g++){ rawcap[g]=stock[g]=demand[g]=supply[g]=0.f; }
        for (int r=0; r<econ->n_regions; r++) if (econ->region[r].owner==cid){
            const RegionEconomy *re=&econ->region[r];
            for (int g=1; g<RES_COUNT; g++){ rawcap[g]+=re->raw_cap[g]; stock[g]+=re->stock[g];
                demand[g]+=re->demand[g]; supply[g]+=re->supply[g]; }
        }
        /* TROU DE CHAÎNE : un raffineur présent dont un intrant manque (ni extrait, ni en stock). */
        Resource chain=RES_NONE; float chain_short=0.f;
        for (int r=0; r<econ->n_regions && chain==RES_NONE; r++) if (econ->region[r].owner==cid){
            const RegionEconomy *re=&econ->region[r];
            for (int i=0; i<re->n_bld && chain==RES_NONE; i++){
                Resource in1,in2,out; building_recipe(re->bld[i].type,&in1,&in2,&out);
                Resource ins[2]={in1,in2};
                for (int k=0;k<2;k++){ Resource g=ins[k]; if(g==RES_NONE) continue;
                    if (rawcap[g]<0.1f && supply[g]<0.5f && stock[g]<1.f){ chain=g; chain_short=1.f; break; } }
            }
        }
        v.chain_gap=chain;
        /* TROU STRATÉGIQUE : une matière qui débloque tech/militaire, extraite NULLE PART. */
        Resource strat=RES_NONE; float strat_short=0.f;
        for (int k=0;k<3;k++){ Resource g=STRAT[k]; if (rawcap[g]<0.1f && stock[g]<0.5f){ strat=g; strat_short=1.f; break; } }
        v.strat_gap=strat;
        /* TROU DE DEMANDE : un bien dont la demande dépasse nettement l'offre (panier non comblé,
         * variante culturelle comprise — la demande des minorités est déjà dans re->demand). */
        Resource dgap=RES_NONE; float dworst=0.f;
        for (int g=RES_PROD_FIRST; g<RES_COUNT; g++){
            float d=demand[g], s=supply[g]+stock[g];
            if (d>1.f && s < d*0.6f){ float sh=(d-s)/d; if (sh>dworst){ dworst=sh; dgap=g; } }
        }
        v.demand_gap=dgap;
        v.gap_acuity = clampf(0.5f*chain_short + 0.5f*strat_short + 0.6f*dworst, 0.f, 1.f);
        /* PRESSION DE PRISE : un trou stratégique, ou un brut de chaîne, INTROUVABLE chez soi
         * (rawcap nul) → on ne peut ni le produire : ne restent que PRENDRE ou COMMERCER. */
        float take=0.f;
        if (strat!=RES_NONE) take += 0.6f;
        if (chain!=RES_NONE && chain<RES_PROD_FIRST && rawcap[chain]<0.1f) take += 0.4f;
        v.take_pressure = clampf(take, 0.f, 1.f);
    }

    /* ── ÉTAGES 1-2 : la PRÉVISION + la FILE DE PRIORITÉS (la motivation éco ÉMERGE) ──
     * Le forecast (runway/shortfall/déficit structurel) se lit des coordonnées (étage 1).
     * La PRIORITÉ d'un flux = stress(runway court) × prix (la valeur de marché, émergente)
     * × deficit_vs_safe (manque sous le coussin de réserve). AUCUNE hiérarchie codée : un
     * empire-bijoux dont l'or crise priorise l'or ; un empire affamé, la nourriture. */
    econ_country_forecast(econ, cid, tune_f("AI_PROJ_HORIZON",25.f), &v.fc);
    v.food_alert = (v.fc.food_runway < tune_f("AI_SAFETY_HORIZON",12.f));
    {
        float safety = tune_f("AI_SAFETY_HORIZON",12.f);
        float safe_months = tune_f("AI_SAFE_STOCK_MONTHS",6.f);
        float pr_sum[RES_COUNT]; float dem_m[RES_COUNT]; float stk2[RES_COUNT]; int pn[RES_COUNT];
        for (int g=0; g<RES_COUNT; g++){ pr_sum[g]=0.f; dem_m[g]=0.f; stk2[g]=0.f; pn[g]=0; }
        for (int r=0; r<econ->n_regions; r++) if (econ->region[r].owner==cid && econ->region[r].colonized){
            const RegionEconomy *re=&econ->region[r];
            for (int g=1; g<RES_COUNT; g++){ pr_sum[g]+=re->price[g]; pn[g]++; dem_m[g]+=re->demand[g]; stk2[g]+=re->stock[g]; }
        }
        Resource top=RES_NONE; float topp=0.f;
        for (int g=1; g<RES_COUNT; g++){
            if (dem_m[g] < 0.01f) continue;                       /* flux non demandé → pas une motivation */
            float pr = (pn[g]>0)? pr_sum[g]/(float)pn[g] : econ_base_price((Resource)g);
            float rw = v.fc.runway[g]; if (rw<0.05f) rw=0.05f;
            float stress = clampf(safety/rw, 0.f, 4.f);          /* runway court → urgent */
            float safe_stock = safe_months * dem_m[g];           /* coussin = N mois de demande */
            float dvs = (safe_stock>1e-3f)? clampf((safe_stock-stk2[g])/safe_stock, 0.f, 1.f) : 0.f;
            float prio = stress * pr * dvs;                      /* émergent : urgence × valeur × manque */
            if (prio>topp){ topp=prio; top=(Resource)g; }
        }
        v.top_flow=top; v.top_priority=topp;
    }
    return v;
}

float ai_consolidation_pressure(const AiView *v){
    /* fragile : la stabilité s'effondre (ordre qui ne tient plus). */
    float fragile = (v->SI < 5.f) ? 1.f : (v->fragilite > 5.f ? 0.6f : 0.f);
    /* surextension : on a avalé plus de diversité que le K ne métabolise. */
    float surext  = clampf(v->Dinf_interne / fmaxf(v->K, 1.f) - 1.f, 0.f, 1.f);
    /* tendu : l'ordre tient surtout par la contrainte (fragilité haute). */
    float tendu   = clampf((v->fragilite - 5.f)/5.f, 0.f, 1.f);
    /* déchiré : deux factions-éthos opposées paralysent la direction (§6) — un frein
     * INTERNE, sœur de la surextension culturelle : la surexpansion ligue le monde
     * dehors, l'incohérence d'éthos te ligue dedans. PLANCHER : un mono-éthos garde
     * une fracture résiduelle (les penchants s'étalent) — seule la fracture AU-DELÀ
     * de ce socle (avaler des éthos divergents) freine, pas la cohésion ordinaire. */
    float dechire = clampf((v->ethos_fracture - AI_ETHOS_FRACTURE_FLOOR)
                           / (1.f - AI_ETHOS_FRACTURE_FLOOR), 0.f, 1.f) * AI_ETHOS_FRACTURE_W;
    float p = fragile; if (surext>p) p=surext; if (tendu>p) p=tendu; if (dechire>p) p=dechire;
    return clampf(p, 0.f, 1.f);
}

#define NEED_W 0.7f   /* poids de la pression de besoin sur l'agression (surface d'équilibrage) */
/* §war-smoothing — l'intensité guerrière du MONDE : le nombre de PAIRES de pays en guerre.
 * Sert à LISSER la distribution (monde fendu → beaucoup de proies → spirale à 25 ; monde consolidé
 * → atone) : plus il y a de guerres, MOINS un empire en ajoute (saturation). */
static int ai_world_war_pairs(const World *w, const DiploState *d){
    int n=0;
    for (int a=0;a<w->n_countries && a<SCPS_MAX_COUNTRY;a++)
        for (int b=a+1;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++)
            if (diplo_status(d,a,b)==DIPLO_WAR) n++;
    return n;
}
/* ── PRÉVISION DIPLO — la menace ENTRANTE (qui va m'attaquer, et suis-je couvert ?), lue des
 * coordonnées : diplo_relation (menace de b sur moi) + ma puissance + la menace ambiante. PURE
 * (const), recalculée au tick, JAMAIS sérialisée — l'analogue diplomatique d'EconForecast. */
typedef struct {
    float threat_in_max;     /* la menace ENTRANTE la plus forte (mon pire voisin hostile) */
    float threat_in_sum;     /* masse de menace entrante (coalition potentielle) */
    int   threat_top;        /* le cid derrière threat_in_max (-1 = aucun) */
    float war_risk;          /* [0..1] = threat_in_max / (ma puissance + menace ambiante) */
    float alliance_need;     /* [0..1] urgence d'allié (menacé ET mon bloc ne couvre pas) */
    float war_outlook_worst; /* pire war_score sur mes guerres EN COURS (négatif = je perds) */
    int   losing_war;        /* 1 si une guerre en cours est nettement perdue */
} DiploForecast;
static DiploForecast ai_diplo_forecast(const World *w, const WorldEconomy *econ,
                                       const WorldProsperity *wp, const DiploState *d, int cid){
    DiploForecast f; memset(&f,0,sizeof f); f.threat_top=-1; f.war_outlook_worst=100.f;
    if (!w||!econ||!d||cid<0||cid>=w->n_countries) return f;
    float my_power = diplo_mil_power(w,econ,cid) + diplo_eco_power(wp,cid);
    float amb = (d->ambient_threat>1e-4f)? d->ambient_threat : 1.f;
    float allied = 0.f;
    for (int b=0;b<w->n_countries;b++){
        if (b==cid) continue;
        DiploStatus st = diplo_status(d,cid,b);
        if (st==DIPLO_ALLIED){ allied += diplo_mil_power(w,econ,b)+diplo_eco_power(wp,b); continue; }
        /* ENNEMI POTENTIEL : déjà en guerre, OU rancune contre moi, OU casus belli (b → moi). */
        bool menacing = (st==DIPLO_WAR) || diplo_rancor(d,b,cid)>0.f
                       || diplo_casus_belli(w,econ,wp,d,b,cid,RES_NONE)!=CB_NONE;
        if (!menacing) continue;
        Relation rel = diplo_relation(w,econ,wp,d,cid,b);   /* rel.threat = menace de b sur cid */
        f.threat_in_sum += rel.threat;
        if (rel.threat > f.threat_in_max){ f.threat_in_max=rel.threat; f.threat_top=b; }
        if (st==DIPLO_WAR){
            float ws = diplo_war_score(d,cid,b);
            if (ws < f.war_outlook_worst) f.war_outlook_worst = ws;
            if (ws < tune_f("AI_WAR_LOSING",-25.f) && d->war_years[cid][b]>0.f) f.losing_war=1;
        }
    }
    f.war_risk = clampf(f.threat_in_max/(my_power+amb), 0.f, 1.f);
    float cover = (f.threat_in_max>1e-4f)? clampf(allied/f.threat_in_max,0.f,1.f) : 1.f;
    f.alliance_need = clampf(f.war_risk*(1.f-cover), 0.f, 1.f);
    return f;
}

float ai_aggression(const AiActor *a, const AiView *v){
    float brake = ai_consolidation_pressure(v);
    float base  = a->w_expand + 0.5f*a->w_faith;
    /* L'agression ne lit plus QUE la fiche : un besoin AIGU dont le seul moyen
     * restant est PRENDRE (bien introuvable chez soi, donc à arracher) POUSSE à la
     * guerre — même un Mercantile bloqué escalade. Le frein la borne toujours
     * (un acteur fragile encaisse le manque plutôt que de se suicider). */
    float need_push = NEED_W * v->gap_acuity * v->take_pressure;
    return (base + need_push) * (1.f - brake);
}

/* ===================================================================== */
/* LECTEURS DE CIBLE (toujours sur des coordonnées existantes)            */
/* ===================================================================== */
static bool countries_adjacent(const WorldEconomy *econ, int a, int b){
    for (int r=0; r<econ->n_regions; r++) if (econ->region[r].owner==a)
        for (int s=0; s<econ->n_regions; s++)
            if (econ->region[s].owner==b && econ->adj[r][s]) return true;
    return false;
}
/* H3 — ADJACENCE MARITIME : a peut frapper b si l'un de ses ports côtiers est
 * à portée de courants d'une côte de b (≤ AI_SEA_WAR_MAX_DAYS).
 * On teste TOUS les ports de a (un pays peut s'étendre sur deux océans). */
static bool countries_sea_adjacent(const World *w, const WorldEconomy *econ, int a, int b){
    for (int r=0; r<econ->n_regions; r++){
        if (econ->region[r].owner!=a || !econ->region[r].coastal || econ->region[r].build.port<=0.f) continue;
        int ax,ay;
        if (!world_region_sea_anchor(w,r,&ax,&ay)) continue;
        for (int s=0; s<econ->n_regions; s++){
            if (econ->region[s].owner!=b || !econ->region[s].coastal) continue;
            int bx,by;
            if (!world_region_sea_anchor(w,s,&bx,&by)) continue;
            float d=world_sea_days(w,ax,ay,bx,by);
            if (d>=0.f && d<=AI_SEA_WAR_MAX_DAYS) return true;
        }
    }
    return false;
}

/* Meilleure cible de guerre : voisine, qu'on peut battre (pas de suicide),
 * pondérée par la menace qu'elle fait peser + un schisme si l'on est zélote. */
static int ai_pick_rival(const AiActor *a, const World *w, const WorldEconomy *econ,
                         const WorldProsperity *wp, const DiploState *diplo, float my_army,
                         Resource want){
    int best=-1; float bestscore=0.f;
    /* VALEUR SUBJECTIVE (pipeline diplo, étage 2) : on vise qui TIENT ce qu'on convoite, pas
     * seulement le faible/menaçant. Le forecast de `a` (runway/shortfall) se calcule UNE fois. */
    EconForecast fc; econ_country_forecast(econ, a->cid, tune_f("AI_PROJ_HORIZON",25.f), &fc);
    float covet_w = tune_f("AI_COVET_W", 0.5f);
    for (int b=0; b<w->n_countries; b++){
        if (b==a->cid) continue;
        if (w->country[b].role==POLITY_UNCLAIMED) continue;
        bool land_adj = countries_adjacent(econ, a->cid, b);
        bool sea_adj  = !land_adj && w && countries_sea_adjacent(w, econ, a->cid, b); /* H3 */
        if (!land_adj && !sea_adj) continue;
        if (diplo_status(diplo, a->cid, b)==DIPLO_ALLIED) continue;  /* on ne frappe pas un allié */
        if (!diplo_can_declare(diplo, a->cid, b)) continue;          /* TRÊVE : on n'enchaîne pas */
        if (diplo_casus_belli(w,econ,wp,diplo,a->cid,b,want)==CB_NONE) continue;  /* PAS DE CB → pas de guerre */
        float their_army = diplo_mil_power(w, econ, b);
        if (my_army < AI_ARMY_MARGIN*their_army) continue;     /* on n'attaque pas plus fort */
        Relation rel = diplo_relation(w, econ, wp, diplo, a->cid, b);
        float opportunism = my_army - their_army;              /* une proie faible = une occasion */
        /* FRICTION : frapper un protégé d'alliés puissants risque d'ÉLARGIR la
         * guerre → la cible se renchérit de la force alliée susceptible d'entrer.
         * Moins de guerres marginales ; le besoin aigu en vaut encore le risque. */
        float widen = diplo_war_widening_cost(w, econ, diplo, a->cid, b);
        /* On frappe ce qui MENACE — et, à proportion de l'appétit de conquête, ce
         * qui est FAIBLE. La RANCUNE pèse (on veut reprendre nos terres) ; la
         * parenté/alliance et le risque d'élargissement retiennent. */
        /* CROISADE : une foi orthodoxe a une CHANCE de frapper qui développe le
         * faustien — pesée par sa ferveur (w_faith). Gardiens vs Transgresseurs. */
        float crusade = diplo_faustian_cb(w,econ,diplo,a->cid,b) ? AI_CRUSADE_W*(0.4f+a->w_faith) : 0.f;
        /* CONVOITISE : la province la plus PRÉCIEUSE (subjectivement) que b me tient → on vise
         * qui détient ce qui me MANQUE (l'affamé fond sur le tenant du grenier). Émergent. */
        float coveted=0.f;
        for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==b){
            float v=ai_province_value(econ, a->cid, r, &fc); if (v>coveted) coveted=v; }
        float score = rel.threat
                    + a->w_expand * 3.0f * (opportunism>0.f ? opportunism : 0.f)
                    + a->w_faith  * 5.0f * rel.schism
                    + AI_RANCOR_W * diplo_rancor(diplo, a->cid, b)
                    + crusade
                    + covet_w * coveted                       /* VALEUR = cible (l'éthos décide la méthode) */
                    - rel.alliance
                    - AI_WIDEN_W * widen;
        if (sea_adj) score *= AI_SEA_WAR_PENALTY; /* H3 : outre-mer = logistique plus dure */
        if (score > bestscore){ bestscore=score; best=b; }
    }
    return best;
}

/* ---- Diplomatie d'équilibre : alliés, coalition (lectures, pas de script) -- */
static float allied_power(const World *w, const WorldEconomy *econ, const DiploState *d, int self){
    float p=0.f;
    for (int k=0;k<w->n_countries;k++) if (k!=self && diplo_status(d,self,k)==DIPLO_ALLIED)
        p += diplo_mil_power(w,econ,k);
    return p;
}
static bool country_at_war(const World *w, const DiploState *d, int c){
    for (int k=0;k<w->n_countries;k++) if (k!=c && diplo_status(d,c,k)==DIPLO_WAR) return true;
    return false;
}
/* L'allié naturel le plus fort (score d'alliance au-delà du seuil) — la friction
 * préventive : on se lie aux complémentaires/parents/menacés-communs. */
/* §D-sat : l'alliance CROISERAIT-elle une ligne de front ? On refuse de se lier à qui
 * fait la guerre à l'un de nos alliés (ou réciproquement) — sinon on est lié des DEUX
 * côtés d'un même conflit et la toile se forme au lieu de camps cohérents. */
static bool crosses_existing(const World *w, const DiploState *d, int a, int cand){
    for (int k=0;k<w->n_countries;k++){
        if (k==a||k==cand) continue;
        if (diplo_status(d,a,k)==DIPLO_ALLIED && diplo_status(d,cand,k)==DIPLO_WAR) return true; /* il frappe mon allié */
        if (diplo_status(d,cand,k)==DIPLO_ALLIED && diplo_status(d,a,k)==DIPLO_WAR) return true; /* je frappe le sien */
    }
    return false;
}
/* §D-sat : l'allié actuel le plus FAIBLE (score d'alliance le plus bas) — candidat à
 * l'éviction quand les slots sont pleins et qu'un meilleur se présente. */
static int weakest_ally(const AiActor *a, const World *w, const WorldEconomy *econ,
                        const WorldProsperity *wp, const DiploState *d, float *out_score){
    int worst=-1; float wv=1e9f;
    for (int b=0;b<w->n_countries;b++){
        if (b==a->cid || diplo_status(d,a->cid,b)!=DIPLO_ALLIED) continue;
        Relation rel=diplo_relation(w,econ,wp,d,a->cid,b);
        if (rel.alliance<wv){ wv=rel.alliance; worst=b; }
    }
    if (out_score) *out_score = (worst>=0)? wv : 0.f;
    return worst;
}
/* #26 — `to` ÉVALUE une OFFRE de `from` et l'ACCEPTE/REFUSE (le « code IA évaluer-offre »).
 * Lu de l'OPINION ±100 (mémoire des actes) + la relation STRUCTURELLE + le score de guerre.
 * sc == NULL ⇒ pas de porte d'opinion (décision relation-seule, rétro-compatible bancs). */
bool ai_consider_offer(const World *w, const WorldEconomy *econ, const WorldProsperity *wp,
                       const DiploState *d, const Statecraft *sc, int from, int to, OfferKind kind){
    if (!w||!econ||!wp||!d) return false;
    if (from<0||to<0||from==to||from>=w->n_countries||to>=w->n_countries) return false;
    if (w->country[to].role==POLITY_UNCLAIMED || w->country[from].role==POLITY_UNCLAIMED) return false;
    int op = sc ? statecraft_opinion(sc, to, from) : 0;   /* ce que `to` pense de `from` (l'offrant) */
    switch (kind){
        case OFFER_ALLIANCE:
            if (diplo_status(d,to,from)==DIPLO_WAR) return false;                 /* pas d'alliance avec un ennemi actif */
            if (diplo_ally_count(d,to) >= DIPLO_ALLY_SLOTS) return false;         /* plus de slot libre */
            if (sc && op < (int)tune_f("AI_OFFER_ALLY_OPINION",10.f)) return false;   /* opinion trop basse → REFUS */
            return diplo_relation(w,econ,wp,d,to,from).alliance > 0.f;            /* + compatibilité réciproque */
        case OFFER_PEACE: {
            if (diplo_status(d,to,from)!=DIPLO_WAR) return true;                  /* déjà en paix : trivialement oui */
            float sf  = diplo_war_score(d, from, to);                            /* score de l'OFFRANT contre `to` */
            float yrs = d->war_years[from][to];
            return sf >= tune_f("AI_WAR_DECISIVE",50.f)*0.5f || yrs >= tune_f("AI_WAR_EXHAUST",10.f);
        }                                                                        /* `to` cède s'il PERD ou s'épuise */
        case OFFER_TRADE_PACT:
            if (diplo_status(d,to,from)==DIPLO_WAR) return false;
            if (sc && op < (int)tune_f("AI_OFFER_PACT_OPINION",0.f)) return false;
            return diplo_relation(w,econ,wp,d,to,from).complement > 0.1f;         /* un pacte = un intérêt commercial */
        case OFFER_MIGRATION:
            /* BRASSAGE — un CRAN de confiance AU-DESSUS du commercial : ouvrir ses frontières
             * à la population de l'autre. `to` CONSENT s'il est ALLIÉ de l'offrant OU si son
             * opinion est HAUTE (AI_OFFER_MIG_OPINION 40). Jamais en guerre. */
            if (diplo_status(d,to,from)==DIPLO_WAR) return false;
            if (diplo_status(d,to,from)==DIPLO_ALLIED) return true;               /* l'alliance suffit */
            return sc && op >= (int)tune_f("AI_OFFER_MIG_OPINION",40.f);          /* sinon, forte confiance */
    }
    return false;
}
static int ai_pick_ally(const AiActor *a, const World *w, const WorldEconomy *econ,
                        const WorldProsperity *wp, const DiploState *d, const Statecraft *sc){
    int best=-1; float bestsc=AI_ALLY_SEUIL;
    /* PRÉVISION DIPLO — BESOIN D'ALLIÉ : menacé ET mon bloc ne couvre pas (alliance_need haut) → je
     * COURTISE en priorité la puissance qui COUVRE ma pire menace, au lieu de la seule affinité.
     * Gaté (need>0.5, rare <12 ans) ⇒ la fenêtre golden reste byte-identique. Le consentement (#26)
     * et les slots restent les portes INCHANGÉES — le forecast ne change que l'URGENCE de sélection. */
    DiploForecast a_dfc = ai_diplo_forecast(w, econ, wp, d, a->cid);
    for (int b=0;b<w->n_countries;b++){
        if (b==a->cid || w->country[b].role==POLITY_UNCLAIMED) continue;
        if (diplo_status(d,a->cid,b)!=DIPLO_NEUTRAL) continue;     /* déjà allié ou en guerre */
        if (!countries_adjacent(econ,a->cid,b)) continue;
        if (diplo_ally_count(d,b) >= DIPLO_ALLY_SLOTS) continue;           /* §D-sat : le candidat n'a plus de slot */
        if (crosses_existing(w,d,a->cid,b)) continue;             /* §D-sat : pas d'alliance croisée */
        if (!ai_consider_offer(w,econ,wp,d,sc, a->cid, b, OFFER_ALLIANCE)) continue;  /* #26 : `b` CONSENT-il ? (bilatéral) */
        Relation rel=diplo_relation(w,econ,wp,d,a->cid,b);
        float score=rel.alliance;
        if (a_dfc.alliance_need > 0.5f){                          /* menacé & sous-couvert → URGENCE */
            float ally_help = clampf((diplo_mil_power(w,econ,b)+diplo_eco_power(wp,b))
                                     /(a_dfc.threat_in_max+1e-4f), 0.f, 1.f);
            score += tune_f("AI_ALLY_NEED_W", 1.0f) * a_dfc.alliance_need * ally_help;
        }
        if (score>bestsc){ bestsc=score; best=b; }
    }
    return best;
}

/* (ai_pick_enemy_region retiré : l'IA ne désigne plus de cible de conquête abstraite —
 * le TERRAIN décide qui occupe quoi ; le règlement §terrain transfère l'occupé.) */

static float content_dist(const PopCulture *a, const PopCulture *b){
    float dv=fabsf(a->valeurs-b->valeurs),   ds=fabsf(a->subsistance-b->subsistance);
    float dp=fabsf(a->parente-b->parente),   dr=fabsf(a->religion-b->religion);
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}
/* Partenaire commercial : région étrangère peuplée dont la distance de contenu
 * approche le PIC de la cloche (D̄≈5 : le plus à échanger). On ne déduplique pas
 * — rouvrir la même artère, c'est l'INTENSIFIER (un négociant y revient). */
static int ai_pick_trade_partner(const WorldEconomy *econ, const RouteNetwork *rn,
                                 int home_region, int cid){
    if (home_region<0 || home_region>=econ->n_regions) return -1;
    const PopCulture *hc = &econ->region[home_region].culture;
    int best=-1; float bestgap=1e9f;
    for (int r=0; r<econ->n_regions; r++){
        const RegionEconomy *re = &econ->region[r];
        if (r==home_region || re->owner==cid) continue;
        if (!re->culture.settled || re->impassable) continue;
        if (rn){ bool deja=false;            /* une route par paire : viser un partenaire NEUF */
            for (int i=0;i<rn->n;i++){ const TradeRoute *t=&rn->route[i];
                if ((t->ra==home_region&&t->rb==r)||(t->ra==r&&t->rb==home_region)){ deja=true; break; } }
            if (deja) continue; }
        float gap = fabsf(content_dist(hc, &re->culture) - 5.f);
        /* commerce asym. §5 : les positions d'AVAL valent plus (estuaires,
         * terminus portuaires — là où le vrac converge et où tout s'achète). */
        if (re->estuary)                      gap -= 0.6f;
        else if (re->coastal && re->build.port>0.f) gap -= 0.3f;
        if (gap < bestgap){ bestgap=gap; best=r; }
    }
    return best;
}

/* H3 — L'IA VEUT LA MER. Un port VRAI tenu par le pays (le quai d'où part la coque). */
static bool ai_true_port(const WorldEconomy *econ, int r){
    return r>=0 && r<econ->n_regions && econ->region[r].build.port>0.f && econ->region[r].coastal;
}
static int ai_owned_port(const WorldEconomy *econ, int cid){
    int best=-1; float bp=-1.f;
    for (int r=0;r<econ->n_regions;r++)
        if (econ->region[r].owner==cid && ai_true_port(econ,r) && econ->region[r].route_pe>bp){
            bp=econ->region[r].route_pe; best=r; }
    return best;
}
/* Un port ÉTRANGER, neuf (pas déjà relié à `myport`). routes_order valide la portée
 * de courants (≤ SEA_ROUTE_MAX_DAYS). Seuil ÷2 vs terre : on accepte le partenaire le
 * plus PROCHE culturellement OU le plus aval — la 1re liaison crée le marché. */
static int ai_pick_sea_partner(const WorldEconomy *econ, const RouteNetwork *rn,
                               int myport, int cid){
    if (!ai_true_port(econ,myport)) return -1;
    const PopCulture *hc=&econ->region[myport].culture;
    int best=-1; float bestgap=1e9f;
    for (int r=0;r<econ->n_regions;r++){
        if (r==myport || econ->region[r].owner==cid) continue;
        if (!ai_true_port(econ,r) || !econ->region[r].culture.settled) continue;
        if (rn){ bool deja=false;
            for (int i=0;i<rn->n;i++){ const TradeRoute *t=&rn->route[i];
                if ((t->ra==myport&&t->rb==r)||(t->ra==r&&t->rb==myport)){ deja=true; break; } }
            if (deja) continue; }
        float gap=fabsf(content_dist(hc,&econ->region[r].culture)-5.f);
        if (econ->region[r].estuary) gap-=0.6f;          /* l'aval vaut plus */
        if (gap<bestgap){ bestgap=gap; best=r; }
    }
    return best;
}

/* Progression institutionnelle K : Tribunal → Chancellerie → Académie. */
static Edifice ai_next_k_edifice(const WorldEconomy *econ, int region){
    if (region<0 || region>=econ->n_regions) return EDI_TRIBUNAL;
    float k = econ->region[region].build.K_inst;
    if (k < 1.0f) return EDI_TRIBUNAL;
    if (k < 2.5f) return EDI_CHANCELLERIE;
    return EDI_ACADEMIE;
}
/* Progression coercitive H : Garnison → Forteresse → Citadelle (le chemin de
 * l'Ordre de Fer : tenir par la force au lieu de métaboliser). */
static Edifice ai_next_h_edifice(const WorldEconomy *econ, int region){
    if (region<0 || region>=econ->n_regions) return EDI_GARNISON;
    float h = econ->region[region].build.H_coerc;
    if (h < 1.0f) return EDI_GARNISON;
    if (h < 3.0f) return EDI_FORTERESSE;
    return EDI_CITADELLE;
}
/* Progression de foi : Sanctuaire → Temple → Cathédrale (sacraliser → SOUTIENT L). */
static Edifice ai_next_faith_edifice(const WorldEconomy *econ, int region){
    if (region<0 || region>=econ->n_regions) return EDI_SANCTUAIRE;
    float f = econ->region[region].build.faith;
    if (f < 1.0f) return EDI_SANCTUAIRE;
    if (f < 3.0f) return EDI_TEMPLE;
    return EDI_CATHEDRALE;
}
/* Progression du savoir : Bibliothèque → Monastère (recherche ; le monastère aussi foi). */
static Edifice ai_next_savoir_edifice(const WorldEconomy *econ, int region){
    if (region<0 || region>=econ->n_regions) return EDI_BIBLIOTHEQUE;
    return (econ->region[region].build.savoir < 1.5f) ? EDI_BIBLIOTHEQUE : EDI_MONASTERE;
}
#define AI_FAITH_L 3.0f   /* consentement DÉFAILLANT (Légit<30) → le trône se SACRALISE */
#define AI_FAITH_ZEAL 0.5f /* ZÈLE : un crédo prosélyte (évangéliste 0.6 / purificateur 1.0 ⇒ w_faith haut)
                            * FONDE sa foi PROACTIVEMENT (pas seulement en crise) ; pluraliste 0.1 jamais */
#define AI_SAVOIR_K 2.5f  /* B3 : dès la Chancellerie (K≥2.5) posée, un centre établi se dote
                           * d'une ŒUVRE DE SAVOIR (Bibliothèque 360 → Monastère 540 — qui
                           * consomme des OUTILS, §B2) AVANT de couronner par l'Académie (960).
                           * Sinon le métabolisme K SAUTE le palier 540 (360→960) — la chronique
                           * montrait le 960 bâti sans jamais toucher le 540. */

/* ===================================================================== */
/* TOURS DE DÉCISION                                                       */
/* ===================================================================== */
/* §4 — la famille d'un édifice désigne la faction qu'il AVANCE (bâtir = voter). */
static EthosFaction ai_lever_for_edifice(Edifice e){
    switch (e){
        case EDI_GARNISON: case EDI_FORTERESSE: case EDI_CITADELLE:        return FAC_CONQUERANT;
        case EDI_SANCTUAIRE: case EDI_TEMPLE: case EDI_CATHEDRALE: case EDI_MONASTERE: return FAC_GARDIEN;
        case EDI_GRENIER: case EDI_IRRIGATION: case EDI_AQUEDUC:           return FAC_COMMUNAUTAIRE;
        case EDI_MARCHE: case EDI_ENTREPOT: case EDI_PORT: case EDI_CARAVANSERAIL:
        case EDI_COMPTOIR: case EDI_BANQUE:                               return FAC_MARCHAND;
        default:                                                          return FAC_LEGISTE;  /* Tribunal/Académie/Bibliothèque… */
    }
}

/* §reloc — l'IA PEUPLE sa province-ressource SOUS-EXPLOITÉE pour combler une pénurie.
 * Réveille l'actionneur econ_relocate_pop (jamais appelé) : on déplace du bras d'un RÉSERVOIR
 * (la province la plus peuplée) vers une province qu'on POSSÈDE, qui porte un raw dont l'empire
 * est COURT (demande > offre+stock) mais qu'il sous-extrait faute de bras (grande MARGE
 * d'intensité). Ensemencement MESURÉ, pondéré par l'ÉTHOS (la coercition coûte du H, §5) : le
 * martial déporte volontiers vers ses mines, le marchand/pacifiste répugne et reste court.
 * Pour l'empire qui A la terre de fer mais ne la peuple pas, c'est l'issue ; le marché tient ensuite (§3). */
static void ai_relocate_turn(AiActor *a, WorldEconomy *econ, const AiView *v, int day){
    if (day < a->next_reloc_day) return;
    /* VOLONTÉ ∝ éthos (via les poids effectifs) : conquête haute → déporte ; commerce haut → répugne. */
    float w_reloc = 0.20f + 0.65f*a->w_expand - 0.45f*a->w_trade + 0.20f*a->w_build;
    if (w_reloc < 0.15f) return;                          /* consentuel : reste court, commerce d'abord */
    /* Agrégats par ressource sur les régions POSSÉDÉES (offre/demande/stock) + le RÉSERVOIR (la
     * plus peuplée). NB : le bon signal n'est PAS chain_gap (= AUCUN raw du tout, donc rien à
     * peupler) mais « j'ai la TERRE du raw mais le SOUS-EXTRAIS » : demande > offre+stock ALORS
     * qu'une de mes provinces porte ce raw, sous-peuplée. C'est elle, la montagne de fer à moitié vide. */
    static float agg_s[RES_COUNT], agg_d[RES_COUNT], agg_k[RES_COUNT];
    for (int g=0;g<RES_COUNT;g++){ agg_s[g]=agg_d[g]=agg_k[g]=0.f; }
    int src=-1; float src_pop=0.f;
    for (int r=0;r<econ->n_regions;r++){
        RegionEconomy *re=&econ->region[r];
        if (re->owner!=a->cid || !re->colonized) continue;
        for (int g=1;g<RES_COUNT;g++){ agg_s[g]+=re->supply[g]; agg_d[g]+=re->demand[g]; agg_k[g]+=re->stock[g]; }
        float lab=re->strata[CLASS_LABORER].pop;
        if (lab>src_pop){ src_pop=lab; src=r; }            /* réservoir = la plus peuplée */
    }
    if (src<0 || src_pop < 2.0f*AI_RELOC_FLOOR) return;   /* pas de réservoir → on ne vide pas un hameau */
    int tgt=-1; float best=0.f;
    /* §dév — D'ABORD LE PALIER : on PEUPLE la colonie la plus SOUS le seuil (min(1000, cap_pop)) pour
     * REMPLIR LES JOBS de ses manufactures et viser l'OPTIMUM démographique. La cible = le plus gros
     * déficit au palier (habitable, ≠ réservoir, sous sa capacité d'accueil). */
    for (int r=0;r<econ->n_regions;r++){
        RegionEconomy *re=&econ->region[r];
        if (re->owner!=a->cid || !re->colonized || re->impassable || r==src) continue;
        if (re->habitability < 0.20f) continue;            /* jamais l'infranchissable/quasi-mort */
        float pop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        float floor=fminf(AI_COLONY_FLOOR, re->cap_pop);   /* le palier, borné par la capacité d'accueil */
        if (pop >= floor) continue;                        /* déjà au palier (ou cap atteint) */
        float deficit=floor-pop;
        if (deficit > best){ best=deficit; tgt=r; }
    }
    /* SINON — la logique d'origine : combler une PÉNURIE de raw qu'on SOUS-EXTRAIT (la montagne de fer
     * à moitié vide). score = manque × marge × richesse du gisement, la plus sous-peuplée. */
    if (tgt<0) for (int r=0;r<econ->n_regions;r++){
        RegionEconomy *re=&econ->region[r];
        if (re->owner!=a->cid || !re->colonized || re->impassable || r==src) continue;
        if (re->habitability < 0.20f) continue;
        float lab=re->strata[CLASS_LABORER].pop;
        if (lab > 2.0f*AI_RELOC_SEED) continue;            /* déjà bien peuplée → pas de marge */
        float margin = (AI_RELOC_SEED - lab)/AI_RELOC_SEED; if (margin<0.f) margin=0.f;
        for (int g=1;g<RES_COUNT;g++){
            if (re->raw_cap[g] <= 0.f) continue;
            /* shortfall ANTICIPÉ (étage 3) : le manque immédiat OU le projeté (forecast) — on
             * peuple la province-ressource AVANT le mur, pas une fois la pénurie installée. */
            float shortfall = fmaxf(agg_d[g] - (agg_s[g]+agg_k[g]), v->fc.shortfall_proj[g]);
            if (shortfall <= 0.5f) continue;               /* l'empire n'est pas court de ce raw */
            float score = shortfall * (0.4f+margin) * re->raw_cap[g];
            if (score > best){ best=score; tgt=r; }
        }
    }
    if (tgt<0) return;                                     /* rien à peupler ni à amorcer */
    if (frand(&a->rng) > w_reloc) return;                  /* la volonté n'emporte pas ce tour-ci */
    float seed = fminf(0.25f*src_pop, AI_RELOC_SEED);      /* ensemencement mesuré (≤ une bande) */
    if (seed < AI_RELOC_FLOOR) return;
    econ_relocate_pop(econ, src, tgt, seed);              /* monte la coercition à la source (§5) */
    a->stats.relocations++;
    a->next_reloc_day = day + AI_RELOC_COOLDOWN;          /* ensemencer, pas pomper (§3) */
}

/* (définis plus bas — utilisés par les leviers) */
static Ethos ai_capital_ethos(const World *w, const WorldEconomy *econ, int cid);
static int   ai_owned_regions(const WorldEconomy *econ, int cid);

/* ═══ LEVIERS INTÉRIEURS — l'IA en use selon son ÉTHOS et sa SITUATION (brief §4).
 * Jamais au hasard : le tempérament penche, le déclencheur arme. La purge est RARE
 * (seuil très haut + long verrou) — un événement de récit, pas une routine. ═══ */
#define AI_INTERIOR_CADENCE 900     /* ~2.5 ans entre deux leviers intérieurs */
#define AI_PURGE_LOCK       14600   /* ~40 ans : une purge par génération AU PLUS */
static void ai_interior_turn(AiActor *a, const World *w, WorldEconomy *econ,
                             AgencyState *ag, const DiploState *dp, const AiView *v, int day){
    if (day < a->next_interior_day) return;
    Ethos eth = ai_capital_ethos(w, econ, a->cid);
    bool at_war=false;
    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++)
        if (b!=a->cid && diplo_status(dp,a->cid,b)==DIPLO_WAR){ at_war=true; break; }
    /* balayer SES provinces : la pire agitation, la plus grosse minorité mal intégrée.
     * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir
     * périmé/vide selon l'ordre des tick (cf. econ_aggregate_regions). worst_min nourrit
     * agency_order_assimilate/_purge (region id), dont l'effet RÉEL atterrit déjà sur la
     * province représentative (apply_action/purge_slice, scps_agency.c) — on juge donc SUR
     * LA MÊME province pour que la cible choisie soit celle qui sera vraiment traitée. */
    int worst_agit=-1; float worst_sat=1.f;
    int worst_min=-1;  long  min_count=0; float min_integ=1.f;
    for (int r=0;r<econ->n_regions;r++){
        RegionEconomy *re=&econ->region[r];
        if (re->owner!=a->cid || !re->colonized) continue;
        if (re->satisfaction<worst_sat && re->coercion<0.30f){ worst_sat=re->satisfaction; worst_agit=r; }
        int rpid=econ_region_rep_province(econ, r);
        const ProvincePop *pp=(rpid>=0 && rpid<econ->n_prov) ? &econ->prov[rpid].pop : NULL;
        if (pp && pp->n_groups>=2){
            int dom=0; for (int g=1;g<pp->n_groups;g++) if (pp->groups[g].count>pp->groups[dom].count) dom=g;
            long tot=0; for (int g=0;g<pp->n_groups;g++) tot+=pp->groups[g].count;
            for (int g=0;g<pp->n_groups;g++){
                if (g==dom) continue;
                const PopGroup *pg=&pp->groups[g];
                if (tot>0 && pg->count*3 >= tot && pg->count>min_count){
                    min_count=pg->count; min_integ=pg->integration; worst_min=r;
                }
            }
        }
    }
    /* MATER — fermeté martiale : agitation profonde, la douceur a échoué (ou n'est
     * pas son genre). Dominateur/Honneur/Ordre volontiers ; Bureaucrate en dernier
     * recours ; Pacifiste à l'ultime (et il lèvera vite). */
    bool martial = (eth==ETHOS_DOMINATEUR||eth==ETHOS_HONNEUR||eth==ETHOS_ORDRE);
    float repress_seuil = martial?0.25f : (eth==ETHOS_BUREAUCRATE)?0.12f : 0.08f;
    if (worst_agit>=0 && worst_sat<repress_seuil){
        if (agency_order_repress(ag, worst_agit)){ a->next_interior_day=day+AI_INTERIOR_CADENCE; return; }
    }
    /* FORMER — l'art du Bureaucrate (le Creuset) : une grosse minorité mal intégrée,
     * pas de guerre en cours (on ne scolarise pas sous les bombes). */
    bool integrateur = (eth==ETHOS_BUREAUCRATE||eth==ETHOS_ORDRE||eth==ETHOS_PACIFISTE);
    if (!at_war && worst_min>=0 && min_integ<0.40f && (integrateur || a->has_creuset)){
        if (agency_order_assimilate(ag, worst_min, a->has_creuset)){
            a->next_interior_day=day+AI_INTERIOR_CADENCE; return; }
    }
    /* PURGER — seuil TRÈS haut : credo purificateur au trône, OU Dominateur en crise
     * de cohésion avec une minorité massive et inassimilée. Long verrou (génération).
     * Pacifiste/Mercantile/Bureaucrate : jamais (leur IA — le joueur, lui, peut). */
    if (day>=a->next_purge_ok_day && worst_min>=0){
        int cr=(a->home_region>=0&&a->home_region<econ->n_regions)?a->home_region:-1;
        bool purificateur = (cr>=0 && econ->region[cr].culture.credo==CREDO_PURIFICATEUR);
        bool dominateur_crise = (eth==ETHOS_DOMINATEUR && v->fracture>6.f && min_integ<0.15f);
        if (purificateur || dominateur_crise){
            if (agency_order_purge(ag, worst_min)){
                a->next_purge_ok_day=day+AI_PURGE_LOCK;
                a->next_interior_day=day+AI_INTERIOR_CADENCE; return;
            }
        }
    }
    a->next_interior_day=day+AI_INTERIOR_CADENCE/2;   /* rien à faire : on repasse plus tôt */
}

/* §leviers — IMPOSER À LA VICTOIRE (au lieu d'annexer) : l'éthos choisit le contrat.
 * Le Dominateur/Honneur font des SERFS ; l'Ordre des PROTECTORATS (glacis) ; le
 * Mercantile lie les CITÉS ; le Bureaucrate prend un protectorat s'il est déjà large
 * (un vassal vaut mieux que des provinces ingouvernables) ; le Pacifiste signe et part. */
static void ai_impose_contract(AiActor *a, const World *w, WorldEconomy *econ,
                               DiploState *dp, int loser){
    if (loser<0 || loser>=w->n_countries || diplo_suzerain(dp,loser)>=0) return;
    Ethos eth = ai_capital_ethos(w, econ, a->cid);
    SuzContrat c=CONTRAT_NONE;
    if (eth==ETHOS_DOMINATEUR || eth==ETHOS_HONNEUR) c=CONTRAT_SERVAGE;
    else if (eth==ETHOS_ORDRE) c=CONTRAT_PROTECTORAT;
    else if (eth==ETHOS_MERCANTILE && w->country[loser].role==POLITY_CITY_STATE) c=CONTRAT_CITE;
    else if (eth==ETHOS_BUREAUCRATE && ai_owned_regions(econ,a->cid)>=7) c=CONTRAT_PROTECTORAT;
    if (c!=CONTRAT_NONE) diplo_set_vassal(dp, a->cid, loser, c);
}

/* L'empire EXTRAIT-il `raw` quelque part ? — avec le pool national la matière est fongible, donc
 * la simple PRÉSENCE d'un gisement suffit (la fabrique se pose où l'on veut, le pool la nourrit). */
static bool empire_has_raw(const WorldEconomy *econ, int cid, Resource raw){
    for (int r=0;r<econ->n_regions;r++)
        if (econ->region[r].owner==cid && econ->region[r].raw_cap[raw]>0.f) return true;
    return false;
}
/* L'empire a-t-il DÉJÀ cette fabrique quelque part ? (la chaîne à feu se pose UNE fois.) */
static bool empire_has_bld(const WorldEconomy *econ, int cid, BuildingType b){
    for (int r=0;r<econ->n_regions;r++){
        if (econ->region[r].owner!=cid) continue;
        const RegionEconomy *re=&econ->region[r];
        for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==b) return true;
    }
    return false;
}

/* F-arc — POSER LES MANUFACTURES MILITAIRES PAR DOCTRINE (bâti DÉLIBÉRÉ, gaté par le TIER de la
 * RÉGION-HÔTE + la PUISSANCE ÉCONOMIQUE = l'or). « Combien puis-je poser ? » = ce que le trésor paie ;
 * « par doctrine » : MARTIALE (Dominateur/Honneur/ORDRE — l'État de discipline, le plus apte au feu)
 * → feu + lourd · ARCANIQUE (appétit faustien) → forge céleste + atelier de mage · FLUIDE → atelier
 * d'arc (trait). La chaîne à feu se POSE ENTIÈRE (poudrière + charbonnière), sinon c'est du feu mort.
 * Une fabrique par tour ; les troupes suivent à la levée. Pas d'auto-bâti : c'est un CHOIX payé. */
static void ai_build_manufacture(AiActor *a, const World *w, WorldEconomy *econ){
    int cap=a->home_region;
    if (cap<0 || cap>=econ->n_regions) return;
    RegionEconomy *cre=&econ->region[cap];
    /* « Quelle puissance économique ? » = une RÉGION-HÔTE assez développée (T-gate PAR RÉGION, dans
     * la boucle) ; le trésor gate COMBIEN. Avec le pool national, plus besoin que ce soit la CAPITALE :
     * une province-fer développée héberge la chaîne, et la matière de l'empire y afflue. */
    Ethos eth=ai_capital_ethos(w, econ, a->cid);
    BuildingType want[2]; int nw=0;
    if (eth==ETHOS_DOMINATEUR || eth==ETHOS_HONNEUR || eth==ETHOS_ORDRE){
        /* MARTIALE (Dominateur · Honneur · ORDRE — l'État de DISCIPLINE, le drill à poudre : le type le
         * plus apte à la chaîne à feu) — le FEU d'abord, mais SEULEMENT s'il peut TOURNER : poudre
         * découverte (TECH_POUDRIERE) + salpêtre dans l'empire (le pool national le rend fongible) +
         * chaîne pas encore posée. La capacité-clé s'établit UNE fois ; ensuite l'armurerie lourde
         * (fer, toujours dispo) s'étend. Sans tech/salpêtre : le lourd seul — jamais de feu mort. */
        if (cre->tech_arquebus && empire_has_raw(econ, a->cid, RES_SALTPETER)
            && !empire_has_bld(econ, a->cid, BLD_ARQUEBUS))
            want[nw++]=BLD_ARQUEBUS;
        want[nw++]=BLD_ARMORY_HEAVY;
    }
    else if (a->w_faustian > 0.30f){ want[nw++]=BLD_CELESTIAL_FORGE; want[nw++]=BLD_MAGE_WORKSHOP; }          /* arcanique */
    else { want[nw++]=BLD_BOWYER; }                                                                           /* fluide : trait */
    for (int k=0;k<nw;k++){
        BuildingType b=want[k];
        int btier=bld_min_tier(b);
        /* POSER dans la région-RESSOURCE (l'intrant primaire) — sinon la fabrique ne produit pas
         * (régions spécialisées) : fer pour l'armurerie/arc/arquebuse, cristal/fer céleste pour l'arcane. */
        Resource in=(b==BLD_MAGE_WORKSHOP)?RES_ARCANE_CRYSTAL:(b==BLD_CELESTIAL_FORGE)?RES_CELESTIAL_IRON:RES_IRON;
        int best=-1; float bestcap=0.1f;
        for (int r=0;r<econ->n_regions;r++){
            RegionEconomy *re=&econ->region[r];
            if (re->owner!=a->cid) continue;
            bool have=false; for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==b){ have=true; break; }
            if (have) continue;
            float rpop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
            if (capitale_max_tier((long)rpop) < btier) continue;          /* T-gate PAR RÉGION-HÔTE : la province doit pouvoir héberger ce tier */
            if (rpop < AI_STAFF_PER_MANUF*(float)(re->n_bld+1)) continue;  /* SOUS-STAFFÉ : pas dans le vide */
            if (re->raw_cap[in] > bestcap){ bestcap=re->raw_cap[in]; best=r; }
        }
        if (best<0) continue;                                  /* pas de région avec l'intrant (ou déjà bâtie partout) */
        float cost=tune_f("MANUF_BUILD_COST",50.f)*(float)bld_min_tier(b)*econ_world_ipm(econ);
        if (!credit_can_spend(econ, w, a->cid, cost)) continue;   /* bloque seulement au-delà de la ligne de crédit (dette) */
        if (econ_build_manufacture(econ, best, b)){
            credit_spend(econ, w, a->cid, cost); econ_flux_add(a->cid, FX_SOLDE, -cost);   /* débit AU SUCCÈS — pas d'or perdu si la pose échoue */
            a->stats.builds_other++;
            /* FINIR LA CHAÎNE (comme l'armurier à poudre des cités-états) : poudrière (salpêtre+charbon
             * → poudre) + charbonnière (bois → charbon). Le pool national amène le salpêtre d'où qu'il
             * tombe ; le feu a enfin sa poudre, l'arquebusier paraît — au lieu d'une fabrique muette. */
            if (b==BLD_ARQUEBUS){ econ_build_manufacture(econ, best, BLD_POWDERMILL);
                                  econ_build_manufacture(econ, best, BLD_CHARCOAL); }
        }
        return;                                                /* une fabrique par tour */
    }
}

/* L'IA DÉVELOPPE — la VOLONTÉ de bâtir, PILOTÉE PAR LE LOGEMENT (§dev). Une manufacture LOGE juste en
 * étant bâtie (econ Q6 : eff_cap += Σniveaux·HOUSE_MANUF, plafond ½·cap_pop) — donc on bâtit dans la
 * province la PLUS SOUS-LOGÉE, pas dans la capitale-gisement (qui sur-bâtissait AU-DELÀ du plafond
 * pendant que les provinces pauvres en brut restaient à ½ à vie : cette CONCENTRATION était le vrai
 * verrou du ½→plein, pas la cadence ni le crédit). L'intrant n'a PAS à être extrait sur place : le pool
 * national P1 nourrit la fabrique d'où que tombe la matière. Tier-gatée, staffée (pas dans le vide). Une
 * par tour. C'est ce qui DOUBLE les provinces vers leur plein — la pop suit le bâti. */
static void ai_build_civmanuf(AiActor *a, const World *w, WorldEconomy *econ){
    int cap=a->home_region; if (cap<0||cap>=econ->n_regions) return;
    const float house_manuf = tune_f("HOUSE_MANUF", 100.f);
    int br=-1; BuildingType bb=BLD_TYPE_COUNT; float best_deficit=house_manuf;  /* ≥ 1 niveau de logement manquant pour agir */
    for (int r=0;r<econ->n_regions;r++){
        RegionEconomy *re=&econ->region[r];
        if (re->owner!=a->cid || !re->colonized) continue;
        float rpop=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        if (rpop < AI_STAFF_PER_MANUF*(float)(re->n_bld+1)) continue;  /* SOUS-STAFFÉ : on NE bâtit PAS dans le vide */
        /* §dev — LE DÉFICIT DE LOGEMENT pilote (et non le gisement) : combien de logement bâti il
         * manque encore pour le plein (½·cap_pop). On vise la province la PLUS sous-logée. */
        float manuf_h=0.f; for (int i=0;i<re->n_bld;i++) manuf_h += re->bld[i].level;
        float deficit = re->cap_pop*0.5f - manuf_h*house_manuf;
        if (deficit <= best_deficit) continue;                        /* déjà (presque) plein, ou moins sous-logé qu'un candidat */
        int rtier=capitale_max_tier((long)rpop);
        BuildingType pick=BLD_TYPE_COUNT; float pick_raw=-1.f;
        for (int b=0;b<BLD_TYPE_COUNT;b++){
            if (bld_is_faustian((BuildingType)b)) continue;          /* pas les transmuteurs (charge/tech) */
            if (rtier < bld_min_tier((BuildingType)b)) continue;      /* province trop petite pour cette fabrique */
            Resource in1,in2,out; building_recipe((BuildingType)b,&in1,&in2,&out);
            if (out==RES_NONE || in1==RES_NONE) continue;            /* pas une manufacture à intrant */
            /* le CIVIL seulement : l'armement & l'arcane sont la voie DOCTRINALE (ai_build_manufacture,
             * tier-gatée, mesurée) — les semer partout gonflait mil_power (poudre/armes lues par la
             * diplomatie) → emballement des guerres. Le civil métabolise le brut sans armer le monde. */
            if (out==RES_ARMS || out==RES_ARMS_HEAVY || out==RES_ARMS_RANGED || out==RES_FIREARM
                || out==RES_GUNPOWDER || out==RES_ENCHANTED_ARMS || out==RES_ESSENCE || out==RES_FLUX) continue;
            bool have=false; for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==(BuildingType)b){ have=true; break; }
            if (have) continue;                                       /* slot déjà rempli */
            /* §dev — l'intrant doit être NOURRISSABLE : extrait ICI, OU produit ailleurs dans l'empire (le
             * pool P1 l'amène). Fin du gate « gisement sur place » qui condamnait les provinces pauvres ;
             * à feedabilité égale on prend la manuf au gisement local le + riche (efficacité conservée). */
            if (re->raw_cap[in1] <= 0.f && !empire_has_raw(econ, a->cid, in1)) continue;
            if (re->raw_cap[in1] > pick_raw){ pick_raw=re->raw_cap[in1]; pick=(BuildingType)b; }
        }
        if (pick==BLD_TYPE_COUNT) continue;                           /* aucune manuf nourrissable posable ici */
        best_deficit=deficit; br=r; bb=pick;
    }
    if (br<0) return;                                                /* toutes les provinces sont (presque) pleines */
    float cost=tune_f("MANUF_BUILD_COST",50.f)*econ_world_ipm(econ);  /* T1 : la moins chère (le développement de base) */
    if (!credit_can_spend(econ, w, a->cid, cost)) return;            /* bloque seulement au-delà de la ligne de crédit */
    if (econ_build_manufacture(econ, br, bb)){
        credit_spend(econ, w, a->cid, cost); econ_flux_add(a->cid, FX_SOLDE, -cost);   /* débit AU SUCCÈS */
        a->stats.builds_other++;
    }
}

/* EXPLOITATION PAR LE FORECAST (décision ÉCONOMIQUE, pas un seuil arbitraire) — on améliore une
 * exploitation (+1 palier de boost, +RAW_BOOST_PER_TIER) SSI les DEUX conditions tiennent :
 *   (1) PÉNURIE APPROCHANTE — fc->runway[r] < AI_SAFETY_HORIZON. Le runway est désormais BOOST-AWARE
 *       (le forecast inclut les paliers déjà posés) → il REMONTE à mesure qu'on boost → la décision se
 *       RÉGULE (l'ancien gate shortfall>0.5 était quasi toujours vrai → 500 paliers/sim, prix cassés).
 *   (2) ROI — le +5% d'extraction ANNUELLE de la brute, valorisé au prix RÉGIONAL (qui MONTE avec la
 *       rareté → le boost devient rentable PILE quand c'est rare), rembourse le coût en PAYBACK ans.
 * On vise la brute au runway le plus COURT (la plus urgente). Une amélioration par tour. */
static bool ai_build_raw_boost(AiActor *a, const World *w, WorldEconomy *econ, const EconForecast *fc){
    int   max_tier = (int)tune_f("RAW_BOOST_MAX_TIER", 8.f);
    float safety   = tune_f("AI_SAFETY_HORIZON", 12.f);
    float payback  = tune_f("RAW_BOOST_PAYBACK", 8.f);
    float per_tier = tune_f("RAW_BOOST_PER_TIER", 0.05f);
    float cost     = tune_f("RAW_BOOST_COST", 40.f) * econ_world_ipm(econ);
    Resource pick=RES_NONE; float urgent=1e30f; int pick_region=-1;
    for (int r=1;r<RES_PROD_FIRST;r++){
        if (fc->runway[r] >= safety) continue;                /* (1) pas de pénurie APPROCHANTE (boost-aware) */
        int br=-1; float bcap=0.f;
        for (int rr=0;rr<econ->n_regions;rr++){
            RegionEconomy *re=&econ->region[rr];
            if (re->owner!=a->cid || !re->colonized) continue;
            if (re->raw_cap[r] <= 0.f || re->raw_boost[r] >= max_tier) continue;  /* brute ABSENTE ou au plafond */
            if (re->raw_cap[r] > bcap){ bcap=re->raw_cap[r]; br=rr; }              /* la province la + riche */
        }
        if (br<0) continue;
        /* (2) ROI : +5% de l'extraction ANNUELLE (supply×12) × prix RÉGIONAL (rareté incluse). */
        float O_year  = econ->region[br].supply[r]*12.f;
        float val_year= per_tier * O_year * econ->region[br].price[r];
        if (val_year*payback < cost) continue;                /* le palier ne se rembourse pas → on s'abstient */
        if (fc->runway[r] < urgent){ urgent=fc->runway[r]; pick=(Resource)r; pick_region=br; }
    }
    if (pick==RES_NONE || pick_region<0) return false;
    if (!credit_can_spend(econ, w, a->cid, cost)) return false;
    credit_spend(econ, w, a->cid, cost); econ_flux_add(a->cid, FX_SOLDE, -cost);
    econ->region[pick_region].raw_boost[pick]++;              /* +1 palier d'EXPLOITATION (boost d'extraction) */
    a->stats.builds_other++;
    return true;
}

/* Économie : commercer OU bâtir (le frein réoriente l'énergie vers le K). */
/* ── ALLOCATION DE MAIN-D'ŒUVRE & l'IA ──
 * MESURÉ : un override par poids (proportionnel) sous-performe le split AUTO glouton de l'IA
 * (66-69 % vs 75 % de satisfaction journalier), parce que l'auto FAIT COULER les bras vers les
 * seuls puits qui peuvent les employer (qui peut tourner les prend) et CONCENTRE sur la pénurie,
 * là où un poids fixe en gâche sur les bâtiments bornés en intrant. L'AUTO prix-driven EST donc
 * l'allocation « avisée » de l'IA — on ne l'override pas. L'override (alloc_on) reste l'outil du
 * JOUEUR (micro-gestion de SON empire), exposé par la façade scps_player_alloc_*. */
static void ai_econ_turn(AiActor *a, const World *w, WorldEconomy *econ, const AiView *v,
                         AgencyState *ag, RouteNetwork *rn, float brake, int day){
    /* Famine d'abord : un peuple affamé ne bâtit ni cours ni comptoir. STOCK-SAFE (étage 3b) :
     * on bâtit le GRENIER aussi quand la PRÉVISION voit le mur vivrier (food_alert = food_runway
     * < SAFETY) — proactif, AVANT que la marge ne s'effondre (le grenier relève le plafond de
     * stock + la capacité nourrie → coussin de réserve). */
    if ((v->food < AI_FOOD_FLOOR || v->food_alert) && a->home_region>=0){
        if (agency_build(ag, econ, w, a->home_region, EDI_GRENIER)) a->stats.builds_other++;
        return;
    }
    /* Le chantier (militaire · manufacture civile · bâtiment civil) est PONDÉRÉ PAR LE TEMPÉRAMENT
     * dans le seau « bâtir » plus bas — plus de fabrique INCONDITIONNELLE chaque tour. */
    a->credit_trade += a->w_trade * (1.f - 0.5f*brake);
    a->credit_build += a->w_build + 0.8f*brake;           /* le frein POUSSE à consolider */

    /* On décharge le seau le plus plein (≥ 1). */
    /* MERCANTILE — les ROUTES EN PRIO : si le seau commerce est prêt, le marchand OUVRE sa route
     * AVANT de bâtir (sa largeur, c'est le réseau). Les autres décident au seau le plus plein. */
    bool merc_prio = (a->w_trade>=a->w_build && a->w_trade>=a->w_expand) && a->credit_trade>=1.f;
    if (!merc_prio && a->credit_build>=1.f && a->credit_build>=a->credit_trade){
        a->credit_build -= 1.f;
        /* LE FORK (§ Soulèvements/Ordre de Fer) : sous une crise OUVERTE, un
         * tempérament COERCITIF (appétit de conquête haut) SERRE — il bâtit du H
         * (Garnison→Citadelle : tenir par la force, le chemin de l'Ordre de Fer)
         * au lieu de métaboliser. Les autres RÉFORMENT — ils bâtissent du K.
         * Aucun « si révolution alors » : c'est le même levier (bâtir), choisi
         * par la fiche ; le moteur d'ordre fait le verdict. */
        if (brake > AI_BRAKE_HARD && a->w_expand >= 0.60f){
            Edifice e = ai_next_h_edifice(econ, a->home_region);
            if (a->home_region>=0 && agency_build(ag, econ, w, a->home_region, e)){
                a->stats.builds_h++;
                faction_lever_apply(a->cid, ai_lever_for_edifice(e), AI_LEVER_BUILD);  /* §4 : bâtir = voter */
            }
        } else {
            /* DOCTRINE-PONDÉRÉ — le chantier (hors crise) suit le TEMPÉRAMENT : Bâtisseur (w_build)
             * ½ bâtiment civil · ¼ manuf mil · ¼ manuf civile ; Mercantile (w_trade) ½ manuf civile ·
             * ¼ civil · ¼ mil (ses ROUTES sont déjà priorisées par credit_trade, plus haut) ; Dominateur
             * (w_expand) ½ mil · ¼ manuf civile · ¼ civil. Seuils CUMULÉS [mil, +civmanuf] ; civil = reste. */
            float t_mil, t_civm;
            if (a->w_build >= a->w_trade && a->w_build >= a->w_expand){ t_mil=0.25f; t_civm=0.50f; }  /* Bâtisseur : ½ civil */
            else if (a->w_trade >= a->w_expand){                        t_mil=0.25f; t_civm=0.75f; }  /* Mercantile : ½ manuf civile */
            else {                                                      t_mil=0.50f; t_civm=0.75f; }  /* Dominateur : ½ militaire */
            /* tirage DÉTERMINISTE sur (jour × pays) — sans TOUCHER au rng perso (ne décale pas la
             * séquence dont dépendent les routes/départages) : déterministe, bien distribué par tour. */
            uint32_t hh=(uint32_t)day*2654435761u+(uint32_t)a->cid*40503u; hh^=hh>>13; hh*=0x5bd1e995u; hh^=hh>>15;
            float roll=(float)(hh & 0xFFFFFFu)/(float)0x1000000;
            if (roll < t_mil){
                ai_build_manufacture(a, w, econ);            /* l'arsenal militaire (tier + or, par doctrine) */
            } else if (roll < t_civm){
                /* EXPLOITATION (forecast) D'ABORD dans le créneau manufacture civile : une brute en
                 * déficit qu'on extrait → on AMÉLIORE son exploitation (+1 palier de boost d'extraction)
                 * avant toute autre fabrique. Pas de déficit boostable → manufacture civile normale.
                 * Le créneau FOI/SAVOIR/K (ci-dessous) reste INTACT → la foi peut toujours s'ériger. */
                if (!ai_build_raw_boost(a, w, econ, &v->fc))
                    ai_build_civmanuf(a, w, econ);            /* la manufacture civile (remplir les slots) */
            } else {
            /* le BÂTIMENT CIVIL (la voie existante) : on métabolise (K) ; un trône au consentement bas
             * se SACRALISE d'abord (la foi soutient L) ; institutions mûres → SAVOIR. Dominateur/Honneur
             * fourbissent L'ARSENAL (armes au marché → mil_power) ; le Mercantile bâtit le RÉSEAU
             * (marchés) ; le Bureaucrate ne quitte JAMAIS K (l'institution pure). */
            Ethos eth = ai_capital_ethos(w, econ, a->cid);
            int hr = a->home_region;
            const ProvBuild *bd = (hr>=0&&hr<econ->n_regions)?&econ->region[hr].build:NULL;
            bool faith_crisis = (bd && v->L < AI_FAITH_L && bd->faith < 5.0f);
            /* ZÈLE PROACTIF — un crédo prosélyte (w_faith haut) qui n'a PAS encore de foi bâtit son
             * PREMIER sanctuaire DE LUI-MÊME → la foi se FONDE (bloc RELIGION ci-dessous, sous le
             * plafond ⌈N/3⌉) au lieu de n'émerger qu'en crise de légitimité. On LIT w_faith (l'entrée
             * moteur dérivée du crédo), pas un bonus plat ; borné à UN chantier : athée + faith<1 +
             * aucun chantier de foi déjà en file (anti-spam — agency_build est asynchrone). */
            bool faith_pending = false;
            if (ag && hr>=0) for (int oi=0; oi<ag->n; oi++)
                if (ag->order[oi].active && ag->order[oi].kind==AGY_BUILD && ag->order[oi].region==hr){
                    Edifice pe=(Edifice)ag->order[oi].param;
                    if (pe==EDI_SANCTUAIRE||pe==EDI_TEMPLE||pe==EDI_CATHEDRALE||pe==EDI_MONASTERE){ faith_pending=true; break; }
                }
            bool faith_zeal = (bd && hr>=0 && !faith_pending && bd->faith < 1.0f
                               && religion_of_country(a->cid) < 0
                               && a->w_faith >= tune_f("AI_FAITH_ZEAL",AI_FAITH_ZEAL));
            if (!faith_crisis && !faith_zeal && (eth==ETHOS_DOMINATEUR || eth==ETHOS_HONNEUR) && hr>=0){
                RegionEconomy *cre=&econ->region[hr];
                float price=cre->price[RES_ARMS]; if (price<0.2f) price=0.2f;
                float cost =20.f*price*(cre->import_margin>0.f?cre->import_margin:1.f);
                if (credit_can_spend(econ, w, a->cid, cost)){
                    credit_spend(econ, w, a->cid, cost); cre->stock[RES_ARMS]+=20.f;
                    econ_flux_add(a->cid, FX_SOLDE, -cost);          /* I0 : la ligne militaire */
                    a->stats.builds_h++;                             /* l'arsenal = sa largeur martiale */
                    faction_lever_apply(a->cid, FAC_CONQUERANT, AI_LEVER_BUILD);
                }
            } else {
                Edifice e; int kind = 0;                   /* 0 = voie K (builds_k) · 2 = réseau */
                if (faith_crisis || faith_zeal){
                    e = ai_next_faith_edifice(econ, hr);   /* crise de consentement OU zèle prosélyte → foi */
                } else if (eth==ETHOS_MERCANTILE){
                    e = EDI_MARCHE; kind = 2;              /* la largeur du marchand : le RÉSEAU, toujours */
                } else if (bd && eth!=ETHOS_BUREAUCRATE
                           && bd->K_inst >= tune_f("AI_SAVOIR_K",AI_SAVOIR_K) && bd->savoir < 2.5f){
                    e = ai_next_savoir_edifice(econ, hr);  /* institutions mûres → savoir (pas le Bureaucrate : K pur) */
                    /* M5 — le savoir FORK à la BASE sur le pôle de la région (hystérésé) :
                     * martial → Bibliothèque militaire · fluide → Observatoire · l'Ordre
                     * garde Bibliothèque→Monastère. Branche close (frère bâti) → K. */
                    if (e==EDI_BIBLIOTHEQUE){
                        TechPole p = edifice_region_pole(w, econ, hr, day);
                        if      (p==POLE_MARTIAL) e=EDI_BIBLIO_MIL;
                        else if (p==POLE_FLUIDE)  e=EDI_OBSERVATOIRE;
                    }
                    if (hr>=0 && edifice_build_blocked(econ,hr,e)) e=ai_next_k_edifice(econ,hr);
                } else {
                    e = ai_next_k_edifice(econ, hr);       /* le métabolisme par défaut : K */
                }
                if (hr>=0 && agency_build(ag, econ, w, hr, e)){
                    /* développement institutionnel PROACTIF (la marque du Bâtisseur)
                     * vs DIGESTION imposée par le frein — on ne les confond pas. */
                    if      (kind==2)               a->stats.builds_other++;
                    else if (brake > AI_BRAKE_HARD) a->stats.builds_other++;
                    else                            a->stats.builds_k++;
                    faction_lever_apply(a->cid, ai_lever_for_edifice(e), AI_LEVER_BUILD);  /* §4 : la famille d'édifice vote */
                }
            }
            }   /* fin du tiers BÂTIMENT CIVIL (pondéré par doctrine) */
        }
    } else if (a->credit_trade>=1.f){
        a->credit_trade -= 1.f;
        /* E3 — les Halles acquises : l'Entrepôt du HUB passe AVANT la énième route
         * (une fois — le jeu de stock s'ouvre, puis le négoce reprend son cours). */
        int hub = intertrade_country_centre(econ, a->cid);
        if (hub<0 || hub>=econ->n_regions || econ->region[hub].owner!=a->cid) hub=a->home_region;
        if (a->has_halles && hub>=0 && hub<econ->n_regions
            && econ->region[hub].n_entrepot<1
            && econ->region[hub].treasury>400.f
            && agency_build(ag, econ, w, hub, EDI_ENTREPOT)){
            a->stats.builds_other++;
            faction_lever_apply(a->cid, FAC_MARCHAND, AI_LEVER_BUILD);
        } else {
            int p = ai_pick_trade_partner(econ, rn, a->home_region, a->cid);
            if (p>=0 && routes_order(rn, w, econ, a->home_region, p, false)){
                a->stats.routes++;
                faction_lever_apply(a->cid, FAC_MARCHAND, AI_LEVER_BUILD);   /* §4 : le négoce AVANCE les Marchands */
            } else if (a->home_region>=0 && agency_build(ag, econ, w, a->home_region, EDI_MARCHE)){
                a->stats.builds_other++;                   /* pas de partenaire : on bâtit le carrefour */
                faction_lever_apply(a->cid, FAC_MARCHAND, AI_LEVER_BUILD);
            }
            /* H3 — et la MER : si elle tient un port et qu'un port étranger est à portée
             * de courants, elle ouvre une route MARITIME (seuil ÷2 : la 1re liaison crée
             * le marché). C'est ce qui ranime le large entre continents séparés (G0.6). */
            { int mp=ai_owned_port(econ,a->cid);
              int sp=(mp>=0)?ai_pick_sea_partner(econ,rn,mp,a->cid):-1;
              if (sp>=0 && routes_order(rn,w,econ,mp,sp,true)){
                  a->stats.routes++;
                  faction_lever_apply(a->cid, FAC_MARCHAND, AI_LEVER_BUILD);
              } }
        }
    }
}

/* Le BIEN que l'IA veut arracher (pour le casus belli économique) : son trou le
 * plus aigu (stratégique → demande → chaîne). */
static Resource ai_war_want(const AiView *v){
    if (v->strat_gap !=RES_NONE) return v->strat_gap;
    if (v->demand_gap!=RES_NONE) return v->demand_gap;
    return v->chain_gap;
}

static int ai_owned_regions(const WorldEconomy *econ, int cid){
    int n=0; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid) n++; return n;
}
/* Cache module : TECH_ESCLAVAGE par pays (synchronisé en recherche, ligne ~1148) — pour
 * résoudre winner_enslaves d'un TIERS au règlement (p.ex. b vainqueur à la capitulation,
 * dont ai_strat_turn n'a pas l'acteur sous la main). Dérivé de la tech : non sauvegardé. */
static bool g_ai_enslave[SCPS_MAX_COUNTRY];
static bool ai_enslaves(int cid){ return (cid>=0&&cid<SCPS_MAX_COUNTRY) ? g_ai_enslave[cid] : false; }
/* §terrain : valeur cumulée des régions du `loser` que `winner` OCCUPE (ce que le
 * règlement pourra transférer) — l'arbitrage « budget couvert par les occupations ». */
static float ai_occupied_value(const DiploState *d, const WorldEconomy *econ, int winner, int loser){
    if (!d||!econ) return 0.f;
    float v=0.f;
    for (int r=0;r<econ->n_regions && r<SCPS_MAX_REG;r++)
        if (d->occupier[r]==winner && econ->region[r].owner==loser)
            v += diplo_province_price(econ, r);
    return v;
}
/* (Le « coup de grâce » par surcoût de capitale est désormais SUBSUMÉ par le §5 combat :
 * la capitale, cœur développé, coûte cher → seule une victoire décisive — budget de score
 * couvrant tout le territoire — l'arrache. Voir diplo_province_price / diplo_war_budget.) */

/* Stratégie : conquérir, déclarer la guerre, ou CONSOLIDER (le frein). */
static Heritage ai_capital_heritage(const World *w, const WorldEconomy *econ, int cid);   /* défini plus bas (recherche) */
static void ai_strat_turn(AiActor *a, World *w, WorldEconomy *econ, WorldProsperity *wp,
                          WorldLegitimacy *wl, DiploState *diplo, const Statecraft *sc,
                          const AiView *v, float brake, int day){
    if (ai_owned_regions(econ, a->cid)==0) return;      /* polité ABSORBÉE : inerte (plus de stratégie) */
    /* FREIN DUR : on a trop avalé / l'ordre craque → paix générale + verrou. */
    if (brake > AI_BRAKE_HARD){
        a->credit_consolidate += brake;
        if (a->credit_consolidate >= 1.f){
            a->credit_consolidate -= 1.f;
            for (int b=0; b<w->n_countries; b++)
                if (b!=a->cid && diplo_status(diplo, a->cid, b)==DIPLO_WAR)
                    diplo_settle(diplo, w, econ, wl, a->cid, b, a->can_enslave);  /* solde : garde l'occupé, relâche le reste */
            a->peace_lock_until = day + AI_PEACE_LOCK;       /* hystérésis : on digère */
            a->stats.consolidations++;
        }
        return;
    }

    /* RELIGION — le PLAFOND mondial ⌈N/3⌉ borne le TOTAL des religions (fondation ET schisme).
     * N = empires de GENÈSE (religion_empire_ref, stable) : « 6 empires ⇒ 2 religions » même quand
     * les sécessions multiplient les polities. */
    int n_emp = religion_empire_ref();

    /* FONDER au 1er édifice religieux (comme le joueur) : si le pays a bâti un sanctuaire/temple/…
     * et n'a pas de foi, il en FONDE une (aléatoire) tant que le TOTAL est sous le plafond ; au-delà,
     * il RALLIE une foi existante (les empires se PARTAGENT les religions). GATED : aucun édifice ⇒ no-op. */
    if (religion_of_country(a->cid) < 0){
        uint32_t emask=(1u<<EDI_SANCTUAIRE)|(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE)|(1u<<EDI_MONASTERE);
        int has_edi=0;
        for (int r=0;r<econ->n_regions && r<SCPS_MAX_REG;r++)
            if (econ->region[r].owner==a->cid && (econ->region[r].edi_built & emask)){ has_edi=1; break; }
        if (has_edi){
            int cp=w->country[a->cid].capital_prov, centre=0;
            if (cp>=0 && cp<w->n_provinces){ int sx=w->province[cp].seed_x, sy=w->province[cp].seed_y;
                if (sx>=0 && sy>=0) centre=sy*SCPS_W+sx; }
            uint32_t h=(uint32_t)(a->cid*0x9e3779b1u) ^ (uint32_t)((day+7)*2654435761u);
            int rid = religion_can_found(n_emp)
                      ? religion_found_random(a->cid, centre, h)
                      : religion_adopt_existing(a->cid, h);
            if (rid>=0) religion_inherit_regions(w, a->cid);
        }
    }

    /* RELIGION (P7) — SCHISME : si le pays ne contrôle plus le centre de sa foi (RUPTURE,
     * centre conquis), il ROMPT en une foi AUTONOME — repick ALÉATOIRE valide (2 slots),
     * crédo & teinte aléatoires, et l'adopte (son nouveau centre = sa capitale → l'éligibilité
     * se résout, pas de spam). GATED par le PLAFOND PAR RACINE : au plus RELIG_SCHISM_MAX schismes
     * par foi fondatrice (la foi en exil PERSISTE au-delà) — sans foi, éligibilité NONE ⇒ no-op. */
    if (religion_schism_eligible(w, a->cid) == RSE_RUPTURE
        && religion_can_schism(religion_of_country(a->cid))){
        int parent = religion_of_country(a->cid);
        if (parent >= 0){
            int cp = w->country[a->cid].capital_prov, centre = 0;
            if (cp>=0 && cp<w->n_provinces){ int sx=w->province[cp].seed_x, sy=w->province[cp].seed_y;
                if (sx>=0 && sy>=0) centre = sy*SCPS_W + sx; }
            uint32_t h = (uint32_t)(a->cid*2654435761u) ^ (uint32_t)((day+1)*40503u);
            for (int tries=0; tries<16; tries++){
                h ^= h>>13; h *= 0x5bd1e995u; h ^= h>>15;
                int sa = (int)(h%3u); h/=3u;
                int sb = (sa + 1 + (int)(h%2u)) % 3; h/=2u;             /* sb != sa */
                int pa = (int)(h%(uint32_t)RP_COUNT); h/=(uint32_t)RP_COUNT;
                h ^= h>>11; h *= 0x9e3779b1u;
                int pb = (int)(h%(uint32_t)RP_COUNT); h/=(uint32_t)RP_COUNT;
                int nc = (int)(h%(uint32_t)CREDO_COUNT);
                int trad[3]; for(int i=0;i<3;i++) trad[i]=g_religions[parent].traditions[i];
                trad[sa]=pa; trad[sb]=pb;
                if (!religion_picks_valid(trad[0],trad[1],trad[2])) continue;
                int child = religion_schism(parent, sa,pa, sb,pb, nc, centre, a->cid, 1, h);
                if (child >= 0){
                    religion_set_country(a->cid, child);     /* adopte la foi autonome */
                    religion_inherit_regions(w, a->cid);     /* ses régions suivent */
                }
                break;
            }
        }
    }
    /* RELIGION (P7) — EMPLOI du LETTRÉ : une région de foi MINORITAIRE gronde et aucun
     * lettré n'est déployé → en recruter un dessus (rôle dérivé du crédo). GATED. */
    if (religion_of_country(a->cid) >= 0 && !religion_scholar_active(a->cid)){
        int mine = religion_of_country(a->cid), target = -1;
        for (int r=0;r<econ->n_regions && r<SCPS_MAX_REG;r++){
            if (econ->region[r].owner!=a->cid) continue;
            int rr=religion_of_region(r);
            if (rr>=0 && rr!=mine){ target=r; break; }
        }
        if (target>=0) religion_scholar_recruit(a->cid, target);
    }

    /* REDDITION (§3) : si l'on est DÉFENSEUR dans une guerre nettement PERDUE (le
     * bras-de-fer penche fort vers l'attaquant) et militairement sans espoir → on
     * CAPITULE plutôt que de se faire anéantir. (L'IA lit le score + l'armée + le frein.) */
    for (int b=0; b<w->n_countries; b++){
        if (b==a->cid || diplo_status(diplo,a->cid,b)!=DIPLO_WAR) continue;
        if (diplo_war_goal(diplo,b,a->cid)==CB_NONE) continue;          /* b est l'attaquant */
        float their_score = diplo_war_score(diplo, b, a->cid);
        if (their_score >= AI_SURRENDER && v->armee < AI_ARMY_MARGIN*diplo_mil_power(w,econ,b)){
            /* §5 : si la victoire de b est DÉCISIVE (son budget de score couvre une bonne
             * part de notre territoire), nul tribut ne l'arrête — il veut TOUT, et son
             * budget enfle d'occupation à mesure qu'il prend → annexion. Sinon on capitule. */
            if (diplo_war_budget(diplo,w,econ,b,a->cid) >= AI_ANNEX_FRAC*diplo_country_value(econ,a->cid)) continue;
            diplo_reparations(diplo, w, econ, a->cid, b);               /* le vaincu indemnise le vainqueur */
            diplo_settle(diplo, w, econ, wl, b, a->cid, ai_enslaves(b)); /* capitulation : b ANNEXE ce qu'il occupe (peut nous TUER) */
        }
    }

    if (day < a->peace_lock_until) return;                  /* on tient la paix (digestion) */

    /* Appétit agressif, gaté par le frein (mou) et nourri par la foi — puis LISSÉ : un SOCLE (les
     * mondes consolidés voient AUSSI la guerre, pas l'atonie) ÷ la SATURATION mondiale (les mondes
     * fendus ne SPIRALENT plus à 25 : plus il y a de guerres en cours, moins on en ajoute). */
    { float aggr = ai_aggression(a, v) + tune_f("AI_WAR_BASELINE", 0.05f);
      aggr /= (1.f + tune_f("AI_WAR_SATURATION", 0.20f) * (float)ai_world_war_pairs(w, diplo));
      /* PRÉVISION DIPLO — FREIN À LA MENACE ENTRANTE : si mon pire voisin hostile m'écrase
       * (war_risk au-delà du seuil), je FREINE ma PROPRE offensive — anticiper la coalition qui se
       * forme plutôt que d'ouvrir un second front. Deadband (AI_THREAT_GATE) : sous le seuil, AUCUN
       * effet → la fenêtre golden (voisins peu menaçants en 12 ans) reste byte-identique. */
      DiploForecast dfc = ai_diplo_forecast(w, econ, wp, diplo, a->cid);
      if (dfc.war_risk > tune_f("AI_THREAT_GATE", 0.55f))
          aggr *= (1.f - tune_f("AI_THREAT_BRAKE", 0.5f) * dfc.war_risk);
      a->credit_war += aggr; }
    if (a->credit_war < 1.f) return;
    /* §war-smoothing — CAP mondial : au-delà de N paires en guerre, on N'EN OUVRE PLUS (le monde
     * fendu ne SPIRALE plus à 25 ; la saturation seule ne faisait que RETARDER). Les mondes
     * consolidés (peu de guerres) ne l'atteignent jamais → eux gardent leur appétit (socle). */
    if (ai_world_war_pairs(w, diplo) >= (int)tune_f("AI_WAR_CAP", 3.f)) return;

    /* Déjà en guerre ? Priorité : ENCAISSER (conquérir une région ennemie). On
     * ne multiplie pas les fronts — un seul à la fois. */
    int at_war = 0;
    for (int b=0; b<w->n_countries; b++)
        if (b!=a->cid && diplo_status(diplo, a->cid, b)==DIPLO_WAR) at_war++;
    if (at_war>0){
        /* §terrain — LA GUERRE SE GAGNE SUR LE TERRAIN : l'IA ne CONQUIERT plus en
         * guerre. Ses ARMÉES occupent (couche sim) ; ici elle ARBITRE quand RÉGLER.
         * La propriété ne change qu'à la PAIX (diplo_settle), bornée par le budget §5 :
         *   - but NON-territorial : UNE région tenue = point fait → règle ;
         *   - territorial : on presse tant que la valeur occupée < budget (on peut
         *     s'offrir plus) ; quand elle le COUVRE → on encaisse (settle) ;
         *   - ÉPUISEMENT : une guerre qui traîne se solde (gains partiels encaissés,
         *     ou paix blanche si rien d'occupé). loot/réparations/contrats AU RÈGLEMENT. */
        for (int b=0; b<w->n_countries; b++){
            if (b==a->cid || diplo_status(diplo, a->cid, b)!=DIPLO_WAR) continue;
            CasusBelli goal = diplo_war_goal(diplo, a->cid, b);
            int   occ       = (b<SCPS_MAX_COUNTRY)? diplo->conquered[a->cid][b] : 0;
            float budget    = diplo_war_budget(diplo, w, econ, a->cid, b);
            float occ_value = ai_occupied_value(diplo, econ, a->cid, b);
            float wy        = (b<SCPS_MAX_COUNTRY)? diplo->war_years[a->cid][b] : 0.f;

            /* PAIX DÉCLARÉE (règle nette, lisible à l'UI) : score ≥ 50 ET au moins une
             * région TENUE → victoire DÉCISIVE, on encaisse l'occupé (diplo_settle, borné
             * par le budget §5) ; OU 5 ANS écoulés → la guerre s'éteint en PAIX BLANCHE
             * (rien d'occupé → ~0 transfert). Le « ET occupé » laisse les SIÈGES aboutir :
             * sans lui, le score-batailles touche 50 avant qu'une place tombe → 0 prise. */
            float wscore   = (b<SCPS_MAX_COUNTRY)? diplo_war_score(diplo, a->cid, b) : 0.f;
            bool  decisive = (wscore >= tune_f("AI_WAR_DECISIVE",AI_WAR_DECISIVE) && occ >= 1);
            bool  exhausted= (wy     >= tune_f("AI_WAR_EXHAUST",AI_WAR_EXHAUST));
            if (!decisive && !exhausted) continue;           /* sinon : on laisse les armées travailler */

            float leftover = budget - occ_value; if (leftover<0.f) leftover=0.f;
            diplo_loot(w, econ, a->cid, b, leftover);        /* le budget non pris en TERRE vide les coffres */
            diplo_reparations(diplo, w, econ, a->cid, b);
            if (goal==CB_SUBJUGATION && diplo_suzerain(diplo,b)<0 && occ>=1)
                diplo_set_vassal(diplo, a->cid, b, CONTRAT_SERVAGE);  /* la vassalité EST le but */
            else
                ai_impose_contract(a, w, econ, diplo, b);    /* §leviers : imposer plutôt qu'annexer */
            int got = diplo_settle(diplo, w, econ, wl, a->cid, b, a->can_enslave);  /* la propriété change ICI */
            a->credit_war -= 1.f; a->stats.conquests += got;
            if (got>0) faction_lever_apply(a->cid, FAC_CONQUERANT, AI_LEVER_WAR);    /* §4 : prendre AVANCE les Conquérants */
        }
        return;
    }

    /* En paix : ÉQUILIBRE avant prédation (rétroaction négative, jamais d'interdit). */

    int heg = diplo_perceived_hegemon(w, econ, wp, diplo, a->cid);

    /* §D1 — L'ALLIANCE A UNE SORTIE : chaque tick, on rompt celles dont la raison
     * s'efface. La raison, c'est la MENACE COMMUNE (shared_rel), PAS le score global —
     * lequel reste gonflé par le complément + la parenté, si bien qu'une alliance
     * d'AFFINITÉ ne lâchait jamais et figeait tout. (a) DÉSUÉTUDE : la menace commune
     * relative tombe sous ALLY_THREAT_KEEP → le lien lâche, complémentaires ou non.
     * (b) TRAHISON : l'allié a snowballé au point d'être MON hégémon perçu (son threat,
     * amplifié par le momentum, domine) → il EST la menace, on le lâche. */
    for (int b=0;b<w->n_countries;b++){
        if (b==a->cid || diplo_status(diplo,a->cid,b)!=DIPLO_ALLIED) continue;
        Relation rel = diplo_relation(w,econ,wp,diplo,a->cid,b);
        bool menace_evanouie = (rel.shared_rel < ALLY_THREAT_KEEP);   /* la menace commune a fondu */
        if (menace_evanouie || b==heg)
            diplo_make_peace(diplo, a->cid, b);          /* ALLIED→NEUTRAL : le slot se libère → réalignement */
    }

    /* (1) COALITION — se liguer contre l'HÉGÉMON perçu (anti-runaway, émergent des
     * menaces sommées). On se joint si l'hégémon GUERROIE déjà (pile-on) et que
     * notre camp (soi + alliés) pèse assez. Le frein gouverne : un fragile n'ose pas. */
    if (heg>=0 && heg!=a->cid && diplo_status(diplo,a->cid,heg)==DIPLO_NEUTRAL
        && diplo_can_declare(diplo,a->cid,heg) && country_at_war(w,diplo,heg)){
        float my_side = v->armee + allied_power(w,econ,diplo,a->cid);
        CasusBelli cb = diplo_casus_belli(w,econ,wp,diplo,a->cid,heg, ai_war_want(v));
        if (cb!=CB_NONE && my_side >= AI_ARMY_MARGIN*diplo_mil_power(w,econ,heg)){
            diplo_declare_war_cb(diplo, a->cid, heg, cb);   /* la ligue a une raison (souvent territoriale) */
            a->credit_war -= 1.f; a->stats.wars++;
            return;
        }
    }

    /* (1-bis) PREMIÈRE FRAPPE DU CONQUÉRANT — un tempérament très agressif (w_expand ≥
     * AI_CONQUEROR_W) face à une PROIE adjacente nettement plus faible la SAISIT plutôt
     * que de se lier : sinon, en monde calme (menace ambiante basse §D2), l'alliance
     * facile étoufferait toute conquête (le conquérant s'allie même à sa proie). Les
     * tempéraments modérés, eux, préfèrent encore le pacte (étape 2). Le frein veille en
     * amont (credit_war < 1 → on n'arrive jamais ici quand on est surétendu). */
    if (a->w_expand >= AI_CONQUEROR_W){
        Resource pwant = ai_war_want(v);
        int prey = ai_pick_rival(a, w, econ, wp, diplo, v->armee, pwant);   /* déjà : adjacent, + faible, AVEC CB */
        if (prey>=0){
            CasusBelli cb = diplo_casus_belli(w,econ,wp,diplo,a->cid,prey,pwant);
            diplo_declare_war_cb(diplo, a->cid, prey, cb);
            a->credit_war -= 1.f; a->stats.wars++;
            return;
        }
    }

    /* (2) ALLIANCE EN ACTE — se lier à l'allié naturel le plus fort (friction préventive :
     * la menace partagée et le complément écrasent la prédation mutuelle). MAIS au plus
     * AI_ALLY_SLOTS pactes : l'alliance est une ressource RARE qu'on arbitre, pas un seuil
     * que tout le monde franchit. Slots pleins → on n'élargit pas ; on n'ÉVINCE le plus
     * faible que si le nouveau le dépasse NETTEMENT (marge) — sinon on garde ce qu'on a. */
    int ally = ai_pick_ally(a, w, econ, wp, diplo, sc);
    if (ally>=0){
        if (diplo_ally_count(diplo, a->cid) < DIPLO_ALLY_SLOTS){
            diplo_form_alliance(diplo, a->cid, ally);
            a->credit_war = fmaxf(0.f, a->credit_war - 0.5f);    /* l'énergie passe à se lier */
            return;
        }
        float wv; int worst = weakest_ally(a, w, econ, wp, diplo, &wv);
        Relation arel = diplo_relation(w,econ,wp,diplo,a->cid,ally);
        if (worst>=0 && arel.alliance > wv + AI_ALLY_SWAP_MARGIN){
            diplo_make_peace(diplo, a->cid, worst);          /* libère le slot le plus faible */
            diplo_form_alliance(diplo, a->cid, ally);        /* … pour un allié nettement meilleur */
            a->credit_war = fmaxf(0.f, a->credit_war - 0.5f);
            return;
        }
    }

    /* (2c) BRASSAGE — LE PACTE MIGRATOIRE avec un ALLIÉ d'un AUTRE héritage : la
     * population échangée diffuse son savoir (métabolisation). Un cran de confiance
     * au-dessus du commercial ⇒ réservé aux ALLIÉS ; l'autre héritage = l'intérêt à
     * digérer (rien à apprendre d'un clone). Une proposition/tour, consentement
     * bilatéral. Ne PASSE PAS avant l'an-12 (les alliances ne se forment pas si tôt →
     * golden intact ; le brassage vit au-delà). */
    { Heritage myh = ai_capital_heritage(w, econ, a->cid);
      for (int b=0; b<w->n_countries && b<SCPS_MAX_COUNTRY; b++){
          if (b==a->cid || diplo_status(diplo,a->cid,b)!=DIPLO_ALLIED) continue;
          if (diplo_migration_pact(diplo,a->cid,b)) continue;              /* déjà lié */
          if (ai_capital_heritage(w,econ,b)==myh) continue;               /* même héritage → rien à digérer */
          if (ai_consider_offer(w,econ,wp,diplo,sc, a->cid, b, OFFER_MIGRATION)){
              diplo_set_migration_pact(diplo, a->cid, b, true);
              break;
          }
      }
    }

    /* (2b) LA COLERE DU GEANT (coques 5) — un commanditaire de pirates nous
     * saigne depuis trop longtemps : le CB anti-piraterie PRIME sur la prédation
     * (la guerre a un but : faire DESARMER la course). */
    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++){
        if (b==a->cid || diplo_status(diplo,a->cid,b)!=DIPLO_NEUTRAL) continue;
        if (diplo_pirate_rancor(diplo,a->cid,b)<1.5f) continue;        /* en-deca, nul CB possible */
        if (diplo_casus_belli(w,econ,wp,diplo,a->cid,b,RES_NONE)!=CB_ANTIPIRATERIE) continue;
        if (!diplo_can_declare(diplo,a->cid,b)) continue;
        diplo_declare_war_cb(diplo, a->cid, b, CB_ANTIPIRATERIE);
        a->credit_war -= 1.f; a->stats.wars++;
        return;
    }

    /* (3) PRÉDATION — la meilleure cible (lue) : hors trêve, hors allié, AVEC un
     * casus belli qui colle au but, friction d'élargissement comprise. */
    Resource want = ai_war_want(v);
    int rival = ai_pick_rival(a, w, econ, wp, diplo, v->armee, want);
    if (rival>=0){
        CasusBelli cb = diplo_casus_belli(w,econ,wp,diplo,a->cid,rival, want);
        diplo_declare_war_cb(diplo, a->cid, rival, cb);   /* la guerre a une RAISON (gate la paix) */
        a->credit_war -= 1.f; a->stats.wars++;
    }
}

/* ===================================================================== */
/* RECHERCHE — l'arbre de tech vivant (buts + penchant de heritage + frein)     */
/* ===================================================================== */
static Heritage ai_capital_heritage(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0||cid>=w->n_countries) return HERITAGE_ADAPTATIF;
    int cp=w->country[cid].capital_prov;
    if (cp<0||cp>=w->n_provinces) return HERITAGE_ADAPTATIF;
    int cr=w->province[cp].region;
    if (cr<0||cr>=econ->n_regions) return HERITAGE_ADAPTATIF;
    return econ->region[cr].culture.heritage;
}
static Ethos ai_capital_ethos(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0||cid>=w->n_countries) return ETHOS_ORDRE;
    int cp=w->country[cid].capital_prov; if (cp<0||cp>=w->n_provinces) return ETHOS_ORDRE;
    int cr=w->province[cp].region;       if (cr<0||cr>=econ->n_regions) return ETHOS_ORDRE;
    return econ->region[cr].culture.ethos;
}
/* §éthos — BIAIS DE COÛT (briefs Savoir/Forge/Société §3 : « biais, jamais mur »).
 * L'éthos rend une FONCTION plus/moins chère à pousser et pèse sur l'attrait du faustien ;
 * multiplicateur BORNÉ → il ne ferme JAMAIS une branche (un Dominateur atteint l'Université,
 * juste plus cher ; la pointe arcane reste accessible à un Pacifiste, au prix fort). */
static float ai_tech_cost_mult(Ethos e, const TechNode *n){
    float m=1.0f;
    switch (n->func){
        case FN_PRODUCTION:                                    /* savoir/commerce/industrie au service de l'État */
            if (e==ETHOS_BUREAUCRATE || e==ETHOS_MERCANTILE) m*=0.80f;
            else if (e==ETHOS_PACIFISTE) m*=0.88f;
            break;
        case FN_ARMEE:                                         /* la levée / l'arme */
            if (e==ETHOS_DOMINATEUR || e==ETHOS_HONNEUR) m*=0.80f;
            else if (e==ETHOS_PACIFISTE) m*=1.30f;
            else if (e==ETHOS_MERCANTILE) m*=1.10f;
            break;
        case FN_RENFORCEMENT:                                  /* tenir : institutions, intégration, foi */
            if (e==ETHOS_BUREAUCRATE) m*=0.80f;                /* tient la diversité, assimile à bas coût */
            else if (e==ETHOS_ORDRE)  m*=0.85f;
            else if (e==ETHOS_HONNEUR) m*=1.20f;               /* mauvais intégrateur */
            break;
        default: break;
    }
    if (n->faustian){                                          /* l'interdit : fui ou embrassé selon l'éthos */
        if      (e==ETHOS_PACIFISTE)  m*=1.40f;
        else if (e==ETHOS_ORDRE)      m*=1.15f;
        else if (e==ETHOS_DOMINATEUR) m*=0.90f;
    }
    return clampf(m, 0.6f, 1.6f);                              /* biais borné : jamais un mur */
}
/* §4 RELIGION — la posture de la foi régnante sur l'interdit [0..1] (orthodoxe bas
 * ↔ culte haut), lue de l'éthos de la culture-capitale (même barème que scps_faith).
 * L'orthodoxe INTERDIT le faustien (sacrilège) ; le culte le SACRALISE. */
static float ai_faith_stance(const World *w, const WorldEconomy *econ, int cid){
    if (cid<0||cid>=w->n_countries) return 0.25f;
    int cp=w->country[cid].capital_prov; if (cp<0||cp>=w->n_provinces) return 0.25f;
    int cr=w->province[cp].region;       if (cr<0||cr>=econ->n_regions) return 0.25f;
    switch (econ->region[cr].culture.ethos){
        case ETHOS_DOMINATEUR: return 0.36f; case ETHOS_HONNEUR:  return 0.30f;
        case ETHOS_MERCANTILE: return 0.26f; case ETHOS_PACIFISTE:return 0.20f;
        case ETHOS_BUREAUCRATE:return 0.14f; case ETHOS_ORDRE:    return 0.10f;
        default:               return 0.20f;
    }
}
float ai_country_population(const World *w, const WorldEconomy *econ, int cid){
    float pop=0.f; (void)w;
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid){
        const RegionEconomy *re=&econ->region[r];
        for (int k=0;k<CLASS_COUNT;k++) pop += re->strata[k].pop;
    }
    return pop;
}
/* §SYNCRÉTIQUE — la porte de tech n'est plus la RACE mais le PROFIL CULTUREL.
 * Chaque heritage-signature des NODES[] définit un ARCHÉTYPE = le CENTROÏDE culturel de
 * ses porteurs au monde (axes de contenu, pondéré pop). Un empire ATTEINT l'archétype
 * — donc peut chercher ses techs-signatures — si une culture qu'il GOUVERNE (la sienne
 * comprise) est à portée du centroïde (D∞ ≤ PORTEE). C'est l'accès par SOI OU par
 * CONTACT de gouvernance (le seul canal qui atteint le secret). La heritage seule n'ouvre
 * plus RIEN : un ésotérique assimilé au marchand perd l'accès arcane ; un non-ésotérique au profil
 * arcane l'obtient. Le déverrouillage reste LOQUETÉ (tech unlocked[] permanent) → la
 * tech survit à l'assimilation/disparition de la source. Topologie de l'arbre intacte. */
#define ARCH_PORTEE_PROFIL 2.5f   /* D∞ max (axes [0..10]) pour « porter » l'archétype — surface d'équilibrage */

/* D∞ sur les axes de CONTENU (valeurs/subsistance/parenté/religion), comme
 * culture_content_distance — mais directement sur PopCulture (struct région). */
static float pc_content_dist(const PopCulture *a, const PopCulture *b){
    float dv=fabsf(a->valeurs-b->valeurs),   ds=fabsf(a->subsistance-b->subsistance),
          dp=fabsf(a->parente-b->parente),   dr=fabsf(a->religion-b->religion);
    float m=dv; if(ds>m)m=ds; if(dp>m)m=dp; if(dr>m)m=dr; return m;
}
/* Centroïde culturel (contenu) de chaque heritage-archétype au monde, pondéré population.
 * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir de LA
 * SEULE province représentative (capitale, sinon la plus peuplée), pas un agrégat de toute
 * la région. Un centroïde MONDIAL doit voir TOUTE la diversité de population (chaque province,
 * chaque groupe) — on scanne donc econ->prov[] directement (pattern a, comme
 * econ_country_metabolized), plus fidèle qu'une région-par-région sur son seul représentant. */
static void world_archetype_centroids(const WorldEconomy *econ, PopCulture cen[HERITAGE_COUNT], bool present[HERITAGE_COUNT]){
    double sv[HERITAGE_COUNT]={0}, ss[HERITAGE_COUNT]={0}, sp[HERITAGE_COUNT]={0}, sr[HERITAGE_COUNT]={0}, wsum[HERITAGE_COUNT]={0};
    int nprov=econ->n_prov; if (nprov>SCPS_MAX_PROV) nprov=SCPS_MAX_PROV;
    for (int p=0;p<nprov;p++){
        const ProvinceEconomy *pe=&econ->prov[p];
        if (!pe->active || !pe->colonized) continue;
        if (pe->pop.n_groups>0){
            for (int g=0;g<pe->pop.n_groups;g++){
                const PopGroup *pg=&pe->pop.groups[g]; int rr=pg->heritage;
                if (rr<0||rr>=HERITAGE_COUNT || pg->count<=0) continue;
                double wq=(double)pg->count; const PopCulture *c=&pg->culture;
                sv[rr]+=wq*c->valeurs; ss[rr]+=wq*c->subsistance; sp[rr]+=wq*c->parente; sr[rr]+=wq*c->religion; wsum[rr]+=wq;
            }
        } else {
            int rr=pe->culture.heritage; if (rr<0||rr>=HERITAGE_COUNT) continue;
            const PopCulture *c=&pe->culture;
            sv[rr]+=c->valeurs; ss[rr]+=c->subsistance; sp[rr]+=c->parente; sr[rr]+=c->religion; wsum[rr]+=1.0;
        }
    }
    for (int r=0;r<HERITAGE_COUNT;r++){
        present[r]=(wsum[r]>0.0);
        if (present[r]){ cen[r].valeurs=(float)(sv[r]/wsum[r]); cen[r].subsistance=(float)(ss[r]/wsum[r]);
                         cen[r].parente=(float)(sp[r]/wsum[r]); cen[r].religion=(float)(sr[r]/wsum[r]); }
        else { cen[r].valeurs=cen[r].subsistance=cen[r].parente=cen[r].religion=0.f; }
    }
}
/* PROFONDEUR de contact par archétype (§4-6) : le canal le PLUS PROFOND par lequel une
 * culture portant l'archétype atteint l'empire — GOUVERNANCE (secret) > FRONTIÈRE/FOI
 * (métier) > COMMERCE/diffusion lointaine (surface). C'est ce qui décide jusqu'où une
 * tradition diffuse : le comptoir passe la surface, seule la gouvernance atteint le secret.
 * depth[] indexé par heritage-signature (archétype ↔ heritage, 1:1). */
/* La culture porte-t-elle l'archétype ar ? — distance au centroïde (signatures de heritage,
 * 0..HERITAGE_COUNT-1) ou correspondance d'ÉTHOS (profils d'éthos au-delà : bureaucrate, marchand). */
static bool culture_bears_arch(const PopCulture *c, int ar, const PopCulture cen[HERITAGE_COUNT], const bool present[HERITAGE_COUNT]){
    if (ar>=0 && ar<HERITAGE_COUNT) return present[ar] && pc_content_dist(c,&cen[ar])<=ARCH_PORTEE_PROFIL;
    if (ar==ARCH_BUREAUCRATIQUE) return c->ethos==ETHOS_BUREAUCRATE;
    if (ar==ARCH_MERCANTILE)     return c->ethos==ETHOS_MERCANTILE;
    return false;
}
/* COHÉSION d'une région gouvernée [0..1] — l'assentiment du sol : intégration moyenne
 * (pondérée pop) des groupes. Une province homogène (cœur) est pleinement digérée (1.0) ;
 * une conquête fraîche, mal intégrée, est basse → elle ne transmet son art qu'à mi-voix.
 * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir de LA
 * SEULE province représentative. La cohésion d'une région-GOUVERNANCE (le sceau qui ouvre le
 * secret d'archétype) doit refléter TOUTES ses provinces membres — une capitale bien digérée
 * ne doit pas maquiller une marche périphérique mal intégrée (ou l'inverse). Scan complet via
 * le champ miroir prov[].region (comme econ_aggregate_regions), pas juste le représentant. */
static float region_cohesion(const WorldEconomy *econ, int r){
    double si=0.0, sw=0.0;
    int nprov=econ->n_prov; if (nprov>SCPS_MAX_PROV) nprov=SCPS_MAX_PROV;
    for (int p=0;p<nprov;p++){
        const ProvinceEconomy *pe=&econ->prov[p];
        if (pe->region!=r) continue;
        for (int g=0;g<pe->pop.n_groups;g++){ double wq=(double)pe->pop.groups[g].count; if(wq<=0)continue;
            si+=wq*pe->pop.groups[g].integration; sw+=wq; }
    }
    return (sw>0.0)?(float)(si/sw):1.0f;
}
/* Une région (ses groupes de population) PORTE-t-elle l'archétype ar ? Le culture_bears_arch
 * sur la culture-AGRÉGÉE (re->culture, un agrégat légitime — capitale/dominante, PAS .pop) est
 * tenté d'abord (bon marché) ; sinon on cherche une MINORITÉ porteuse.
 * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir de LA
 * SEULE province représentative. Une minorité qui porte l'archétype peut vivre dans N'IMPORTE
 * QUELLE province de la région (pas forcément la capitale/rep) — on scanne donc TOUTES les
 * provinces membres (champ miroir prov[].region), pas seulement le représentant. */
static bool region_bears_arch(const WorldEconomy *econ, int r, int ar,
                              const PopCulture cen[HERITAGE_COUNT], const bool present[HERITAGE_COUNT]){
    if (r<0 || r>=econ->n_regions) return false;
    if (culture_bears_arch(&econ->region[r].culture, ar, cen, present)) return true;
    int nprov=econ->n_prov; if (nprov>SCPS_MAX_PROV) nprov=SCPS_MAX_PROV;
    for (int p=0;p<nprov;p++){
        const ProvinceEconomy *pe=&econ->prov[p];
        if (pe->region!=r) continue;
        for (int g=0;g<pe->pop.n_groups;g++)
            if (culture_bears_arch(&pe->pop.groups[g].culture, ar, cen, present)) return true;
    }
    return false;
}
static void ai_archetype_depth(const World *w, const WorldEconomy *econ, const RouteNetwork *rn, int cid, unsigned char depth[ARCH_COUNT]){
    PopCulture cen[HERITAGE_COUNT]; bool present[HERITAGE_COUNT];
    world_archetype_centroids(econ, cen, present);
    for (int r=0;r<ARCH_COUNT;r++) depth[r]=PROF_NONE;
    /* credo dominant (capitale) → canal RELIGION (la foi partagée ouvre le métier). */
    Credo mycredo=(Credo)0; bool has_credo=false;
    { int cp=(cid>=0&&cid<w->n_countries)?w->country[cid].capital_prov:-1;
      int cr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
      if (cr>=0&&cr<econ->n_regions){ mycredo=econ->region[cr].culture.credo; has_credo=true; } }
    /* régions-FRONTIÈRE : possédées par un AUTRE, mais adjacentes à une région à moi. */
    static bool border[SCPS_MAX_REG];
    for (int r=0;r<econ->n_regions && r<SCPS_MAX_REG;r++){
        border[r]=false;
        const RegionEconomy *re=&econ->region[r];
        if (re->owner==cid || !re->colonized) continue;
        for (int k=0;k<econ->n_regions;k++) if (econ->adj[r][k] && econ->region[k].owner==cid){ border[r]=true; break; }
    }
    for (int r=0;r<econ->n_regions;r++){
        const RegionEconomy *re=&econ->region[r];
        if (!re->active || !re->colonized) continue;
        Profondeur ch;
        if (re->owner==cid){
            /* §6 — la GOUVERNANCE atteint le secret, mais GRADÉE par la COHÉSION : un sol
             * mal digéré (conquête fraîche, faible intégration) ne transmet son art qu'à
             * mi-voix (métier), pas jamais — il faut LÉGITIMER pour ouvrir le profond/secret.
             * Course avec l'assimilation : digérer avant que la source ne se fonde (§2). */
            float coh=region_cohesion(econ, r);
            ch = (coh>=0.66f)?PROF_SECRET : (coh>=0.33f)?PROF_PROFOND : PROF_METIER;
        }
        else if (has_credo && re->culture.credo==mycredo)            ch=PROF_METIER;    /* co-religion */
        else if (r<SCPS_MAX_REG && border[r])                        ch=PROF_METIER;    /* frontière */
        else                                                         ch=PROF_SURFACE;   /* commerce / diffusion */
        for (int ar=0; ar<ARCH_COUNT; ar++){
            if (depth[ar]>=(unsigned char)ch) continue;
            if (region_bears_arch(econ, r, ar, cen, present)) depth[ar]=(unsigned char)ch;
        }
    }
    /* S1 — LE CHEMIN COMMERCIAL vers l'archétype (Venise ← Grèce, sans conquérir). Un
     * contact COMMERCIAL soutenu — une route OUVERTE (≥120 j de mer rien que pour s'établir)
     * où JE suis partie et dont l'AUTRE bout, à un AUTRE pays, PORTE l'archétype — creuse la
     * profondeur. La MER pèse FORT (SEA_W>LAND_W : Venise, la Hanse, le Gujarat) ; le VOLUME
     * module ; et l'on SOMME sur les ENTITÉS distinctes (le meilleur lien par partenaire) →
     * plus de partenaires porteurs = plus profond. Franchir PROFOND ouvre la porte d'archétype
     * (l'accès recherche) SANS gouverner — c'est la différence avec la conquête. */
    if (rn){
        static float gbest[ARCH_COUNT][SCPS_MAX_COUNTRY];
        for (int a=0;a<ARCH_COUNT;a++) for (int o=0;o<SCPS_MAX_COUNTRY;o++) gbest[a][o]=0.f;
        float sea_w=tune_f("SYNC_TRADE_SEA_W",2.0f), land_w=tune_f("SYNC_TRADE_LAND_W",1.0f);
        float yref=tune_f("SYNC_TRADE_YIELDREF",5.0f); if(yref<1e-3f)yref=1e-3f;
        for (int i=0;i<rn->n;i++){
            const TradeRoute *t=&rn->route[i];
            if (!t->open) continue;                              /* une relation TENUE, pas d'un tour */
            if (t->ra<0||t->rb<0||t->ra>=econ->n_regions||t->rb>=econ->n_regions) continue;
            int oa=econ->region[t->ra].owner, ob=econ->region[t->rb].owner, far;
            if      (oa==cid && ob!=cid && ob>=0 && ob<SCPS_MAX_COUNTRY) far=t->rb;
            else if (ob==cid && oa!=cid && oa>=0 && oa<SCPS_MAX_COUNTRY) far=t->ra;
            else continue;                                       /* JE dois être partie, l'autre un AUTRE pays */
            int po=econ->region[far].owner; if (po<0||po>=SCPS_MAX_COUNTRY) continue;
            const RegionEconomy *fr=&econ->region[far];
            if (!fr->active || !fr->colonized) continue;
            float vol=t->yield/yref; if(vol>1.f)vol=1.f; if(vol<0.f)vol=0.f;     /* le VOLUME module */
            float wgt=(t->maritime?sea_w:land_w)*(0.5f+0.5f*vol);                /* la MER pèse FORT */
            for (int ar=0; ar<ARCH_COUNT; ar++){
                bool bears = region_bears_arch(econ, far, ar, cen, present);
                if (bears && wgt>gbest[ar][po]) gbest[ar][po]=wgt;              /* le MEILLEUR lien par ENTITÉ */
            }
        }
        float prof=tune_f("SYNC_TRADE_PROFOND",2.0f), met=tune_f("SYNC_TRADE_METIER",1.0f);
        for (int ar=0; ar<ARCH_COUNT; ar++){
            float score=0.f; for (int o=0;o<SCPS_MAX_COUNTRY;o++) score+=gbest[ar][o];   /* SOMME des entités distinctes */
            Profondeur ch = (score>=prof)?PROF_PROFOND : (score>=met)?PROF_METIER : (score>0.f)?PROF_SURFACE : PROF_NONE;
            if (depth[ar] < (unsigned char)ch) depth[ar]=(unsigned char)ch;
        }
    }
}
/* Fallbacks compilés des seuils de la BARRE D'ACCÈS (registre scps_tune_list.h). */
#ifndef METAB_TIER1
#define METAB_TIER1 0.10f
#define METAB_TIER2 0.20f
#define METAB_TIER3 0.35f
#endif

/* BARRE D'ACCÈS GRADUÉE (Temps 2) — par héritage, le TIER d'accès (0..3) recherchable, en MAX
 * de DEUX voies : (1) la PROFONDEUR de contact (ai_archetype_depth : commerce→SURFACE→tier 1,
 * frontière/foi→MÉTIER→tier 2, gouvernance digérée→PROFOND→tier 3) — « les techs s'échangent
 * par le commerce jusqu'à un seuil » ; (2) la MÉTABOLISATION active (part d'âmes digérées de cet
 * héritage : ≥T1/T2/T3 ⇒ tier 1/2/3) — « incorporer ce peuple ouvre ses techs ». L'héritage
 * NATIF = plein (tier 3). Encodé 2 bits/héritage (cf. tech_heritage_access_tier). */
static unsigned heritage_access_pack(const unsigned char depth[ARCH_COUNT],
                                     const float metab[HERITAGE_COUNT], Heritage native){
    float t1=tune_f("METAB_TIER1",METAB_TIER1), t2=tune_f("METAB_TIER2",METAB_TIER2),
          t3=tune_f("METAB_TIER3",METAB_TIER3);
    unsigned m=0;
    for (int r=0;r<HERITAGE_COUNT;r++){
        int dt = (depth[r]>=(unsigned char)PROF_PROFOND)?3
               : (depth[r]>=(unsigned char)PROF_METIER) ?2
               : (depth[r]>=(unsigned char)PROF_SURFACE)?1 : 0;
        float mb = metab?metab[r]:0.f;
        int mt = (mb>=t3)?3 : (mb>=t2)?2 : (mb>=t1)?1 : 0;
        int t = dt>mt?dt:mt;
        if ((int)r==(int)native) t=3;               /* son propre héritage : accès PLEIN */
        m |= ((unsigned)(t&3)) << (2*r);
    }
    return m;
}
unsigned ai_heritage_access(const World *w, const WorldEconomy *econ, const RouteNetwork *rn, int cid){
    unsigned char depth[ARCH_COUNT]; ai_archetype_depth(w, econ, rn, cid, depth);
    float metab[HERITAGE_COUNT]; econ_country_heritage_digested(w, econ, cid, metab);
    return heritage_access_pack(depth, metab, ai_capital_heritage(w, econ, cid));
}
/* §syncrétique — rafraîchit le cercle d'un empire : cache la profondeur de contact par
 * archétype (lue par la membrane) et loquette les nœuds de diffusion atteints. */
void ai_sync_refresh(const World *w, const WorldEconomy *econ, const RouteNetwork *rn, TechState *ts, int cid){
    if (!ts) return;
    unsigned char adepth[ARCH_COUNT];
    ai_archetype_depth(w, econ, rn, cid, adepth);
    for (int r=0;r<ARCH_COUNT;r++) ts->arch_depth[r]=adepth[r];
    tech_sync_tick(ts, adepth);                                 /* §8 : diffusion par contact (auto-latch) */
}

/* REMISE DE PRIX PAR DIFFUSION (métabolisation, 3e effet) — « une tech déjà déverrouillée par
 * un autre empire coûte moins cher » : le savoir diffuse, la (re)découverte d'un savoir RÉPANDU
 * est plus facile. g_tech_diff[id] = nb d'empires VIVANTS qui possèdent la tech (recalculé chaque
 * tick par tech_diffusion_refresh, appelé depuis sim_day — DÉTERMINISTE, non sérialisé : fonction
 * pure des TechState). À 0 (bancs qui n'appellent pas refresh) ⇒ mult=1 (aucune remise). */
#ifndef AI_TECH_DIFFUSE_MAX
#define AI_TECH_DIFFUSE_MAX 0.40f   /* remise MAX (tech possédée par tous les autres ⇒ −40 %) */
#endif
static int g_tech_diff[TECH_COUNT];
static int g_tech_living = 0;
void tech_diffusion_refresh(const World *w, const TechState *all, int n_ts){
    for (int i=0;i<TECH_COUNT;i++) g_tech_diff[i]=0;
    g_tech_living=0;
    if (!w || !all) return;
    for (int c=0;c<w->n_countries && c<n_ts;c++){
        PolityRole role=w->country[c].role;
        if (role!=POLITY_PLAYER && role!=POLITY_ANTAGONIST) continue;
        g_tech_living++;
        for (int i=0;i<TECH_COUNT;i++) if (all[c].unlocked[i]) g_tech_diff[i]++;
    }
}
float tech_diffusion_mult(TechId id){
    if (id<0 || id>=TECH_COUNT || g_tech_living<=1) return 1.f;
    float frac = (float)g_tech_diff[id] / (float)g_tech_living;   /* part qui la possède déjà */
    if (frac>1.f) frac=1.f;
    return 1.f - tune_f("AI_TECH_DIFFUSE_MAX",AI_TECH_DIFFUSE_MAX) * frac;
}
/* coût EFFECTIF d'une tech pour l'IA : géologie (√N) × biais d'éthos × remise de diffusion. */
static float ai_effective_cost(TechId id, float nprov, Ethos eth){
    return tech_cost(id, nprov) * ai_tech_cost_mult(eth, tech_node(id)) * tech_diffusion_mult(id);
}

/* Le nœud à déverrouiller : score = BUTS (la fonction répond au besoin lu) +
 * PENCHANT de heritage (biais vers son thème + ses signatures) − FREIN (le faustien
 * n'est pris que si la pente dépasse le frein). Aucun « si heritage==X ». */
static TechId ai_pick_tech(const AiActor *a, const TechState *ts, const World *w,
                           const WorldEconomy *econ, const WorldProsperity *wp,
                           unsigned access, float nprov){
    AiView v = ai_observe(wp, w, econ, a->cid);
    float brake = ai_consolidation_pressure(&v);
    TechTheme affinity = tech_heritage_affinity(ai_capital_heritage(w,econ,a->cid));
    Ethos eth = ai_capital_ethos(w,econ,a->cid);           /* §éthos : biais de coût par fonction */
    float faith_stance = ai_faith_stance(w,econ,a->cid);   /* §4 : orthodoxe interdit, culte sacralise */
    /* M4 (design §3) — les signaux du score MULTIPLICATIF : credo+valeurs de la
     * capitale (l'appétit faustien) et la MATIÈRE (un pays sans matière arcane va
     * moins loin dans le faustien — la carte gate la profondeur, pas un verrou). */
    Credo credo=CREDO_PLURALISTE; float valeurs=5.f;
    { int cp2=w->country[a->cid].capital_prov;
      int cr2=(cp2>=0&&cp2<w->n_provinces)?w->province[cp2].region:-1;
      if (cr2>=0&&cr2<econ->n_regions){ credo=econ->region[cr2].culture.credo;
                                        valeurs=econ->region[cr2].culture.valeurs; } }
    bool has_arcane_raw=false;
    for (int r=0;r<econ->n_regions && !has_arcane_raw;r++)
        if (econ->region[r].owner==a->cid &&
            (econ->region[r].raw_cap[RES_CELESTIAL_IRON]>0.1f ||
             econ->region[r].raw_cap[RES_ARCANE_CRYSTAL]>0.1f ||
             econ->region[r].raw_cap[RES_SALTPETER]>0.1f)) has_arcane_raw=true;
    TechId best=TECH_COUNT; float bestscore=-1e30f;
    for (int i=0;i<TECH_COUNT;i++){
        TechId id=(TechId)i;
        if (!tech_can_research(ts,id,access)) continue;
        const TechNode *n=tech_node(id);
        float cost=ai_effective_cost(id, nprov, eth);   /* géologie √N × biais d'éthos × remise de diffusion */
        if (cost > ts->research_points + 0.01f) continue;          /* pas encore les moyens */
        float score=0.f;
        /* BUTS — la fonction du nœud répond à un besoin lu de la VUE (pas de script). */
        if (n->func==FN_ARMEE)        score += 1.2f*a->w_expand + 2.0f*v.take_pressure + 0.25f*n->dMil;
        if (n->func==FN_PRODUCTION)   score += 1.0f*a->w_trade  + 1.5f*v.gap_acuity   + 0.25f*n->dEco;
        if (n->func==FN_RENFORCEMENT){ score += 1.0f*a->w_build + 0.4f*n->dK + 0.3f*n->dL;
            if (n->dFracture<0.f) score += 0.05f*v.fracture; }                 /* anti-fracture si fracturé */
        if (n->theme==THM_SOCIETE && n->func==FN_RENFORCEMENT) score += 0.6f*a->w_faith;
        /* M4 — LE SCORE MULTIPLICATIF (design §3) REMPLACE le biais plat de thème :
         * souche(thème) × éthos(FN) × credo(faustien) × matière. Le plancher +0.4
         * garde une pente même sans besoin aigu (la curiosité du domaine). */
        { float mult = ETHOS_FN[eth][n->func]
                     * ((n->theme==affinity) ? 1.6f : 1.0f)
                     * (n->faustian ? ai_faustian_appetite(credo, valeurs) : 1.0f)
                     * ((n->faustian && !has_arcane_raw) ? 0.6f : 1.0f);
          score = (score + 0.4f) * mult; }
        if (n->native!=HERITAGE_COUNT) score += AI_TECH_SIGNATURE;     /* une signature accessible se prend */
        /* FREIN — le faustien rapproche la Brèche : pris seulement si la pente l'emporte. */
        if (n->faustian){          /* la pente faustienne, FREINÉE ou BÉNIE par la foi (§4) */
            float religious = (faith_stance - 0.5f)*2.f;   /* −1 orthodoxe (sacrilège) … +1 culte */
            score += AI_TECH_FAUSTIAN*(a->w_faustian - brake) - 0.3f*n->charge + AI_FAITH_FAUSTIAN*religious;
        }
        if (id==TECH_FOREUSE && v.chain_gap==RES_IRON)      /* §4 COUPLAGE : l'empire affamé de fer COURT à la foreuse */
            score += AI_FOREUSE_HUNGER;
        score -= 0.002f*cost;      /* à score égal : le plus proche (le moins cher) d'abord */
        if (score>bestscore){ bestscore=score; best=id; }
    }
    return best;
}

float ai_research_income(const TechState *ts, float pop){
    if (!ts) return 0.f;
    return (AI_RESEARCH_RATE/365.f) * tech_research_yield(ts) * (1.f + pop/AI_RESEARCH_POPREF);
}

/* S3 — LE PROCHAIN PAS vers une cible (beeline) : remonte la chaîne de prérequis de `target`
 * et renvoie le nœud LE PLUS PROFOND non encore acquis qui soit RECHERCHABLE (accès + prérequis).
 * TECH_COUNT si la cible est déjà acquise OU si le pas suivant reste verrouillé (archétype/ruine). */
static TechId ai_step_toward(const TechState *ts, TechId target, unsigned access){
    if (target<0||target>=TECH_COUNT||ts->unlocked[target]) return TECH_COUNT;
    TechId chain[24]; int nc=0; TechId t=target;
    while (t>=0 && t<TECH_COUNT && nc<24){ chain[nc++]=t; t=tech_node(t)->prereq; }
    for (int i=nc-1;i>=0;i--){                 /* de la racine VERS la cible : le 1er non-acquis = le pas suivant */
        if (ts->unlocked[chain[i]]) continue;
        return tech_can_research(ts, chain[i], access) ? chain[i] : TECH_COUNT;
    }
    return TECH_COUNT;
}

/* FAU0.6 — LE DÉTECTEUR DE FAMINE GÉNÉRALISÉ (généralise AI_FOREUSE_HUNGER au-delà du fer) :
 * l'empire est-il en PÉNURIE du bien `g` ? (demande > offre soutenue ET stock bas, sur tout le
 * pays). Lu des MÊMES données que la membrane — fer/bois (intrants) ET nourriture (subsistance). */
static bool ai_resource_famine(const WorldEconomy *econ, int cid, Resource g){
    if (g<=RES_NONE||g>=RES_COUNT) return false;
    float stock=0.f, demand=0.f, supply=0.f;
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid){
        stock+=econ->region[r].stock[g]; demand+=econ->region[r].demand[g]; supply+=econ->region[r].supply[g];
    }
    return demand > supply*1.15f + 1.f && stock < demand;
}

void ai_research_step(AiActor *a, TechState *ts, const World *w,
                      const WorldEconomy *econ, const RouteNetwork *rn,
                      const WorldProsperity *wp, int day){
    if (!ts || day < a->next_research_day) return;
    a->next_research_day = day + AI_RESEARCH_CADENCE;
    float pop = ai_country_population(w, econ, a->cid);
    float nprov = (float)w->country[a->cid].n_regions;   /* coût des techs ∝ √N (provinces), découplé de la pop */
    /* ASSIETTE : la pop PRODUIT la recherche (revenu ∝ pop) ; le COÛT monte ∝ √N (provinces),
     * sous-linéaire → l'expansion (wide) est récompensée (coût marginal < apport), sans snowball. */
    float income = (AI_RESEARCH_RATE/365.f)*AI_RESEARCH_CADENCE
                 * tech_research_yield(ts) * (1.f + pop/AI_RESEARCH_POPREF);
    /* MÉTABOLISATION (Temps 1) — un empire CREUSET (qui a digéré des âmes d'un autre
     * héritage) cherche plus vite : « incorporer d'autres gens dans sa culture fonctionne ».
     * Signal ~0 tôt (l'assimilation prend des décennies) ⇒ la fenêtre golden ne bouge pas. */
    income *= (1.f + tune_f("AI_METAB_RES_W",AI_METAB_RES_W)
                     * econ_country_metabolized(w, econ, a->cid));
    ts->research_points += income;
    ai_sync_refresh(w, econ, rn, ts, a->cid);                   /* §4-13 : cache la profondeur + loquette la diffusion (+ S1 : le commerce) */
    /* BARRE D'ACCÈS (Temps 2) : tier par héritage = MAX(profondeur cachée, métabolisation). */
    float metab[HERITAGE_COUNT]; econ_country_heritage_digested(w, econ, a->cid, metab);
    unsigned access = heritage_access_pack(ts->arch_depth, metab, ai_capital_heritage(w, econ, a->cid));
    TechId pick = ai_pick_tech(a, ts, w, econ, wp, access, nprov);
    /* §4 COUPLAGE : une fois l'Industrie en poche, l'empire AFFAMÉ DE FER ÉPARGNE pour la
     * foreuse (chère, faustienne) plutôt que d'éparpiller — l'issue tentante précipite sa Brèche. */
    if (pick!=TECH_FOREUSE && tech_can_research(ts, TECH_FOREUSE, access)){
        AiView vg = ai_observe(wp, w, econ, a->cid);
        if (vg.chain_gap==RES_IRON) return;        /* on garde les points : la foreuse d'abord */
    }
    /* FAU5 — LA PENTE TOURNE : la famine de BOIS/NOURRITURE court à son transmuteur (réplicateur/
     * corne), MAIS l'IA PÈSE l'échappatoire contre la Brèche — au BORD (proximité de crise élevée)
     * elle N'Y CÈDE PAS ; loin du seuil et désespérée, OUI. Jamais sans la tech. (≠ la foreuse,
     * §4/S3, qui garde son ressort propre.) Beeline via ai_step_toward (à travers TECH_ALCHIMIE). */
    if (tech_crisis_proximity(ts) < tune_f("FAUST_BRECHE_CAUTION",0.55f)){
        TechId tgt = ai_resource_famine(econ,a->cid,RES_WOOD)  ? TECH_TRANSMUTATION
                   : ai_resource_famine(econ,a->cid,RES_GRAIN) ? TECH_FORGE_RUNES : TECH_COUNT;
        if (tgt!=TECH_COUNT && !ts->unlocked[tgt]){
            TechId step=ai_step_toward(ts, tgt, access);
            if (step!=TECH_COUNT){
                float sc=ai_effective_cost(step, nprov, ai_capital_ethos(w,econ,a->cid));
                if (ts->research_points < sc) return;      /* on ÉPARGNE pour le pas suivant */
                pick=step;                                 /* on AVANCE vers l'échappatoire */
            }
        }
    }
    /* S1 — LA GREFFE CULTURELLE (la cristallisation de Venise) : un empire INVESTISSEUR
     * (mercantile/bâtisseur) ÉPARGNE pour acquérir le métier-signature d'un archétype qu'il a
     * ATTEINT — par le COMMERCE (ma route maritime, S1) ou la gouvernance —, au lieu d'éparpiller.
     * Sans ce ressort les signatures (tier-3, chères) ne sont JAMAIS payées (l'IA gloutonne prend
     * toujours le moins cher) : c'est le MÊME mécanisme que la foreuse. Il TIENT les points jusqu'à
     * l'acquérir (pas de fuite). Borné : NON-faustien (le faustien = S3) et ≤ 2 greffes (le 2e
     * archétype) — la guerre n'INTERROMPT pas (l'investisseur, peu belliqueux, finit sa greffe). */
    if (a->w_trade > 0.5f || a->w_build > 0.5f){
        int got=0;
        for (int id=0; id<TECH_COUNT; id++)
            if (ts->unlocked[id] && tech_node((TechId)id)->native!=HERITAGE_COUNT) got++;
        if (got < 2){
            Ethos eg = ai_capital_ethos(w,econ,a->cid);
            TechId sig=TECH_COUNT; float sigcost=1e30f;
            for (int id=0; id<TECH_COUNT; id++){
                const TechNode *tn=tech_node((TechId)id);
                if (tn->native==HERITAGE_COUNT || tn->faustian || ts->unlocked[id]) continue;
                if (!tech_can_research(ts, (TechId)id, access)) continue;     /* accessible (accès+prérequis) */
                float cc=ai_effective_cost((TechId)id, nprov, eg);
                if (cc<sigcost){ sig=(TechId)id; sigcost=cc; }                /* la moins chère d'abord */
            }
            if (sig!=TECH_COUNT){
                if (ts->research_points < sigcost) return;                    /* pas les moyens : on ÉPARGNE (et TIENT) */
                pick = sig;                                                   /* assez de Savoir : on ACQUIERT la greffe */
            }
        }
    }
    /* S3 — LA QUÊTE FAUSTIENNE DE L'EMBLÈME (forge runique × arcane = métallurgiste × ésotérique).
     * Le verrou n'était PAS le frein (déjà franchissable) mais la PROFONDEUR : l'emblème est tier-3
     * derrière la Poudrière, jamais atteinte par l'IA gloutonne, et AUCUN empire n'est assez
     * transgressif pour s'y forcer. Un empire à fort APPÉTIT faustien (pluraliste à HAUTES valeurs
     * OU culte — ai_faustian_appetite ≥ seuil), DEUX archétypes en main (S1 : commerce/gouvernance),
     * BEELINE la chaîne (Poudrière → Forge à runes) en épargnant à chaque pas. RARE (la porte
     * d'appétit) et COÛTEUX (la charge → Brèche borne les conséquences) ; on NE touche PAS la
     * foreuse. Le frein AI_TECH_FAUSTIAN abaissé (2.5→1.2) scelle la rencontre appétit/frein. */
    if (!ts->unlocked[TECH_FORGE_RUNES]
        && tech_heritage_access_tier(access, HERITAGE_METALLURGISTE)>=3
        && tech_heritage_access_tier(access, HERITAGE_ESOTERIQUE)>=3){
        Credo cr=CREDO_PLURALISTE; float val=5.f;
        { int cp=w->country[a->cid].capital_prov;
          int crg=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
          if (crg>=0&&crg<econ->n_regions){ cr=econ->region[crg].culture.credo; val=econ->region[crg].culture.valeurs; } }
        if (ai_faustian_appetite(cr, val) >= AI_FAUST_QUEST){               /* la SOIF d'interdit (rare) */
            TechId step=ai_step_toward(ts, TECH_FORGE_RUNES, access);
            if (step!=TECH_COUNT){
                float sc=ai_effective_cost(step, nprov, ai_capital_ethos(w,econ,a->cid));
                if (ts->research_points < sc) return;                      /* on ÉPARGNE pour le pas suivant */
                pick=step;                                                 /* on AVANCE vers l'emblème */
            }
        }
    }
    /* DOCTRINE MILITAIRE (S4) — l'empire BEELINE la tech de SON unité de doctrine (sinon l'IA
     * gloutonne ne paie jamais ces tiers, et la chaîne d'armes F-arc n'a aucun consommateur — les
     * unités avancées n'apparaissent jamais) : MARTIAL (Dominateur/Honneur) → Poudrière (arquebusier) ;
     * ARCANE (appétit faustien) → Magie de bataille (mage) puis Alchimie (alchimiste). Même ressort
     * d'épargne que la foreuse/S3. La Forge runique reste S3 (l'emblème) → on lui cède la priorité ;
     * Caserne→hallebardier est déjà commune. Épargne bornée : on AVANCE d'un pas par tour. */
    { Ethos eth=ai_capital_ethos(w,econ,a->cid);
      TechId tgt=TECH_COUNT;
      if (eth==ETHOS_DOMINATEUR||eth==ETHOS_HONNEUR){
          if (!ts->unlocked[TECH_POUDRIERE]) tgt=TECH_POUDRIERE;
      } else if (a->w_faustian>0.30f){
          bool s3=tech_heritage_access_tier(access,HERITAGE_METALLURGISTE)>=3&&tech_heritage_access_tier(access,HERITAGE_ESOTERIQUE)>=3&&!ts->unlocked[TECH_FORGE_RUNES];
          if (!s3){                                              /* sinon la Forge runique (S3) a la priorité */
              if      (!ts->unlocked[TECH_MAGIE_BATAILLE]) tgt=TECH_MAGIE_BATAILLE;
              else if (!ts->unlocked[TECH_ALCHIMIE])       tgt=TECH_ALCHIMIE;
          }
      }
      if (tgt!=TECH_COUNT){
          TechId step=ai_step_toward(ts, tgt, access);
          if (step!=TECH_COUNT){
              float sc=ai_effective_cost(step, nprov, eth);
              if (ts->research_points < sc) return;              /* on ÉPARGNE pour le pas suivant */
              pick=step;                                         /* on AVANCE vers la tech d'unité */
          }
      } }
    if (pick!=TECH_COUNT){
        float cost = ai_effective_cost(pick, nprov, ai_capital_ethos(w,econ,a->cid));
        if (ts->research_points >= cost && tech_research(ts, pick, access)){
            ts->research_points -= cost;
            a->stats.techs++;
            if (tech_node(pick)->faustian){ a->stats.techs_faustian++;
                faction_lever_apply(a->cid, FAC_TRANSGRESSEUR, AI_LEVER_TECH);  /* §4 : franchir l'interdit AVANCE les Transgresseurs */
            }
        }
    }
    /* §4c : le gate de l'esclavage. La TECH (Économie servile, signature Clanique) l'INSTITUE ;
     * mais un éthos CONQUÉRANT (Dominateur/Honneur) déporte le vaincu par COUTUME sans l'avoir
     * recherché — l'esclavage devient la pratique des sociétés belliqueuses (Rome, empires
     * esclavagistes), pas d'une seule ethnie. Le VOLUME reste faible (SLAVE_FRACTION) et l'apport
     * de savoir marginal (METAB_DIFFUSE_SLAVE) : présent, jamais massif. */
    { Ethos eeth = ai_capital_ethos(w, econ, a->cid);
      a->can_enslave = ts->unlocked[TECH_ESCLAVAGE]
                    || eeth==ETHOS_DOMINATEUR || eeth==ETHOS_HONNEUR; }
    if (a->cid>=0 && a->cid<SCPS_MAX_COUNTRY) g_ai_enslave[a->cid]=a->can_enslave;  /* cache pour le règlement d'un TIERS */
    a->has_creuset = ts->unlocked[TECH_INTEGRATION]; /* §leviers : le Creuset forme mieux */
    a->has_halles  = ts->unlocked[TECH_HALLES];      /* E3 : l'IA stockeuse exige les Halles */
}

/* ===================================================================== */
/* E3 — L'IA STOCKEUSE : acheter bas vers l'entrepôt, vendre haut          */
/* ===================================================================== */
/* B1 — la spéculation LISSE, elle ne secoue pas. L'ancien réglage (25 % du stock
 * aspiré, 50 % du magot dumpé par tick) faisait MONTER la variance des prix
 * (σ 0.025→0.029) : l'IA s'auto-impactait. Désormais des gestes MENUS, bornés en
 * volume, avec un cooldown par bien, bandes symétriques. */
#define SPEC_BUY_BAND   0.80f   /* prix < 0.80 × moyenne mobile → acheter */
#define SPEC_SELL_BAND  1.25f   /* prix > 1.25 × moyenne mobile → vendre (symétrique : 1/0.8) */
#define SPEC_GOLD_FLOOR 350.f   /* le trésor JAMAIS saigné : la spéculation est un emploi du surplus */
#define SPEC_VOL_FRAC   0.05f   /* B1 — au plus 5 % du stock régional par geste (était 25 %) */
#define SPEC_VOL_ABS    50.f    /* B1 — et au plus 50 unités par geste (plafond dur) */
#define SPEC_SELL_FRAC  0.15f   /* B1 — vente EN FILET : 15 % du magot par tick (était 50 %) */
#define SPEC_COOLDOWN   1        /* B1 — repos d'un tick (≈30 j) après un geste sur un bien */
void ai_speculate_tick(AiActor *a, WorldEconomy *econ){
    if (!a || !econ) return;
    int hub = intertrade_country_centre(econ, a->cid);
    if (hub<0) hub = a->home_region;
    if (hub<0 || hub>=econ->n_regions) return;
    RegionEconomy *re=&econ->region[hub];
    if (re->owner != a->cid) return;
    /* la moyenne mobile (~1 an) s'entretient TOUJOURS — même sans entrepôt
     * (l'IA apprend ses prix avant d'avoir les murs pour jouer). */
    for (int g=1; g<RES_COUNT; g++){
        float p=re->price[g];
        a->spec_avg[g] = (a->spec_avg[g]<=0.f)? p : a->spec_avg[g]*(11.f/12.f) + p*(1.f/12.f);
        if (a->spec_cd[g]>0) a->spec_cd[g]--;   /* le repos s'écoule, même sans entrepôt */
    }
    if (re->n_entrepot<1) return;            /* sans Entrepôt : cap 200, pas de jeu */
    float space = ECON_STOCK_CAP_ENTREPOT*(float)re->n_entrepot;   /* l'aile spéculative */
    float held=0.f; for (int g=1;g<RES_COUNT;g++) held+=a->hoard[g];
    float cap_stock = ECON_STOCK_CAP_BASE + ECON_STOCK_CAP_ENTREPOT*(float)re->n_entrepot;
    for (int g=1; g<RES_COUNT; g++){
        if (a->spec_cd[g]>0) continue;                        /* B1 — ce bien se repose */
        float p=re->price[g], xb=a->spec_avg[g];
        if (xb<=0.f || p<=0.f) continue;
        if (p < tune_f("SPEC_BUY_BAND",SPEC_BUY_BAND)*xb && re->treasury > tune_f("SPEC_GOLD_FLOOR",SPEC_GOLD_FLOOR) && held < space){
            float vol = fminf(re->stock[g]*SPEC_VOL_FRAC, space-held);                /* ≤ 5 % du stock — le bien QUITTE le marché */
            vol = fminf(vol, SPEC_VOL_ABS);                                           /* ≤ 50 unités (plafond dur) */
            vol = fminf(vol, (re->treasury-tune_f("SPEC_GOLD_FLOOR",SPEC_GOLD_FLOOR))*0.25f/fmaxf(p,0.05f));    /* jamais la famine d'or */
            if (vol>=1.f){
                re->stock[g]-=vol; a->hoard[g]+=vol; held+=vol;
                re->treasury -= vol*p;
                a->stats.spec_vol+=vol; a->stats.spec_gold-=vol*p; a->stats.spec_buys++;
                a->spec_cd[g]=SPEC_COOLDOWN;
            }
        } else if (p > tune_f("SPEC_SELL_BAND",SPEC_SELL_BAND)*xb && a->hoard[g]>=1.f){
            float vol = fminf(a->hoard[g]*SPEC_SELL_FRAC, SPEC_VOL_ABS);              /* filet : 15 % du magot, ≤ 50 u */
            float room = cap_stock - re->stock[g]; if (vol>room) vol=room;            /* l'entrepôt ne déborde pas */
            if (vol>=1.f){
                a->hoard[g]-=vol; re->stock[g]+=vol;                                  /* le bien REVIENT au marché */
                re->treasury += vol*p;
                a->stats.spec_vol+=vol; a->stats.spec_gold+=vol*p; a->stats.spec_sells++;
                a->spec_cd[g]=SPEC_COOLDOWN;
            }
        }
    }
}

/* ===================================================================== */
/* LE TICK : dormir → lire → choisir un levier → agir (verbes du joueur)   */
/* ===================================================================== */
void ai_step(AiActor *a, World *w, WorldEconomy *econ, WorldProsperity *wp,
             WorldLegitimacy *wl, AgencyState *ag, RouteNetwork *rn,
             DiploState *diplo, const Statecraft *sc, int day){
    if (a->cid<0 || a->cid>=w->n_countries) return;
    bool econ_due  = (day >= a->next_econ_day);
    bool strat_due = (day >= a->next_strat_day);
    if (!econ_due && !strat_due) return;

    AiView v = ai_observe(wp, w, econ, a->cid);
    float brake = ai_consolidation_pressure(&v);

    if (econ_due){
        ai_econ_turn(a, w, econ, &v, ag, rn, brake, day);
        /* I5 — L'AUDIT DES OFFICES : un État dont la CORRUPTION dépasse 40 réprime la
         * capture des factions — coût (50+8·corr)·IPM (×2 si une faction TIENT l'État),
         * corr −20. La légitimité MONTE de 0.3 si corr>50 (l'assainissement applaudi),
         * sinon BAISSE de 0.3 (la chasse aux sorcières inquiète) : l'IA accepte ce coût
         * de réputation entre 40 et 50 pour casser la capture tôt. Cooldown 5 ans. Un
         * sink en or de plus : la probité se paie. */
        if (day >= a->next_audit_day){
            int corr = faction_corruption_0_100(a->cid);
            int cr = (a->home_region>=0 && a->home_region<econ->n_regions) ? a->home_region : -1;
            if (corr > 40 && cr>=0){
                bool held = faction_capture_total(a->cid) > 0.4f;            /* une faction TIENT → elle résiste */
                float cost = (50.f + 8.f*(float)corr) * econ_world_ipm(econ) * (held?2.f:1.f);
                /* RE-KEY PROVINCE : treasury province-owned — route sur la représentative. */
                int crp=econ_region_rep_province(econ,cr);
                if (crp>=0 && crp<econ->n_prov && econ->prov[crp].treasury >= cost){
                    econ->prov[crp].treasury -= cost;
                    econ_flux_add(a->cid, FX_AUDIT, -cost);    /* I0 : la ligne audits */
                    faction_audit(a->cid);
                    if (wl) wl->L[cr] = clampf(wl->L[cr] + (corr>50?0.3f:-0.3f), 0.f, 10.f);
                    a->next_audit_day = day + 5*SCPS_DAYS_PER_YEAR;
                }
            }
        }
        ai_relocate_turn(a, econ, &v, day);   /* §reloc : peupler sa province-ressource pour combler une pénurie */
        ai_interior_turn(a, w, econ, ag, diplo, &v, day);   /* §leviers : mater/former/purger selon l'éthos */
        /* §leviers — GUERRE COMMERCIALE : l'embargo est l'arme PRINCIPALE du Mercantile
         * (il ne purge pas, mauvais pour les affaires — il étrangle). Cible : l'hégémon
         * perçu, s'il n'est ni allié ni client (un pacte de cité est sacré). */
        if (day >= a->next_embargo_day){
            a->next_embargo_day = day + 3600;            /* ~10 ans entre deux décrets */
            if (ai_capital_ethos(w,econ,a->cid)==ETHOS_MERCANTILE){
                int heg=diplo_perceived_hegemon(w, econ, wp, diplo, a->cid);
                if (heg>=0 && heg!=a->cid
                    && diplo_status(diplo,a->cid,heg)!=DIPLO_ALLIED
                    && !diplo_trade_pact(diplo,a->cid,heg)
                    && diplo_suzerain(diplo,a->cid)!=heg)
                    intertrade_order_embargo(a->cid, heg, true);
            }
        }
        a->next_econ_day = day + AI_ECON_CADENCE/2 + (int)(frand(&a->rng)*AI_ECON_CADENCE);
    }
    if (strat_due){
        ai_refresh_ethos(a, w, econ);   /* §3 : l'éthos effectif GLISSE avec la composition avant d'agir */
        ai_strat_turn(a, w, econ, wp, wl, diplo, sc, &v, brake, day);
        a->next_strat_day = day + AI_STRAT_CADENCE/2 + (int)(frand(&a->rng)*AI_STRAT_CADENCE);
    }
}
